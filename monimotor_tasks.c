#include "include/monimotor_tasks.h"

void* sound_capture_task(void* args){
    while(1){
        // Timespec variables??

        // Capture sound (audioRecordingCallback)

        // Store sound in buffer (cab_buffer_t_write)

        //Periodic sleep
    }
}

void* preprocessing_task(void* args){
    while(1){
        // Timespec variables??

        // Read sound from buffer (cab_buffer_t_read)

        // Preprocess sound (fft)

        // Store preprocessed sound in buffer (cab_buffer_t_write on another buffer)

        // Periodic sleep (if needed)
    }
}