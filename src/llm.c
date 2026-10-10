/*
 * Man Page Compiler (MPC) - Ollama LLM backend
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2025, Divan Visagie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#include "llm.h"

typedef struct {
    char *data;
    size_t size;
} http_response_t;

// HTTP response callback for curl
static size_t write_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
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
    
    json_object_set_new(json, "model", json_string(config->llm_model));
    json_object_set_new(json, "prompt", json_string(prompt));
    json_object_set_new(json, "stream", json_false());
    
    json_payload = json_dumps(json, 0);
    
    snprintf(url, sizeof(url), "%s/api/generate", config->llm_url);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    printf("Generating code via %s...\n", config->llm_model);
    
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
    
    json_t *response_text = json_object_get(response_json, "response");
    if (response_text && json_is_string(response_text)) {
        generated_code = strdup(json_string_value(response_text));
    }
    
    json_decref(response_json);
    free(response.data);
    
    if (generated_code) {
        printf("Generated %zu bytes of code\n", strlen(generated_code));
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
