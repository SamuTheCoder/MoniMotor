/** DOXYGEN DOCS
* @file cab_buffer.h
* @brief This file contains the declarations for a CAB type buffer. Widely used in real time systems.
*/

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifndef CAB_BUFFER_H
#define CAB_BUFFER_H


/**
 * @struct buffer
 * @brief Struct that represents a buffer
 */
typedef struct {
    uint8_t* ptr;
    size_t size; //size of ptr in bytes
    uint8_t* current_ptr;
    uint8_t link_count; //number of tasks using the buffer  
} buffer;

/**
 * @struct cab_buffer_t
 * @brief Struct that represents a CAB buffer
 */
typedef struct {
    buffer* buffer_array;
    int most_recent; //most recent buffer written
} cab_buffer_t;



/**
 * @brief Initializes a CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 * @param n_buffers Number of buffers
 * @param buffer_size Size of each buffer
 */
void cab_buffer_t_init(cab_buffer_t* cab_buffer, uint8_t n_buffers, size_t buffer_size);

/**
 * @brief Destroys a CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 */
void cab_buffer_t_destroy(cab_buffer_t* cab_buff);

/**
 * @brief Reads from a CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 * @return Pointer to the buffer read
 */
uint8_t* cab_buffer_t_read(cab_buffer_t* cab_buff);

/**
 * @brief Writes to a CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 * @param stream Pointer to the stream to be written
 * @return 1 if the write was successful, 0 otherwise
 */
uint8_t cab_buffer_t_write(cab_buffer_t* cab_buff, uint8_t* stream);

//falta a quinta

#endif 
