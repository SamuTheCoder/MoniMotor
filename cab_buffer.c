#include "./include/cab_buffer.h"

cab_handle_t cab_buffer_t_create(size_t nSamples, size_t sampleSize) {
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
}

void cab_buffer_t_push(cab_handle_t buffer, uint8_t* data){
    memcpy(buffer->head, data, buffer->sampleSize);

    buffer->head = advance_pointer(buffer, buffer->head);

    if(is_empty(buffer)){
        buffer->tail = advance_pointer(buffer, buffer->tail);
    }
}

void cab_buffer_t_pop(cab_handle_t buffer){
    if(is_empty(buffer)){
        printf("CAB buffer is empty\n");
        return;
    }
    buffer->tail=advance_pointer(buffer, buffer->tail);
}

uint8_t* advance_pointer(cab_handle_t buffer, uint8_t* ptr){
    ptr += buffer->sampleSize;
    if(ptr == buffer->buffer + buffer->buffSize){
        ptr = buffer->buffer;
    }
    return ptr;
}

uint8_t is_empty(cab_handle_t buffer){
    return buffer->head == buffer->tail;
}