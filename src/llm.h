/*
 * Man Page Compiler (MPC) - Ollama LLM backend
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#ifndef MPC_LLM_H
#define MPC_LLM_H

#include "config.h"

// Call LLM API to generate code
char* call_llm(const char *prompt, const config_t *config);

// Extract C code from LLM response (handles markdown code blocks)
char* extract_code(const char *response);

#endif
