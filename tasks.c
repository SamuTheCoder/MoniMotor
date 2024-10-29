#include "include/monimotor.h"

void* preprocessing_task_code(void* arg){
    printf("Preprocessing Task: Started\n");
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
			//printf("Preprocessing Task: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r", (float)min_iat/1000, (float)max_iat/1000);
			update = 0;
		}

		//Wait for signal from audioRecordingCallback
		LOCK(&choose_buffer_mutex);
		pthread_cond_wait(&gPreprocessingSignal, &choose_buffer_mutex);

		//Appply LP filter and store on CAB buffer
		//if choose_buffer is 1, write to 2, if its 2 write to 1
		if(choose_buffer == gRecordingBuffer1){
			filterLP(1000, SAMP_FREQ, gRecordingBuffer2, gBufferBytePosition2/sizeof(uint16_t));

			if(cab_buffer_t_write(cab_buffer, gRecordingBuffer2)){
                printf("Preprocessing Task: Couldn't write to CAB\n");
                continue;
            }
		}
		else{
			filterLP(1000, SAMP_FREQ, gRecordingBuffer1, gBufferBytePosition/sizeof(uint16_t));
			if(cab_buffer_t_write(cab_buffer, gRecordingBuffer1)){
                printf("Preprocessing Task: Couldn't write to CAB\n");
                continue;
            }
		}

		UNLOCK(&choose_buffer_mutex);
	}
}

void start_preprocessing_task(){
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
		printf("\n\r Error creating Preprocessing Thread [%s]", strerror(err));

}

void* speed_task_code(void* arg){
    printf("Speed Task: Started\n");
    /* Timespec variables to manage time */
	struct timespec ts, // thread next activation time (absolute)
			ta, 		// activation time of current thread activation (absolute)
			tit, 		// thread time from last execution,
			ta_ant, 	// activation time of last instance (absolute),
			tp; 		// Thread period

	int niter = 0;
	int update;
	
	usleep(1000000);

	tp.tv_nsec = 500 * 1000 * 1000;
	tp.tv_sec = 0;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts = TsAdd(ts, tp);

    float* fk = (float*)malloc(gBufferByteSize * sizeof(float));
    float* Ak = (float*)malloc(gBufferByteSize * sizeof(float));

	uint16_t* aux_buffer;

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
			  min_iat_speed = tit.tv_nsec;
			  max_iat_speed = tit.tv_nsec;
			  update = 1;
		}else
		if( niter > BOOT_ITER){ 	// Update max/min, if boot time elapsed 	    
			if(tit.tv_nsec < min_iat_speed) {
			  min_iat_speed = tit.tv_nsec;
			  update = 1;
			}
			if(tit.tv_nsec > max_iat_speed) {
			  max_iat_speed = tit.tv_nsec;
			  update = 1;
			}
		}
		ta_ant = ta; // Update ta_ant

		/* Print maximum/minimum mount of time between successive executions */
		if(update) {		  
			printf("Speed Task: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r", (float)min_iat_speed/1000, (float)max_iat_speed/1000);
			update = 0;
		}
        cab_buffer_t *cab_buffer = (cab_buffer_t*)arg;

        //Convert data to complex numbers
        //For each sample on cab
        //convert to complex and store on gSpeedBuffer
        aux_buffer = (uint16_t *) cab_buffer_t_read(cab_buffer);

		uint16_t number_samples = gBufferByteSize / 2;
		
        if(aux_buffer == NULL){
            printf("Speed Task: CAB buffer is still empty\n");
            continue;
        }

        for(int i = 0; i < number_samples; i++){
            gSpeedBuffer[i] = (complex double)aux_buffer[i];
        }


        //Convert from time domain to frequency domain (FFT)
        fftCompute(gSpeedBuffer, number_samples);
        
        //Check what frequency has the most amplitude
		if(gSpeedBuffer == NULL)
            printf("Speed Task: Speed buff is null\n");
        
        fftGetAmplitude(gSpeedBuffer, number_samples, SAMP_FREQ, fk, Ak);


		//for(int i = 0; i<=number_samples / 2; i++) { // Floats 
			//printf("Amplitude at frequency %f Hz is %f \n", fk[i], Ak[i]);
		//}

		
        // Check for highest value of Ak
        // Must be frequencies between 2kHz and 5kHz
        float max_amplitude = 0;
        float equivalent_frequency = 0;
        for(int i = 1; i < number_samples; i++) {
            if(Ak[i] > max_amplitude && fk[i] >= 2000 && fk[i] <= 5000){
				//printf("Speed Task: Found a higher amplitude: %f\n", Ak[i]);
                max_amplitude = Ak[i];
                equivalent_frequency = fk[i];
            }
        }

        //Return values to RTDB
        gRTDB->motor_speed = equivalent_frequency;
        gRTDB->highest_amplitude = max_amplitude;

        //printf("Speed Task: motor speed: %d, highest amplitude: %f\n", gRTDB->motor_speed, gRTDB->highest_amplitude);
		
	}
    free(fk);
    free(Ak);
}

void start_speed_task(){
    //Start real time threads
	// For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm; 
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity
	
	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

	int priority = 50;
	int periodicity = 500;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	int err=pthread_create(&threadid, &attr, speed_task_code, (void*)cab_buffer);
	if(err != 0)
		printf("\n\r Error creating start speed task Thread [%s]", strerror(err));
}

void *issues_task_code(void *arg)
{
	printf("Issues Task: Started\n");
    /* Timespec variables to manage time */
	struct timespec ts, // thread next activation time (absolute)
			ta, 		// activation time of current thread activation (absolute)
			tit, 		// thread time from last execution,
			ta_ant, 	// activation time of last instance (absolute),
			tp; 		// Thread period

	int niter = 0;
	int update;
	
	usleep(1000000);

	tp.tv_nsec = 500 * 1000 * 1000;
	tp.tv_sec = 0;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts = TsAdd(ts, tp);

    float* fk = (float*)malloc(gBufferByteSize * sizeof(float));
    float* Ak = (float*)malloc(gBufferByteSize * sizeof(float));

	uint16_t* aux_buffer;

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
			  min_iat_issues = tit.tv_nsec;
			  max_iat_issues = tit.tv_nsec;
			  update = 1;
		}else
		if( niter > BOOT_ITER){ 	// Update max/min, if boot time elapsed 	    
			if(tit.tv_nsec < min_iat_issues) {
			  min_iat_issues = tit.tv_nsec;
			  update = 1;
			}
			if(tit.tv_nsec > max_iat_issues) {
			  max_iat_issues = tit.tv_nsec;
			  update = 1;
			}
		}
		ta_ant = ta; // Update ta_ant

		if(update) {		  
			printf("Issues Task: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r", (float)min_iat_issues/1000, (float)max_iat_issues/1000);
			update = 0;
		}
		cab_buffer_t *cab_buffer = (cab_buffer_t *)arg;

		// Apply FFT (Convert firstly to complex numbers)
		uint16_t *aux_buffer = (uint16_t *) cab_buffer_t_read(cab_buffer);
		if(aux_buffer == NULL){
            printf("Issues Task: CAB buffer is still empty\n");
            continue;
        }

		uint16_t samples_number = gBufferByteSize / 2;

		for (int i = 0; i < samples_number; i++)
		{
			gIssuesBuffer[i] = (complex double)aux_buffer[i];
		}

		fftCompute(gIssuesBuffer, samples_number);

		// Check frequencies below 200hz
		// Check what frequency has the most amplitude
		fftGetAmplitude(gSpeedBuffer, samples_number, SAMP_FREQ, fk, Ak);
		gRTDB->has_bearing_issues = 0;
		for (int i = 1; i < samples_number; i++){
			if (fk[i] < 200) {
				if (Ak[i] > 0.2 * gRTDB->highest_amplitude) {
					gRTDB->has_bearing_issues = 1;
					printf("Max Amplitude: %f", gRTDB->highest_amplitude);
					printf("Found freq %f - amp %f: bearing issue\n", fk[i], Ak[i]);
					break;
				}
			}
		}
	}

	free(fk);
	free(Ak);
}

void start_issues_task(){
    //Start real time threads
	// For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm; 
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity
	
	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

	int priority = 50;
	int periodicity = 500;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	int err=pthread_create(&threadid, &attr, issues_task_code, (void*)cab_buffer);
	if(err != 0)
		printf("\n\r Error creating Issues Thread [%s]", strerror(err));
}

void *rtdb_task_code(void *arg)
{
	printf("RTDB Task: Started\n");
	/* Timespec variables to manage time */
	struct timespec ts, // thread next activation time (absolute)
			ta, 		// activation time of current thread activation (absolute)
			tit, 		// thread time from last execution,
			ta_ant, 	// activation time of last instance (absolute),
			tp; 		// Thread period

	int niter = 0;
	int update;
	
	usleep(1000000);

	tp.tv_nsec = 500 * 1000 * 1000;
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

		if(update) {		  
			printf("Issues Task: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r", (float)min_iat/1000, (float)max_iat/1000);
			update = 0;
		}

		//Print RTDB Values
		printf("RTDB Values:\n");
		printf("Motor Speed: %dHz\n", gRTDB->motor_speed);
		printf("Highest Amplitude: %f\n", gRTDB->highest_amplitude);
		printf("Bearing Issues: %u\n", gRTDB->has_bearing_issues);
	}
}


void start_rtdb_task(){
    //Start real time threads
	// For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm; 
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity
	
	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

	int priority = 30;
	int periodicity = 500;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	int err=pthread_create(&threadid, &attr, rtdb_task_code, NULL);
	if(err != 0)
		printf("\n\r Error creating RTDB Thread [%s]", strerror(err));
}