#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifndef CAB_BUFFER_H
#define CAB_BUFFER_H

typedef struct {
    uint8_t* buffer;
    size_t buffSize;
    size_t sampleSize;
    uint8_t* head;
    uint8_t* tail;
    size_t currentSize;
    pthread_mutex_t mutex;
} cab_buffer_t;

typedef cab_buffer_t* cab_handle_t;

cab_handle_t cab_buffer_t_init(size_t nSamples, size_t sampleSize);

void cab_buffer_t_destroy(cab_handle_t buffer);

void cab_buffer_t_reset(cab_handle_t buffer);

uint8_t cab_buffer_t_write(cab_handle_t buffer, uint8_t* data);

uint8_t* cab_buffer_t_read(cab_handle_t buffer);

uint8_t* advance_pointer(cab_handle_t buffer, uint8_t* ptr);

uint8_t is_empty(cab_handle_t buffer);

uint8_t is_full(cab_handle_t buffer);
#endif 
