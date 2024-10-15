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

}

void cab_buffer_t_reset(cab_handle_t ptr){
    ptr->head = ptr->ptr;
    ptr->tail = ptr->ptr;
    ptr->currentSize = 0;
}

uint8_t cab_buffer_t_write(cab_buffer_t ptr){
    pthread_mutex_lock(&ptr->mutex);

    memcpy(ptr->head, data, ptr->sampleSize);
    ptr->currentSize++;

    pthread_mutex_unlock(&ptr->mutex);

    return ptr->head = advance_pointer(ptr, ptr->head);
}

uint8_t* cab_buffer_t_read(cab_buffer_t ptr){
    pthread_mutex_lock(&ptr->mutex);

    if(is_empty(ptr)){
        return NULL;
    }
    uint8_t* data = ptr->tail;
    ptr->currentSize--;
    ptr->tail=advance_pointer(ptr, ptr->tail);

    pthread_mutex_unlock(&ptr->mutex);
    return data;
}

uint8_t* advance_pointer(cab_handle_t ptr, uint8_t* ptr){
    ptr += ptr->sampleSize;
    if(ptr == ptr->ptr + ptr->buffSize){
        ptr = ptr->ptr;
    }
    return ptr;
}


