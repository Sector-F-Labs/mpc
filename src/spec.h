/*
 * Man Page Compiler (MPC) - LLM prompt specification
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#ifndef MPC_SPEC_H
#define MPC_SPEC_H

#include "config.h"
#include "manpage.h"

#define MAX_PROMPT_SIZE (1024 * 1024)  // 1MB for LLM prompt

// Generate structured English specification for LLM
char* generate_specification(const manpage_t *manpage, const config_t *config, const char *error_feedback);

#endif
