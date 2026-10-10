/*
 * Man Page Compiler (MPC) - runtime configuration
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#ifndef MPC_CONFIG_H
#define MPC_CONFIG_H

typedef struct {
    char *input_file;
    char *output_file;
    char *target_language;

    char *llm_model;
    char *llm_url;
    int verbose;
    int debug;
    int max_retries;
} config_t;

#endif
