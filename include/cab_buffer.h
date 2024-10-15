#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifndef CAB_BUFFER_H
#define CAB_BUFFER_H

typedef struct {
    uint8_t* ptr;
    size_t size; //size of ptr in bytes
    uint8_t* current_ptr;
    uint8_t link_count; //number of tasks using the buffer  
} buffer;

typedef struct {
    buffer* buffer_array;
    int most_recent;
} cab_buffer_t;

typedef cab_buffer_t* cab_handle_t;

void cab_buffer_t_init(cab_buffer_t* cab_buffer, uint8_t n_buffers, size_t buffer_size);

void cab_buffer_t_destroy(cab_buffer_t* cab_buff);

void cab_buffer_t_read(cab_buffer_t* cab_buff);

void cab_buffer_t_write(cab_buffer_t* cab_buff);

//falta a quinta

#endif 
