#include "./include/cab_buffer.h"

//Before using init, you must create a cab_buffer_t pointer and use malloc
//Ex: cab_buffer_t* cab_buffer = (cab_buffer_t*)malloc(sizeof(cab_buffer_t));
//Only after you must use init
void cab_buffer_t_init(cab_buffer_t* cab_buffer, uint8_t n_buffers, size_t buffer_size) {
    cab_buffer->most_recent = 0;

    cab_buffer->buffer_array = (buffer*)malloc(n_buffers * sizeof(buffer));
    if(cab_buffer->buffer_array == NULL)
    {
        return NULL;
    }

    for(int i = 0; i < n_buffers; i++)
    {
        buffer* buffer = &cab_buffer->buffer_array[i];
        buffer->ptr = (uint8_t*)malloc(buffer_size);
        if(buffer->ptr == NULL)
        {
            return NULL;
        }
        buffer->size = buffer_size;

        buffer->current_ptr = buffer->ptr;
        buffer->link_count = 0;
    }
}

void cab_buffer_t_destroy(cab_buffer_t* cab_buffer){
    if (cab_buffer->buffer_array)
    {
        for(int i = 0; i < sizeof(cab_buffer->buffer_array)/sizeof(buffer); i++)
        {
            if(cab_buffer->buffer_array[i].ptr)
            {
                free(cab_buffer->buffer_array[i].ptr);
            }
        }
        free(cab_buffer->buffer_array);
    }
    free(cab_buffer);
}

uint8_t cab_buffer_t_write(cab_buffer_t* ptr, uint8_t* stream){
    // For writing, preprocessing task must look for the link_count of the buffer
    // To check if the buffer is being used by other tasks
    // And also never write on the current most recent buffer

    for(int i = 0; i <= sizeof(ptr->buffer_array)/sizeof(buffer); i++){
        if(ptr->buffer_array[i].link_count == 0 && ptr->most_recent != i){
            // Write to buffer
            memcpy(ptr->buffer_array[i].ptr, stream, ptr->buffer_array[i].size);
            ptr->most_recent = i;
            return 1;
        }
    }
    return 0;
}

uint8_t* cab_buffer_t_read(cab_buffer_t* ptr){
    // For reading, tasks must look for the most recently written buffer
    // Doesn't matter if the buffer is in use or not

    for(int i = 0; i < sizeof(ptr->buffer_array)/sizeof(buffer); i++)
    {
        if(i == ptr->most_recent)
        {
            ptr->buffer_array[i].link_count++;
            return ptr->buffer_array[i].ptr;
        }
    }
    return NULL;
}



