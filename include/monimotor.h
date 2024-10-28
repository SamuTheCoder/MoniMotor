#pragma once

#define _GNU_SOURCE             /* Must precede #include <sched.h> for sched_setaffinity */ 

#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <complex.h>
#include "../fft/fft.h"
#include "cab_buffer.h"
#include <sys/time.h>
#include <string.h>
#include <sched.h> //sched_setscheduler
#include <pthread.h>
#include <errno.h>
#include <signal.h> // Timers
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <complex.h>
#include <rt_db.h>

#define LOCK_P(mutex)      do if(pthread_mutex_lock(mutex) != 0) { fprintf(stderr,"pthread_mutex_lock() failed\n"); exit(1); } while(0)
#define UNLOCK_P(mutex)    do if(pthread_mutex_unlock(mutex) != 0) { fprintf(stderr,"pthread_mutex_unlock() failed\n"); exit(1); } while(0)
#define SIGNAL_P(cond)  do if(pthread_cond_broadcast(cond)    != 0) { fprintf(stderr,"pthread_cond_broadcast() failed --- producer\n"); exit(1); } while(0)

/* ***********************************************
* App specific defines
* ***********************************************/

#define MONO 1 					/* Sample and play in mono (1 channel) */
#define SAMP_FREQ 44100			/* Sampling frequency used by audio device */
#define FORMAT AUDIO_U16		/* Format of each sample (signed, unsigned, 8,16 bits, int/float, ...) */
#define ABUFSIZE_SAMPLES 4096	/* Audio buffer size in sample FRAMES (total samples divided by channel count) */

#define NS_IN_SEC 1000000000L

#define PERIOD_NS (100*1000*1000) 	// Period (ns component)
#define PERIOD_S (0)				// Period (seconds component)

#define BOOT_ITER 10				// Number of activations for warm-up
									// There is an initial transient in which first activations
									// often have an irregular behaviour (cache issues, ..)
#define DEFAULT_PRIO 50				// Default (fixed) thread priority  
#define THREAD_INIT_OFFSET 1000000	// Initial offset (i.e. delay) of rt thread


clock_t clk_callback = 0;

const int MAX_RECORDING_DEVICES = 10;		/* Maximum allowed number of souns devices that will be detected */

//Maximum recording time
const int MAX_RECORDING_SECONDS = 5;

//Maximum recording time plus padding
const int RECORDING_BUFFER_SECONDS = MAX_RECORDING_SECONDS + 1;

//Recording data buffer (device driver)
Uint8 * gRecordingBuffer1 = NULL;

//Preprocessing data buffer
Uint8 * gRecordingBuffer2 = NULL;

//Shared variable for choosing which buffer to use
Uint8 * choose_buffer = NULL;

//Mutex for choosing buffer
pthread_mutex_t choose_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

//Size of data buffer
Uint32 gBufferByteSize = 0;

//Position in data buffer
Uint32 gBufferBytePosition = 0;

//Position in data buffer 2
Uint32 gBufferBytePosition2 = 0;

//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;

Uint32 gBufferByteMaxPosition2 = 0;

//Size of preprocessing buffer
Uint32 gPreprocessingBufferByteSize = 0;

//Position in preprocessing buffer
Uint32 gPreprocessingBufferBytePosition = 0;

//Maximum position in preprocessing buffer for recording
Uint32 gPreprocessingBufferByteMaxPosition = 0;

//signal from callback to preprocessing task
pthread_cond_t gPreprocessingSignal = PTHREAD_COND_INITIALIZER;

//CAB buffer
cab_buffer_t *cab_buffer;

// Real Time Database
rt_db_t* gRTDB;

uint64_t min_iat, max_iat; // Hold the minimum/maximum observed time between successive executions
uint64_t min_iat_speed, max_iat_speed; // Hold the minimum/maximum observed time between successive executions
uint64_t min_iat_issues, max_iat_issues;

/************************************* 
 * Buffers for issues and speed tasks
 *************************************/

complex double * gSpeedBuffer = NULL;
complex double * gIssuesBuffer = NULL;

//Receieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;

int gRecordingDeviceCount = 0;

/* ***********************************************
* Arguments Structure
* ***********************************************/
// Thread structure
typedef struct
{
    pthread_t tid; // thread identifier
} thread_speed_data_t;

// Thread structure
typedef struct
{
    pthread_t tid; // thread identifier
} thread_issues_data_t;

// Preprocessing thread code
void *preprocessing_task_code(void *arg);

// Speed thread code
void* speed_task_code(void *arg);

// Issues thread code
void* issues_task_code(void *arg);

// RDTB thread code
void *rtdb_task_code(void *arg);


// Aux functions

/* ************************************************************** 
 * Callback issued by the capture device driver. 
 * Args are:
 * 		userdata: optional user data passed on AudioSpec struct
 * 		stream: buffer of samples acquired 
 * 		len: length of buffer ***in bytes*** (not # of samples) 
 * 
 * Just copy the data to the application buffer and update index 
 * **************************************************************/

void audioRecordingCallback(void* userdata, Uint8* stream, int len );

void filterLP(uint32_t cof, uint32_t sampleFreq, uint8_t * buffer, uint32_t nSamples);

/* *************************************************************************************
 * Audio processing example:
 *      Generates a sine wave. 
 *      Frequency in Hz, durationMS in miliseconds, amplitude 0...0xFFFF, stream buffer 
 * 
 * *************************************************************************************/ 
void genSineU16(uint16_t freq, uint32_t durationMS, uint16_t amp, uint8_t *buffer);

/**
 * Function to initiate real time preprocessing task
 */
void start_preprocessing_task();

/**
 * Function to initiate real time speed task
 */
void start_speed_task();

/**
 * Function to initiate real time issues task
 */
void start_issues_task();

/**
 * Function to initiate real time print database task
 */
void start_rtdb_task();

/* *************************************************************************************
 * Debug function 
 *      Returns the max and min amplitude of signal in a buffer - uint16 format
 * 
 * *************************************************************************************/ 
void getMaxMinU16(uint8_t * buffer, uint32_t nSamples, uint32_t * max, uint32_t * min)
{	
	int i=0;
		
	uint16_t * origBuffer; 	/* Pointer to original buffer, with right sample type (UINT16 in the case) */
			
	/* Get pointer to buffer of the right type */
	origBuffer = (uint16_t *)buffer;
	
	/* Get max value */
	*max=origBuffer[0];
	*min=*max;
	for(i = 1; i < nSamples; i++) {		
		if(origBuffer[i] > *max)
			*max=origBuffer[i];
		if(origBuffer[i] < *min)
			*min=origBuffer[i];		
	}
	
	return;	
}

// Adds two timespect variables
struct  timespec  TsAdd(struct  timespec  ts1, struct  timespec  ts2){
	
	struct  timespec  tr;
	
	// Add the two timespec variables
		tr.tv_sec = ts1.tv_sec + ts2.tv_sec ;
		tr.tv_nsec = ts1.tv_nsec + ts2.tv_nsec ;
	// Check for nsec overflow	
	if (tr.tv_nsec >= NS_IN_SEC) {
			tr.tv_sec++ ;
		tr.tv_nsec = tr.tv_nsec - NS_IN_SEC ;
		}

	return (tr) ;
}

// Subtracts two timespect variables
struct  timespec  TsSub (struct  timespec  ts1, struct  timespec  ts2) {
  struct  timespec  tr;

  // Subtract second arg from first one 
  if ((ts1.tv_sec < ts2.tv_sec) || ((ts1.tv_sec == ts2.tv_sec) && (ts1.tv_nsec <= ts2.tv_nsec))) {
	// Result would be negative. Return 0
	tr.tv_sec = tr.tv_nsec = 0 ;  
  } else {						
	// If T1 > T2, proceed 
		tr.tv_sec = ts1.tv_sec - ts2.tv_sec ;
		if (ts1.tv_nsec < ts2.tv_nsec) {
			tr.tv_nsec = ts1.tv_nsec + NS_IN_SEC - ts2.tv_nsec ;
			tr.tv_sec-- ;				
		} else {
			tr.tv_nsec = ts1.tv_nsec - ts2.tv_nsec ;
		}
	}

	return (tr) ;

}

/* ***********************************************
 * Debug function: 
 *       Prints the buffer contents - 8 bit samples * 
 * **********************************************/
void printSamplesU8(uint8_t * buffer, int size) {
	int i=0;
	
	printf("\n\r Samples: \n\r");
	for(i = 0; i < size; i++) {
		printf("%3u ",buffer[i]);
		if((i%20) == 0)
			printf("\n\r");
	}		
}

/* ***********************************************
 * Debug function: 
 *       Prints the buffer contents - uint16 samples * 
 * **********************************************/
void printSamplesU16(uint8_t * buffer, int nsamples) {
	int i=0;
	uint16_t * bufu16 = (uint16_t *)buffer;
	 
	printf("\n\r Samples: \n\r");
	for(i = 0; i < nsamples; i++) {
		printf("%5u ",bufu16[i]);
		if((i%20) == 0)
			printf("\n\r");
	}		
}
