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
    uint8_t* ptr; // pointer to buffer
    size_t size; //size of ptr in bytes
    uint8_t* current_ptr; // buffer write pointer
    uint8_t link_count; //number of tasks using the buffer
    pthread_mutex_t mutex; // Mutex to read and write the link_count safely  
} buffer;

/**
 * @struct cab_buffer_t
 * @brief Struct that represents a CAB buffer
 */
typedef struct {
    buffer* buffer_array; // ptr to array of buffers
    uint8_t n_buffers; // number of buffer
    int most_recent; //most recent buffer written
} cab_buffer_t;



/**
 * @brief Initializes a CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 * @param n_buffers Number of buffers
 * @param buffer_size Size of each buffer
 * @return 0 if the init was successful
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
 * @return 0 if the write was successful
 */
uint8_t cab_buffer_t_write(cab_buffer_t* cab_buff, uint8_t* stream);


/**
 * @brief Prints a buffer from CAB buffer
 * @param cab_buffer Pointer to the CAB buffer
 * @param buffer_number Number of buffer from the CAB buffer array to print 
 * @return 0 if the print was successful
 */
uint8_t cab_buffer_t_print(cab_buffer_t* cab_buff, uint8_t buffer_number);


#endif 
