#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint8_t* buffer;
    size_t buffSize;
    size_t sampleSize;
    uint8_t* head;
    uint8_t* tail;
} cab_buffer_t;

typedef cab_buffer_t* cab_handle_t;

cab_handle_t cab_buffer_t_create(size_t nSamples, size_t sampleSize);

void cab_buffer_t_destroy(cab_handle_t buffer);

void cab_buffer_t_reset(cab_handle_t buffer);

void cab_buffer_t_push(cab_handle_t buffer, uint8_t* data);

void cab_buffer_t_pop(cab_handle_t buffer);

uint8_t* advance_pointer(cab_handle_t buffer, uint8_t* ptr);

uint8_t is_empty(cab_handle_t buffer);
