/*
 * Man Page Compiler (MPC) - man page parsing
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#ifndef MPC_MANPAGE_H
#define MPC_MANPAGE_H

#define MAX_LINE_LENGTH 4096
#define MAX_SECTIONS 32
#define MAX_OPTIONS 64
#define MAX_EXAMPLES 32

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

// Parse man page into structured format
int parse_manpage(const char *filename, manpage_t *manpage);

#endif
