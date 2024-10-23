#include "./include/cab_buffer.h"

//Before using init, you must create a cab_buffer_t pointer and use malloc
//Ex: cab_buffer_t* cab_buffer = (cab_buffer_t*)malloc(sizeof(cab_buffer_t));
//Only after you must use init
uint8_t cab_buffer_t_init(cab_buffer_t* cab_buffer, uint8_t n_buffers, size_t buffer_size) {
    if(cab_buffer == NULL)
    {
        fprintf(stderr, "cab_buffer is NULL\n");
        return 1;
    }
    cab_buffer->most_recent = -1; // Every buffer is free to use
    cab_buffer->n_buffers = n_buffers;

    cab_buffer->buffer_array = (buffer*)malloc(n_buffers * sizeof(buffer));
    if(cab_buffer->buffer_array == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for buffer array\n");
        return 1; 
    }

    for(int i = 0; i < n_buffers; i++)
    {
        buffer* buf = &cab_buffer->buffer_array[i];
        buf->ptr = (uint8_t*)malloc(buffer_size);
        if(buf->ptr == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for buffer %d\n", i);            
            return 1;
        }
        buf->size = buffer_size;
        buf->current_ptr = buf->ptr;
        buf->link_count = 0;

        // Initialize the mutex for each buffer
        if (pthread_mutex_init(&buf->mutex, NULL) != 0) {
            fprintf(stderr, "Failed to initialize mutex for buffer %d\n", i);
            return 1;
        }
    }

    return 0;
}


uint8_t cab_buffer_t_destroy(cab_buffer_t* cab_buffer){
    if (cab_buffer->buffer_array)
    {
        for(int i = 0; i < cab_buffer->n_buffers; i++) // sizeof(cab_buffer->buffer_array)/sizeof(buffer)
        {
            if(cab_buffer->buffer_array[i].ptr)
            {
                free(cab_buffer->buffer_array[i].ptr);
            }
            pthread_mutex_destroy(&cab_buffer->buffer_array[i].mutex);
        }
        free(cab_buffer->buffer_array);
    }
    free(cab_buffer);
}


uint8_t cab_buffer_t_write(cab_buffer_t* cab_buffer, uint8_t* stream){
    // For writing, preprocessing task must look for the link_count of the buffer
    // To check if the buffer is being used by other tasks
    // And also never write on the current most recent buffer

    for(int i = 0; i < cab_buffer->n_buffers; i++) // sizeof(cab_buffer->buffer_array)/sizeof(buffer) 
    {
        if(cab_buffer->buffer_array[i].link_count == 0 && cab_buffer->most_recent != i) // Nobody is using and not the most recent
        {
            // Write to buffer
            memcpy(cab_buffer->buffer_array[i].ptr, stream, cab_buffer->buffer_array[i].size);
            cab_buffer->most_recent = i;
            return 0;
        }
    }
    return 1; // No buffer found
}


uint8_t* cab_buffer_t_read(cab_buffer_t* cab_buffer){
    // For reading, tasks must look for the most recently written buffer
    // Doesn't matter if the buffer is in use or not

    if (cab_buffer->most_recent >= 0 && cab_buffer->most_recent < cab_buffer->n_buffers) 
    {
        LOCK(&cab_buffer->buffer_array[cab_buffer->most_recent].mutex);
        cab_buffer->buffer_array[cab_buffer->most_recent].link_count++; // Increase number of users
        UNLOCK(&cab_buffer->buffer_array[cab_buffer->most_recent].mutex);

        return cab_buffer->buffer_array[cab_buffer->most_recent].ptr;
    }
    return NULL; // No buffer found
}


uint8_t cab_buffer_t_release(cab_buffer_t* cab_buffer, uint8_t* buffer_ptr) {
    for (int i = 0; i < cab_buffer->n_buffers; i++) {
        buffer* buf = &cab_buffer->buffer_array[i];
        if (buf->ptr == buffer_ptr) {  
            LOCK(&buf->mutex);  // Lock the buffer to modify link_count safely
            if (buf->link_count > 0) {
                buf->link_count--;  // Decrease the link count
            } else {
                fprintf(stderr, "Buffer link_count is already 0, nothing to release\n");
                UNLOCK(&buf->mutex);
                return 1;  // Error: trying to release a buffer that's not in use
            }
            UNLOCK(&buf->mutex);
            return 0;  // Successfully released
        }
    }
    fprintf(stderr, "Invalid buffer pointer passed to release function\n");
    return 1;  // Error: buffer pointer not found in cab_buffer
}


uint8_t cab_buffer_t_print(cab_buffer_t* cab_buff, uint8_t buffer_number) {
    if (buffer_number >= cab_buff->n_buffers) {
        fprintf(stderr, "Invalid buffer number\n");
        return 1;
    }

    buffer* buf = &cab_buff->buffer_array[buffer_number];
    printf("Buffer %d:\n", buffer_number);
    printf("Size: %zu\n", buf->size);
    printf("Link count: %d\n", buf->link_count);
    printf("Data: ");
    for (size_t i = 0; i < buf->size; i++) {
        printf("%d ", buf->ptr[i]);
    }
    printf("\n");

    return 0;
}



