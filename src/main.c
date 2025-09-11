/*
 * Man Page Compiler (MPC) - Documentation-driven CLI utility generation
 * 
 * BSD 3-Clause License
 * Copyright (c) 2025, Divan Visagie
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <curl/curl.h>
#include <jansson.h>

#define MAX_LINE_LENGTH 4096
#define MAX_SECTIONS 32
#define MAX_OPTIONS 64
#define MAX_EXAMPLES 32
#define MAX_PROMPT_SIZE (1024 * 1024)  // 1MB for LLM prompt

typedef struct {
    char *flags[8];
    int flag_count;
    char *description;
    int takes_value;
} option_t;

typedef struct {
    char *command;
    char *expected_output;
    char *description;
} example_t;

typedef struct {
    char *name;
    char *description;
    char *synopsis;
    char *sections[MAX_SECTIONS];
    char *section_names[MAX_SECTIONS];
    int section_count;
    option_t options[MAX_OPTIONS];
    int option_count;
    example_t examples[MAX_EXAMPLES];
    int example_count;
} manpage_t;

typedef struct {
    char *data;
    size_t size;
} http_response_t;

// Configuration
typedef struct {
    char *input_file;
    char *output_file;
    char *target_language;

    char *llm_model;
    char *llm_url;
    char *api_key;
    int use_remote;
    int verbose;
    int debug;
    int max_retries;
} config_t;

// HTTP response callback for curl
size_t write_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
    size_t total_size = size * nmemb;
    
    response->data = realloc(response->data, response->size + total_size + 1);
    if (!response->data) {
        printf("Error: Memory allocation failed\n");
        return 0;
    }
    
    memcpy(response->data + response->size, contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    
    return total_size;
}

// Parse man page into structured format
int parse_manpage(const char *filename, manpage_t *manpage) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open file %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    char *current_section = NULL;
    char *section_content = malloc(8192);
    section_content[0] = '\0';
    
    memset(manpage, 0, sizeof(manpage_t));
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Parse .TH header
        if (strncmp(line, ".TH ", 4) == 0) {
            char *name_start = line + 4;
            char *name_end = strchr(name_start, ' ');
            if (name_end) {
                *name_end = '\0';
                manpage->name = strdup(name_start);
                // Convert to lowercase
                for (int i = 0; manpage->name[i]; i++) {
                    if (manpage->name[i] >= 'A' && manpage->name[i] <= 'Z') {
                        manpage->name[i] += 32;
                    }
                }
            }
        }
        
        // Parse .SH sections
        else if (strncmp(line, ".SH ", 4) == 0) {
            // Save previous section
            if (current_section && strlen(section_content) > 0) {
                manpage->section_names[manpage->section_count] = strdup(current_section);
                manpage->sections[manpage->section_count] = strdup(section_content);
                manpage->section_count++;
            }
            
            // Start new section
            free(current_section);
            current_section = strdup(line + 4);
            section_content[0] = '\0';
        }
        
        // Skip groff commands but collect content
        else if (line[0] != '.' && strlen(line) > 0) {
            if (strlen(section_content) > 0) {
                strcat(section_content, " ");
            }
            strcat(section_content, line);
        }
        
        // Parse options in OPTIONS section
        else if (strncmp(line, ".BR ", 4) == 0 || strncmp(line, ".B ", 3) == 0) {
            if (current_section && strcmp(current_section, "OPTIONS") == 0) {
                option_t *opt = &manpage->options[manpage->option_count];
                
                // Extract flags
                char *flag_text = strchr(line, '-');
                if (flag_text) {
                    char *token = strtok(flag_text, " ,\"");
                    while (token && opt->flag_count < 8) {
                        if (token[0] == '-') {
                            opt->flags[opt->flag_count++] = strdup(token);
                        }
                        token = strtok(NULL, " ,\"");
                    }
                    if (opt->flag_count > 0) {
                        manpage->option_count++;
                    }
                }
            }
        }
        
        // Parse examples
        else if (strncmp(line, "$ ", 2) == 0) {
            if (current_section && strcmp(current_section, "EXAMPLES") == 0) {
                example_t *ex = &manpage->examples[manpage->example_count++];
                ex->command = strdup(line);
                ex->description = strdup("Example from man page");
            }
        }
    }
    
    // Save final section
    if (current_section && strlen(section_content) > 0) {
        manpage->section_names[manpage->section_count] = strdup(current_section);
        manpage->sections[manpage->section_count] = strdup(section_content);
        manpage->section_count++;
    }
    
    // Extract description from NAME section
    for (int i = 0; i < manpage->section_count; i++) {
        if (strcmp(manpage->section_names[i], "NAME") == 0) {
            char *dash_pos = strstr(manpage->sections[i], " - ");
            if (!dash_pos) {
                dash_pos = strstr(manpage->sections[i], " \\- ");
            }
            if (dash_pos) {
                manpage->description = strdup(dash_pos + 3);
            }
            break;
        }
    }
    
    // Extract synopsis
    for (int i = 0; i < manpage->section_count; i++) {
        if (strcmp(manpage->section_names[i], "SYNOPSIS") == 0) {
            manpage->synopsis = strdup(manpage->sections[i]);
            break;
        }
    }
    
    free(section_content);
    free(current_section);
    fclose(fp);
    
    printf("✓ Parsed man page: %s (%d sections, %d options, %d examples)\n", 
           manpage->name ? manpage->name : "unknown",
           manpage->section_count, manpage->option_count, manpage->example_count);
    
    return 0;
}

// Generate structured English specification for LLM
char* generate_specification(const manpage_t *manpage, const config_t *config, const char *error_feedback) {
    char *spec = malloc(MAX_PROMPT_SIZE);
    if (!spec) return NULL;
    
    snprintf(spec, MAX_PROMPT_SIZE,
        "Write a complete %s program named '%s' that implements this exact specification.\n\n"
        "DESCRIPTION: %s\n\n"
        "COMMAND LINE INTERFACE:\n"
        "Usage: %s\n\n"
        "OPTIONS:\n",
        config->target_language,
        manpage->name ? manpage->name : "program", 
        manpage->description ? manpage->description : "A command-line utility",
        manpage->synopsis ? manpage->synopsis : "program [OPTIONS]"
    );
    
    // Add options
    for (int i = 0; i < manpage->option_count; i++) {
        char flags_str[256] = "";
        for (int j = 0; j < manpage->options[i].flag_count; j++) {
            if (j > 0) strcat(flags_str, ", ");
            strcat(flags_str, manpage->options[i].flags[j]);
        }
        
        char option_text[512];
        snprintf(option_text, sizeof(option_text),
            "- **%s**\n  - %s\n\n",
            flags_str,
            manpage->options[i].description ? manpage->options[i].description : "Option description"
        );
        strcat(spec, option_text);
    }
    
    // Add examples
    if (manpage->example_count > 0) {
        strcat(spec, "### Examples and Expected Behavior\n\n");
        for (int i = 0; i < manpage->example_count; i++) {
            char example_text[512];
            snprintf(example_text, sizeof(example_text),
                "**Example %d:**\n```bash\n%s\n```\n\n",
                i + 1, manpage->examples[i].command
            );
            strcat(spec, example_text);
        }
    }
    
    // Add implementation requirements  
    strcat(spec, "\nREQUIREMENTS:\n");
    strcat(spec, "- Include ALL necessary #include statements\n");
    strcat(spec, "- Implement proper command-line argument parsing using getopt()\n");
    strcat(spec, "- Add error handling for invalid inputs\n");
    strcat(spec, "- Follow standard C conventions and POSIX compliance\n");
    strcat(spec, "- Make it compile without warnings on macOS and Linux\n");
    strcat(spec, "- Use standard C library functions (printf, fprintf, etc.)\n\n");
    
    strcat(spec, "OUTPUT: Complete working C source code ONLY.\n");
    strcat(spec, "Wrap in ```c code blocks.\n");
    strcat(spec, "Start immediately with #include statements.\n");
    strcat(spec, "DO NOT include explanations or markdown text.\n\n");
    
    strcat(spec, "NOTE: C code generation works exceptionally well because LLMs have been\n");
    strcat(spec, "extensively trained on Linux kernel code and mailing list discussions,\n");
    strcat(spec, "making them excellent at generating clean, portable C code.\n\n");
    
    // Add error feedback if this is a retry
    if (error_feedback && strlen(error_feedback) > 0) {
        strcat(spec, "PREVIOUS ATTEMPT FAILED WITH ERRORS:\n");
        strcat(spec, error_feedback);
        strcat(spec, "\n\nPlease fix these compilation errors in your response:\n");
        strcat(spec, "- Add missing #include statements\n");
        strcat(spec, "- Fix undefined variables and functions\n");
        strcat(spec, "- Ensure proper variable declarations\n");
        strcat(spec, "- Use correct C syntax and types\n\n");
    }
    
    return spec;
}

// Call LLM API to generate code
char* call_llm(const char *prompt, const config_t *config) {
    CURL *curl;
    CURLcode res;
    http_response_t response = {0};
    
    curl = curl_easy_init();
    if (!curl) {
        printf("Error: Failed to initialize curl\n");
        return NULL;
    }
    
    json_t *json = json_object();
    struct curl_slist *headers = NULL;
    char url[512];
    char *json_payload;
    
    if (config->use_remote) {
        // Gemini API format
        json_t *contents_array = json_array();
        json_t *content = json_object();
        json_t *parts_array = json_array();
        json_t *part = json_object();
        
        json_object_set_new(part, "text", json_string(prompt));
        json_array_append_new(parts_array, part);
        json_object_set_new(content, "parts", parts_array);
        json_array_append_new(contents_array, content);
        json_object_set_new(json, "contents", contents_array);
        
        json_payload = json_dumps(json, 0);
        
        // Gemini API URL
        snprintf(url, sizeof(url), 
            "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s", 
            config->llm_model, config->api_key);
        
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        printf("Generating code via Gemini %s...\n", config->llm_model);
        
    } else {
        // Ollama format
        json_object_set_new(json, "model", json_string(config->llm_model));
        json_object_set_new(json, "prompt", json_string(prompt));
        json_object_set_new(json, "stream", json_false());
        
        json_payload = json_dumps(json, 0);
        
        snprintf(url, sizeof(url), "%s/api/generate", config->llm_url);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        printf("Generating code via %s...\n", config->llm_model);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_decref(json);
    free(json_payload);
    
    if (res != CURLE_OK) {
        printf("Error: LLM request failed: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }
    
    // Parse response
    json_error_t error;
    json_t *response_json = json_loads(response.data, 0, &error);
    if (!response_json) {
        printf("Error: Failed to parse LLM response: %s\n", error.text);
        free(response.data);
        return NULL;
    }
    
    char *generated_code = NULL;
    
    if (config->use_remote) {
        // Parse Gemini response format
        json_t *candidates = json_object_get(response_json, "candidates");
        if (candidates && json_is_array(candidates)) {
            json_t *first_candidate = json_array_get(candidates, 0);
            if (first_candidate) {
                json_t *content = json_object_get(first_candidate, "content");
                if (content) {
                    json_t *parts = json_object_get(content, "parts");
                    if (parts && json_is_array(parts)) {
                        json_t *first_part = json_array_get(parts, 0);
                        if (first_part) {
                            json_t *text = json_object_get(first_part, "text");
                            if (text && json_is_string(text)) {
                                generated_code = strdup(json_string_value(text));
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Parse Ollama response format
        json_t *response_text = json_object_get(response_json, "response");
        if (response_text && json_is_string(response_text)) {
            generated_code = strdup(json_string_value(response_text));
        }
    }
    
    json_decref(response_json);
    free(response.data);
    
    if (generated_code) {
        printf("✓ Generated %zu bytes of code\n", strlen(generated_code));
        return generated_code;
    }
    
    printf("Error: No generated code found in response\n");
    return NULL;
}

// Extract C code from LLM response (handles markdown code blocks)
char* extract_code(const char *response) {
    const char *code_start = strstr(response, "```c");
    if (!code_start) {
        // Try generic code block
        code_start = strstr(response, "```");
        if (!code_start) {
            printf("Warning: No code blocks found, using entire response\n");
            return strdup(response);
        }
    }
    
    // Skip to content
    code_start = strchr(code_start, '\n');
    if (!code_start) return strdup(response);
    code_start++; // Skip newline
    
    // Find end marker
    const char *code_end = strstr(code_start, "```");
    if (!code_end) {
        printf("Warning: No closing code block found\n");
        return strdup(code_start);
    }
    
    // Extract code
    size_t code_length = code_end - code_start;
    char *code = malloc(code_length + 1);
    strncpy(code, code_start, code_length);
    code[code_length] = '\0';
    
    return code;
}

// Compile generated C code to executable
int compile_code(const char *code, const config_t *config, char **error_output) {
    char temp_file[256];
    char cmd[512];
    FILE *fp;
    
    // Create build directory
    system("mkdir -p build");
    
    // Generate C code file
    char *base = config->output_file ? basename(strdup(config->output_file)) : "program";
    snprintf(temp_file, sizeof(temp_file), "build/%s.c", base);
    
    fp = fopen(temp_file, "w");
    if (!fp) {
        printf("Error: Cannot create temporary file %s\n", temp_file);
        return -1;
    }
    fprintf(fp, "%s", code);
    fclose(fp);
    
    printf("Compiling C code to executable...\n");
    
    // Use clang on macOS, gcc on Linux
    const char *compiler = "clang";
    if (system("which clang > /dev/null 2>&1") != 0) {
        compiler = "gcc";
    }
    
    // Capture compilation output for error feedback
    char error_cmd[768];
    snprintf(error_cmd, sizeof(error_cmd), "%s -O2 -Wall -Wextra -std=c99 %s -o %s 2>&1", 
             compiler, temp_file, config->output_file);
    
    FILE *compile_output = popen(error_cmd, "r");
    if (compile_output) {
        char *output = malloc(4096);
        size_t output_size = 0;
        char buffer[256];
        
        while (fgets(buffer, sizeof(buffer), compile_output)) {
            size_t len = strlen(buffer);
            output = realloc(output, output_size + len + 1);
            strcpy(output + output_size, buffer);
            output_size += len;
        }
        
        int exit_code = pclose(compile_output);
        
        if (exit_code != 0) {
            printf("Error: %s compilation failed\n", compiler);
            printf("Check build/%s.c for generated code\n", base);
            if (error_output && output_size > 0) {
                *error_output = output;
            } else {
                free(output);
            }
            return -1;
        }
        
        free(output);
    } else {
        // Fallback to original method if popen fails
        snprintf(cmd, sizeof(cmd), "%s -O2 -Wall -Wextra -std=c99 %s -o %s", 
                 compiler, temp_file, config->output_file);
        
        if (system(cmd) != 0) {
            printf("Error: %s compilation failed\n", compiler);
            printf("Check build/%s.c for generated code\n", base);
            return -1;
        }
    }
    
    // Make executable
    chmod(config->output_file, 0755);
    
    return 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] INPUT.1\n\n", program_name);
    printf("Man Page Compiler - Translate man pages to working CLI utilities via LLMs\n\n");
    printf("Options:\n");
    printf("  -o OUTPUT     Output file (default: program name)\n");
    printf("  -m MODEL      LLM model (default: codellama:7b)\n");
    printf("  -u URL        Ollama URL (default: http://localhost:11434)\n");
    printf("  --remote      Use remote Gemini API (requires GEMINI_API_KEY)\n");
    printf("  -r RETRIES    Max retry attempts (default: 3)\n");
    printf("  -v            Verbose output\n");
    printf("  -d            Debug mode (keep intermediate files)\n");
    printf("  -h            Show this help\n\n");
    printf("Environment Variables:\n");
    printf("  GEMINI_API_KEY    Required when using --remote flag\n\n");
    printf("Examples:\n");
    printf("  mpc examples/simple.1\n");
    printf("  mpc -o calculator examples/calculator.1\n");
    printf("  mpc -m codellama:13b examples/hello.1\n");
    printf("  export GEMINI_API_KEY=your_key && mpc --remote -m gemini-1.5-flash examples/calculator.1\n");
    printf("  mpc --remote -m gemini-1.5-pro examples/hello.1\n");
}

int main(int argc, char *argv[]) {
    config_t config = {
        .target_language = "c",
        .llm_model = "codellama:7b",
        .llm_url = "http://localhost:11434",
        .api_key = NULL,
        .use_remote = 0,
        .verbose = 0,
        .debug = 0,
        .max_retries = 3
    };
    
    // Handle --remote flag separately since getopt doesn't handle long options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--remote") == 0) {
            config.use_remote = 1;
            config.llm_model = "gemini-1.5-flash";
            // Remove this argument from argv
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
    }
    
    int opt;
    while ((opt = getopt(argc, argv, "o:m:u:r:vdh")) != -1) {
        switch (opt) {
            case 'o': config.output_file = optarg; break;
            case 'm': config.llm_model = optarg; break;
            case 'u': config.llm_url = optarg; break;
            case 'r': config.max_retries = atoi(optarg); break;
            case 'v': config.verbose = 1; break;
            case 'd': config.debug = 1; break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }
    
    // Check for API key if using remote
    if (config.use_remote) {
        config.api_key = getenv("GEMINI_API_KEY");
        if (!config.api_key) {
            printf("Error: GEMINI_API_KEY environment variable not set\n");
            printf("Set it with: export GEMINI_API_KEY=your_api_key_here\n");
            return 1;
        }
    }
    
    if (optind >= argc) {
        printf("Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    config.input_file = argv[optind];
    
    // Default output file
    if (!config.output_file) {
        char *basename_copy = strdup(config.input_file);
        char *dot = strrchr(basename_copy, '.');
        if (dot) *dot = '\0';
        config.output_file = basename_copy;
    }
    

    
    printf("🚀 Man Page Compiler v1.0\n");
    printf("Input: %s\n", config.input_file);
    printf("Output: %s\n", config.output_file);
    printf("Target: %s\n", config.target_language);
    printf("Model: %s%s\n", config.llm_model, config.use_remote ? " (remote)" : " (local)");
    printf("Max retries: %d\n\n", config.max_retries);
    
    // Step 1: Parse man page
    printf("Parsing man page...\n");
    manpage_t manpage;
    if (parse_manpage(config.input_file, &manpage) != 0) {
        return 1;
    }
    
    // Step 2-4: Generate and compile code with retry logic
    char *error_feedback = NULL;
    char *final_code = NULL;
    
    for (int attempt = 1; attempt <= config.max_retries; attempt++) {
        printf("Generating structured specification... (attempt %d/%d)\n", attempt, config.max_retries);
        
        char *spec = generate_specification(&manpage, &config, error_feedback);
        if (!spec) {
            printf("Error: Failed to generate specification\n");
            free(error_feedback);
            return 1;
        }
        
        if (config.verbose) {
            printf("Generated specification (%zu chars):\n", strlen(spec));
            printf("---\n%s\n---\n", spec);
        }
        
        printf("Generating code via LLM... (attempt %d/%d)\n", attempt, config.max_retries);
        char *generated_code = call_llm(spec, &config);
        if (!generated_code) {
            printf("Error: Failed to generate code on attempt %d\n", attempt);
            free(spec);
            if (attempt == config.max_retries) {
                free(error_feedback);
                return 1;
            }
            continue;
        }
        
        printf("Extracting code from LLM response...\n");
        char *code = extract_code(generated_code);
        
        if (config.debug) {
            printf("Extracted code (attempt %d):\n---\n%s\n---\n", attempt, code);
        }
        
        printf("Compiling to executable... (attempt %d/%d)\n", attempt, config.max_retries);
        char *compile_error = NULL;
        if (compile_code(code, &config, &compile_error) == 0) {
            // Success!
            printf("Compilation successful on attempt %d\n", attempt);
            final_code = code;
            free(spec);
            free(generated_code);
            free(error_feedback);
            free(compile_error);
            break;
        } else {
            // Compilation failed
            printf("Compilation failed on attempt %d/%d\n", attempt, config.max_retries);
            
            if (attempt < config.max_retries && compile_error) {
                printf("Retrying with error feedback...\n");
                free(error_feedback);
                error_feedback = compile_error;
                
                if (config.verbose) {
                    printf("Compilation errors to send to LLM:\n---\n%s\n---\n", error_feedback);
                }
            } else {
                printf("Max retries exceeded. Final compilation failed.\n");
                free(spec);
                free(generated_code);
                free(code);
                free(error_feedback);
                free(compile_error);
                return 1;
            }
            
            free(spec);
            free(generated_code);
            free(code);
        }
    }
    
    printf("\nSuccess! Generated executable: %s\n", config.output_file);
    
    // Show file size
    struct stat st;
    if (stat(config.output_file, &st) == 0) {
        printf("File size: %lld bytes\n", (long long)st.st_size);
    }
    
    // Cleanup
    free(final_code);
    
    return 0;
}