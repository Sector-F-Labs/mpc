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

#include "config.h"
#include "manpage.h"
#include "spec.h"
#include "llm.h"
#include "compile.h"

static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] INPUT.1\n\n", program_name);
    printf("Man Page Compiler - Translate man pages to working CLI utilities via LLMs\n\n");
    printf("Options:\n");
    printf("  -o OUTPUT     Output file (default: program name)\n");
    printf("  -m MODEL      LLM model (default: gemma4)\n");
    printf("  -u URL        Ollama URL (default: http://localhost:11434)\n");
    printf("  -r RETRIES    Max retry attempts (default: 3)\n");
    printf("  -v            Verbose output\n");
    printf("  -d            Debug mode (keep intermediate files)\n");
    printf("  -h            Show this help\n\n");
    printf("Examples:\n");
    printf("  mpc examples/simple.1\n");
    printf("  mpc -o calculator examples/calculator.1\n");
    printf("  mpc -m qwen2.5-coder:14b examples/hello.1\n");
    printf("  mpc -u http://gpu-box:11434 examples/hello.1\n");
}

int main(int argc, char *argv[]) {
    config_t config = {
        .target_language = "c",
        .llm_model = "gemma4",
        .llm_url = "http://localhost:11434",
        .verbose = 0,
        .debug = 0,
        .max_retries = 3
    };
    
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
    

    
    printf("Man Page Compiler v1.0\n");
    printf("Input: %s\n", config.input_file);
    printf("Output: %s\n", config.output_file);
    printf("Target: %s\n", config.target_language);
    printf("Model: %s (%s)\n", config.llm_model, config.llm_url);
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