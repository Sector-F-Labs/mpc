/*
 * Man Page Compiler (MPC) - LLM prompt specification
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec.h"

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
