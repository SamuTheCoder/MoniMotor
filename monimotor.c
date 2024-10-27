#include "./include/monimotor.h"
#include <sched.h> //sched_setscheduler

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
		choose_buffer = gRecordingBuffer2;
	}
	else
	{
		choose_buffer = gRecordingBuffer1;
	}
	//Signal the preprocessing task that there is data to be processed
	pthread_cond_signal(&gPreprocessingSignal);
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

void* preprocessing_task_code(void* arg){
	/* Timespec variables to manage time */
	struct timespec ts, // thread next activation time (absolute)
			ta, 		// activation time of current thread activation (absolute)
			tit, 		// thread time from last execution,
			ta_ant, 	// activation time of last instance (absolute),
			tp; 		// Thread period

	int niter = 0;
	int update;
	
	usleep(1000000);

	tp.tv_nsec = 125 * 1000 * 1000;
	tp.tv_sec = 0;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts = TsAdd(ts, tp);

	while(1){
		/* Wait until next cycle */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,&ts,NULL);
		clock_gettime(CLOCK_MONOTONIC, &ta);		
		ts = TsAdd(ts,tp);		
		
		niter++; // Coount number of activations
		
		/* Compute latency and jitter */
		if( niter == 1) 
			ta_ant = ta; // Init ta_ant at first activation
			
			tit=TsSub(ta,ta_ant);  // Compute time since last activation
		
		if( niter == BOOT_ITER) {	// Boot time finsihed. Init max/min variables	    
			  min_iat = tit.tv_nsec;
			  max_iat = tit.tv_nsec;
			  update = 1;
		}else
		if( niter > BOOT_ITER){ 	// Update max/min, if boot time elapsed 	    
			if(tit.tv_nsec < min_iat) {
			  min_iat = tit.tv_nsec;
			  update = 1;
			}
			if(tit.tv_nsec > max_iat) {
			  max_iat = tit.tv_nsec;
			  update = 1;
			}
		}
		ta_ant = ta; // Update ta_ant

		/* Print maximum/minimum mount of time between successive executions */
		if(update) {		  
		  printf("Preprocessing Task: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r", (float)min_iat/1000, (float)max_iat/1000);
		  update = 0;
		}
		//Wait for signal from audioRecordingCallback
		printf("Task: Waiting for signal\n");
		LOCK(&choose_buffer_mutex);
		pthread_cond_wait(&gPreprocessingSignal, &choose_buffer_mutex);

		//Appply LP filter and store on CAB buffer
		//if choose_buffer is 1, write to 2, if its 2 write to 1
		if(choose_buffer == gRecordingBuffer1){
			filterLP(1000, SAMP_FREQ, gRecordingBuffer2, gBufferByteMaxPosition/sizeof(uint16_t));
			cab_buffer_t_write(cab_buffer, gRecordingBuffer2);
			printf("Task: Written to CAB Buff 2\n");
		}
		else{
			filterLP(1000, SAMP_FREQ, gRecordingBuffer1, gBufferByteMaxPosition/sizeof(uint16_t));
			cab_buffer_t_write(cab_buffer, gRecordingBuffer1);
			printf("Task: Written to CAB Buff 1\n");
		}

		UNLOCK(&choose_buffer_mutex);
	}
}

void* speed_task_code(void* arg){
	cab_buffer_t *cab_buffer = (cab_buffer_t*)arg;
	//Convert data to complex numbers
	//For each sample on cab
	//convert to complex and store on gSpeedBuffer
	uint8_t* aux_buffer = cab_buffer_t_read(cab_buffer);
	for(int i = 0; i < sizeof(aux_buffer); i++){
		gSpeedBuffer[i] = (complex double)aux_buffer[i];
	}

	//Convert from time domain to frequency domain (FFT)
	fftCompute(gSpeedBuffer, sizeof(gSpeedBuffer)/sizeof(complex double));

	//create fk and Ak buffers
	float* fk = (float*)malloc(sizeof(gSpeedBuffer)/sizeof(complex double));
	float* Ak = (float*)malloc(sizeof(gSpeedBuffer)/sizeof(complex double));
	//Check what frequency has the most amplitude
	fftGetAmplitude(gSpeedBuffer, sizeof(gSpeedBuffer)/sizeof(complex double), SAMP_FREQ, fk, Ak);

	//Check for highest value of Ak
	//Must be frequencies between 2kHz and 5kHz
	float max_amplitude = Ak[0];
	float equivalent_frequency = fk[0];
	for(int i = 1; i < sizeof(Ak)/sizeof(float); i++){
		if(Ak[i] > max_amplitude && fk[i] > 2000 && fk[i] < 5000){
			max_amplitude = Ak[i];
			equivalent_frequency = fk[i];
		}
	}

	//Return values to RTDB
	gRTDB->motor_speed = equivalent_frequency;
	gRTDB->highest_amplitude = max_amplitude;

	free(fk);
	free(Ak);
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

void start_tasks(){
	//Start real time threads
	// For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm; 
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity
	
	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

	int priority = 99;
	int periodicity = 125;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	int err=pthread_create(&threadid, &attr, preprocessing_task_code, NULL);
	if(err != 0)
		printf("\n\r Error creating Thread [%s]", strerror(err));

}

int main(int argc, char *argv[]){

	cab_buffer = (cab_buffer_t*)malloc(sizeof(cab_buffer_t));
	cab_buffer_t_init(cab_buffer, 3, 4096);
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
	gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;

	/* Calculate max buffer use - some additional space to allow for extra samples*/
	/* Detection of buffer use is made form device-driver callback, so can be a biffer overrun if some */
	/* leeway is not added */ 
	gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;

	/* Allocate and initialize recording buffers */
	gRecordingBuffer1 = (uint8_t *)malloc(gBufferByteSize);
	memset(gRecordingBuffer1, 0, gBufferByteSize);

	gRecordingBuffer2 = (uint8_t *)malloc(gBufferByteSize);
	memset(gRecordingBuffer2, 0, gBufferByteSize);

	/* Start choose_buffer*/
	choose_buffer = gRecordingBuffer1;

	printf("\n\r *********** \n\r");
	printf("bytesPerSample=%d, bytesPerSecond=%d, buffer byte size=%d (allocated) buffer byte size=%d (for nominal recording)", \
			bytesPerSample, bytesPerSecond,gBufferByteSize, gBufferByteMaxPosition);
	printf("\n\r *********** \n\r");

	/** Start buffers for processing tasks */
	gSpeedBuffer = (complex double *)malloc(gBufferByteSize*8);
	gIssuesBuffer = (complex double *)malloc(gBufferByteSize*8);

	/** Start Tasks */
	start_tasks();

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

//#define GENSINE
//#ifdef GENSINE
//	printf("\n Generating a sine wave \n");
//	genSineU16(1000, 1000, 30000, gRecordingBuffer1); 	/* freq, durationMS, amp, buffer */

//#endif
	//Print contents on the most recent buffer of CAB
	for(int i = 0; i < cab_buffer->buffer_array[cab_buffer->most_recent].size; i+=2){
		uint16_t* sample = (uint16_t* )cab_buffer->buffer_array[cab_buffer->most_recent].ptr + i;
		printf("%d ", *sample);
		printf("-------");
	}

}
