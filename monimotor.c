#include "./include/monimotor.h"
#include <sched.h> //sched_setscheduler
#include "tasks.c"

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
void audioRecordingCallback(void* userdata, Uint8* stream, int len )
{	
	/* Copy bytes acquired from audio stream */
	if(choose_buffer == gRecordingBuffer1)
	{
		printf("Callback Buffer 1\n");
		memcpy(&choose_buffer[gBufferBytePosition], stream, len);
		gBufferBytePosition += len;
	}
	else
	{
		printf("Callback Buffer 2\n");
		memcpy(&choose_buffer[gBufferBytePosition2], stream, len);
		gBufferBytePosition2 += len;
	}

	LOCK(&choose_buffer_mutex);
	printf("Callback: Signaling\n");
	if(choose_buffer == gRecordingBuffer1)
	{
		if(gBufferBytePosition >= gBufferByteMaxPosition)
		{
			printf("Callback: Print samplees to Buff1\n");
			for(int i = 0; i < len; i++){
				printf("%u", stream[i]);
				printf(" ---- ");
			}
			choose_buffer = gRecordingBuffer2;
			pthread_cond_signal(&gPreprocessingSignal);
		}
	}
	else
	{
		if(gBufferBytePosition2 >= gBufferByteMaxPosition2)
		{
			printf("Callback: Print samplees to Buff1\n");
			for(int i = 0; i < len; i++){
				printf("%u", stream[i]);
				printf(" ---- ");
			}
			choose_buffer = gRecordingBuffer1;
			pthread_cond_signal(&gPreprocessingSignal);
		}
	}
	//Signal the preprocessing task that there is data to be processed
	UNLOCK(&choose_buffer_mutex);
}

void filterLP(uint32_t cof, uint32_t sampleFreq, uint8_t * buffer, uint32_t nSamples)
{					
	
	int i;
	
	uint16_t * procBuffer; 	/* Temporary buffer */
	uint16_t * origBuffer; 	/* Pointer to original buffer, with right sample type (UINT16 in the case) */
	
	float alfa, beta; 
		
	/* Compute alfa and beta multipliers */
	alfa = (2 * M_PI / sampleFreq * cof ) / ( (2 * M_PI / sampleFreq * cof ) + 1 );
	beta = 1-alfa;
	
	
	/* Get pointer to buffer of the right type */
	origBuffer = (uint16_t *)buffer;
	
	/* allocate temporary buffer and init it */
	procBuffer = (uint16_t *)malloc(nSamples*sizeof(uint16_t));		
	memset(procBuffer,0, nSamples*sizeof(uint16_t));
	        
	/* Apply the filter */		
	for(i = 1; i < nSamples; i++) {				
		procBuffer[i] = alfa * origBuffer[i] + beta * procBuffer[i-1];		
	}
	
	/* Move data to the original (playback) buffer */
	memcpy(buffer, (uint8_t *)procBuffer, nSamples*sizeof(uint16_t));	
	
	/* Release resources */
	free(procBuffer);	
	
	return;
}
//
void genSineU16(uint16_t freq, uint32_t durationMS, uint16_t amp, uint8_t *buffer)
{	
	int i=0, nSamples=0;
		
	float sinArgK = 2*M_PI*freq;				/* Compute once constant part of sin argument, for efficiency */
	
	uint8_t * bufU16 = (uint8_t *)buffer; 	/* UINT16 pointer to buffer for access sample by sample */
	
	nSamples = ((float)durationMS / 1000) * SAMP_FREQ; 	/* Compute how many samples to generate */
			
	/* Generate sine wave */
	for(i = 0; i < nSamples; i++) {
		bufU16[i] = amp/2*(1+sin((sinArgK*i)/SAMP_FREQ));		
	}		
	
	return;
}

void* issues_task_code(void* arg){
	cab_buffer_t *cab_buffer = (cab_buffer_t*)arg;

	//Apply FFT (Convert firstly to complex numbers)
	uint8_t* aux_buffer = cab_buffer_t_read(cab_buffer);
	for(int i = 0; i < sizeof(aux_buffer); i++){
		gIssuesBuffer[i] = (complex double)aux_buffer[i];
	}

	fftCompute(gIssuesBuffer, sizeof(gIssuesBuffer)/sizeof(complex double));

	//Check frequencies below 200hz
	//create fk and Ak buffers
	float* fk = (float*)malloc(sizeof(gSpeedBuffer)/sizeof(complex double));
	float* Ak = (float*)malloc(sizeof(gSpeedBuffer)/sizeof(complex double));
	//Check what frequency has the most amplitude
	fftGetAmplitude(gSpeedBuffer, sizeof(gSpeedBuffer)/sizeof(complex double), SAMP_FREQ, fk, Ak);

	for(int i = 0; i < sizeof(Ak)/sizeof(float); i++){
		if(fk[i] < 200){
			if(Ak[i] > 0.2*gRTDB->highest_amplitude){
				gRTDB->has_bearing_issues = 1;
				break;
			}
		}
	}

	free(fk);
	free(Ak);
}

int main(int argc, char *argv[]){

	cab_buffer = (cab_buffer_t*)malloc(sizeof(cab_buffer_t));
	if(cab_buffer_t_init(cab_buffer, 3, 4096)){
		printf("Failed to initialize CAB buffer\n");
		return 1;
	}
	gRTDB = (rt_db_t*)malloc(sizeof(rt_db_t));
	gRTDB->has_bearing_issues = 0;
	gRTDB->highest_amplitude = 0;
	gRTDB->motor_speed = 0;

	/* ****************
	 *  Variables 
	 **************** */
	SDL_AudioDeviceID recordingDeviceId = 0; 	/* Structure with ID of recording device */
	const char * deviceName;					/* Capture device name */
	int index;									/* Device index used to browse audio devices */
	int bytesPerSample;							/* Number of bytes each sample requires. Function of size of sample and # of channels */ 
	int bytesPerSecond;							/* Intuitive. bytes per sample sample * sampling frequency */


	/* SDL Init */
	if(SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	/* *************************************
	 * Get and open recording device 
	 ************************************* */
	SDL_AudioSpec desiredRecordingSpec;
	/* Defined in SDL_audio.h */
	SDL_zero(desiredRecordingSpec);				/* Init struct with default values */
	desiredRecordingSpec.freq = SAMP_FREQ;		/* Samples per second */
	desiredRecordingSpec.format = FORMAT;		/* Sampling format */
	desiredRecordingSpec.channels = MONO;		/* 1 - mono; 2 stereo */
	desiredRecordingSpec.samples = ABUFSIZE_SAMPLES;		/* Audio buffer size in sample FRAMES (total samples divided by channel count) */
	desiredRecordingSpec.callback = audioRecordingCallback;

	/* Get number of recording devices */
	gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);		/* Argument is "iscapture": 0 to request playback device, !0 for recording device */

	if(gRecordingDeviceCount < 1)
	{
		printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}
	
	/* and lists them */
	for(int i = 0; i < gRecordingDeviceCount; ++i)
	{
		//Get capture device name
		deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);/* Arguments are "index" and "iscapture"*/
		printf("%d - %s\n", i, deviceName);
	}

	/* If device index supplied as arg, use it, otherwise, ask the user */
	if(argc == 2) {
		index = atoi(argv[1]);		
	} else {
		/* allow the user to select the recording device */
		printf("Choose audio\n");
		scanf("%d", &index);
	}

	if(index < 0 || index >= gRecordingDeviceCount) {
		printf( "Invalid device ID. Must be between 0 and %d\n", gRecordingDeviceCount-1 );
		return 0;
	} else {
		printf( "Using audio capture device %d - %s\n", index, deviceName );
	}

	/* and open it */
	recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	
	/* if device failed to open terminate */
	if(recordingDeviceId == 0)
	{
		//Report error
		printf("Failed to open recording device! SDL Error: %s", SDL_GetError() );
		return 1;
	}

	/* **************************************************
	 * Recording and playback devices opened and OK.
	 * Time to init some data structures 
	 * **************************************************/ 
	/* Calculate number of bytes per sample */
	bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	/* Calculate number of bytes per second */
	bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

	/* Calculate buffer size, for the desired duration  */
	gBufferByteSize = 5 * bytesPerSecond;

	/* Calculate max buffer use - some additional space to allow for extra samples*/
	/* Detection of buffer use is made form device-driver callback, so can be a biffer overrun if some */
	/* leeway is not added */ 
	gBufferByteMaxPosition = gBufferByteMaxPosition2 = 5 * bytesPerSecond;

	/* Allocate and initialize recording buffers */
	gRecordingBuffer1 = (uint8_t *)malloc(gBufferByteSize);
	if(gRecordingBuffer1 == NULL)
	{
		printf("Failed to allocate memory for recording buffer 1\n");
		return 1;
	}
	memset(gRecordingBuffer1, 0, gBufferByteSize);

	gRecordingBuffer2 = (uint8_t *)malloc(gBufferByteSize);
	if(gRecordingBuffer2 == NULL)
	{
		printf("Failed to allocate memory for recording buffer 2\n");
		return 1;
	}
	memset(gRecordingBuffer2, 0, gBufferByteSize);

	/* Start choose_buffer*/
	choose_buffer = gRecordingBuffer1;

	printf("\n\r *********** \n\r");
	printf("bytesPerSample=%d, bytesPerSecond=%d, buffer byte size=%d (allocated) buffer byte size=%d (for nominal recording)", \
			bytesPerSample, bytesPerSecond,gBufferByteSize, gBufferByteMaxPosition);
	printf("\n\r *********** \n\r");

	/** Start buffers for processing tasks */
	gSpeedBuffer = (complex double *)malloc(gBufferByteSize * sizeof(complex double));
	if(gSpeedBuffer == NULL)
	{
		printf("Failed to allocate memory for speed buffer\n");
		return 1;
	}
	gIssuesBuffer = (complex double *)malloc(gBufferByteSize * sizeof(complex double));
	if(gIssuesBuffer == NULL)
	{
		printf("Failed to allocate memory for issues buffer\n");
		return 1;
	}

	/** Start Tasks */
	start_preprocessing_task();
	//start_speed_task();

#define RECORD
#ifdef RECORD

	/* ******************************************************
	 * All set. Time to record, process and play sounds  
	 * ******************************************************/
	
	printf("Recording\n");
	
	/* Set index to the beginning of buffer */
	gBufferBytePosition = 0;

	/* After being open devices have callback processing blocked (paused_on active), to allow configuration without glitches */
	/* Devices must be unpaused to allow callback processing */
	/* Wait until recording buffer full */
	SDL_PauseAudioDevice(recordingDeviceId, 0 ); /* Args are SDL device id and pause_on */

	sleep(5);

	SDL_PauseAudioDevice(recordingDeviceId, 0); /* Args are SDL device id and pause_on */	



	/* *****************************************************************
	 * Recorded data obtained. Now process it and play it back
	 * *****************************************************************/
 
#endif

#define GENSINE
#ifdef GENSINE
	printf("\n Generating a sine wave \n");
	genSineU16(1000, 1000, 30000, choose_buffer); 	/* freq, durationMS, amp, buffer */

#endif


}