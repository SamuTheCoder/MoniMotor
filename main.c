#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cab_buffer.h"

int main() {
    // Create and initialize a cab_buffer_t
    cab_buffer_t* cab_buffer = (cab_buffer_t*)malloc(sizeof(cab_buffer_t));
    if (cab_buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for cab_buffer\n");
        return 1;
    }

    uint8_t n_buffers = 3;
    size_t buffer_size = 10;
    cab_buffer_t_init(cab_buffer, n_buffers, buffer_size);

    // Test writing to the cab_buffer
    uint8_t stream1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t stream2[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    uint8_t stream3[10] = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29};

    cab_buffer_t_write(cab_buffer, stream1);
    cab_buffer_t_write(cab_buffer, stream2);
    cab_buffer_t_write(cab_buffer, stream3);

    // Test reading from the cab_buffer
    uint8_t* read_buffer = cab_buffer_t_read(cab_buffer);
    if (read_buffer != NULL) {
        printf("Read buffer: ");
        for (size_t i = 0; i < buffer_size; i++) {
            printf("%d ", read_buffer[i]);
        }
        printf("\n");
    } else {
        printf("Failed to read buffer\n");
    }

    // Test printing the buffers
    for (uint8_t i = 0; i < n_buffers; i++) {
        cab_buffer_t_print(cab_buffer, i);
    }

    // Destroy the cab_buffer
    cab_buffer_t_destroy(cab_buffer);

    return 0;
}
