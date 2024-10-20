#include "./include/monimotor.h"
#include <sched.h> //sched_setscheduler

int main(int argc, char *argv[]){

    int err, i;
    // For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm; 
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity
	
	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

    // Must be set according to needs
	int priority = atoi(argv[1]);
	int periodicity = atoi(argv[2]);

	if(priority < 1 || priority > 99){
		printf("Priority must be between 1 and 99");
		return -1;
	}
	if(periodicity < 50 || periodicity > 500){
		printf("Periodicity must be between 50 and 500 ms");
		return -1;
	}
					 
	/* Create periodic thread/task with RT scheduling attributes*/
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);

	threadArgs *args = (threadArgs *) malloc(sizeof(threadArgs));
	args->name = procname;
	args->periodicity = periodicity;

	printf("reached here");
	
	err=pthread_create(&threadid, &attr, Thread_1_code, (void*) args);
	if(err != 0)
		printf("\n\r Error creating Thread [%s]", strerror(err));

    // Probably used only for debugging
	else {
		printf("\n\r Process ID: %d", getpid());
		policy=sched_getscheduler(0);
		switch(policy){
			case SCHED_FIFO:
				sched_getparam(0,&parm);
				printf("\n\r Default thread, SCHED_FIFO, priority %d\n\r", parm.sched_priority);
				break;
			case SCHED_RR:
				sched_getparam(0,&parm);
				printf("\n\r Default thread, SCHED_RR, priority %d\n\r", parm.sched_priority);
				break;
			default:
				printf("\n\r Default thread, NOT SCHED_RR nor SCHED_FIFO\n\r");
		}
			
		/* Check process affinity */
		printf("\n\r Process affinity:");
		sched_getaffinity(0, sizeof(cpuset_test),&cpuset_test);
		for(i=0;i<CPU_COUNT(&cpuset_test);i++){
			if(CPU_ISSET(i,&cpuset_test))
				printf("CPU %d - Yes, ",i);
			else
				printf("CPU %d - No, ",i);		
		}
		printf("\n\r");
		

		while(1); // Ok. Thread shall run
	}
}