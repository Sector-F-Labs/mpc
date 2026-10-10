/*
 * Man Page Compiler (MPC) - man page parsing
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "manpage.h"

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
    
    printf("Parsed man page: %s (%d sections, %d options, %d examples)\n", 
           manpage->name ? manpage->name : "unknown",
           manpage->section_count, manpage->option_count, manpage->example_count);
    
    return 0;
}
