/*
 * Man Page Compiler (MPC) - compiling generated code
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#ifndef MPC_COMPILE_H
#define MPC_COMPILE_H

#include "config.h"

// Compile generated C code to executable
int compile_code(const char *code, const config_t *config, char **error_output);

#endif
