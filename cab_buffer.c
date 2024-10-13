#include "./include/cab_buffer.h"

cab_handle_t cab_buffer_t_init(size_t nSamples, size_t sampleSize) {
    cab_handle_t buffer = (cab_handle_t)malloc(sizeof(cab_buffer_t));
    if(buffer == NULL){
        return NULL;
    }
    buffer->buffer = (uint8_t*)malloc(nSamples * sampleSize);

    buffer->buffSize = nSamples * sampleSize;
    buffer->sampleSize = sampleSize;

    buffer->head = buffer->buffer;
    buffer->tail = buffer->buffer;
}

void cab_buffer_t_destroy(cab_handle_t buffer){
    free(buffer->buffer);
    free(buffer);
}

void cab_buffer_t_reset(cab_handle_t buffer){
    buffer->head = buffer->buffer;
    buffer->tail = buffer->buffer;
    buffer->currentSize = 0;
}

uint8_t cab_buffer_t_write(cab_handle_t buffer, uint8_t* data){
    pthread_mutex_lock(&buffer->mutex);
    if(is_full(buffer)){
        return NULL;
    }

    memcpy(buffer->head, data, buffer->sampleSize);
    buffer->currentSize++;

    pthread_mutex_unlock(&buffer->mutex);

    return buffer->head = advance_pointer(buffer, buffer->head);
}

uint8_t* cab_buffer_t_read(cab_handle_t buffer){
    pthread_mutex_lock(&buffer->mutex);

    if(is_empty(buffer)){
        return NULL;
    }
    uint8_t* data = buffer->tail;
    buffer->currentSize--;
    buffer->tail=advance_pointer(buffer, buffer->tail);

    pthread_mutex_unlock(&buffer->mutex);
    return data;
}

uint8_t* advance_pointer(cab_handle_t buffer, uint8_t* ptr){
    ptr += buffer->sampleSize;
    if(ptr == buffer->buffer + buffer->buffSize){
        ptr = buffer->buffer;
    }
    return ptr;
}

uint8_t is_empty(cab_handle_t buffer){
    return buffer->currentSize == 0;
}

uint8_t is_full(cab_handle_t buffer){
    return buffer->currentSize == buffer->buffSize;
}
