/*
 * Man Page Compiler (MPC) - compiling generated code
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

#include "compile.h"

int compile_code(const char *code, const config_t *config, char **error_output) {
    char temp_file[256];
    char cmd[512];
    FILE *fp;
    
    // Create build directory
    mkdir("build", 0755);
    
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
