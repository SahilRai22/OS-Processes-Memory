/**
 * @Author: Sahil Rai
*/
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#include "coursework.c"
#include "linkedlist.c"
/*define processes*/
Process * simProcess[NUMBER_OF_PROCESSES];
Process * process; 
Process * tP; 
Process * sP;
Process * pP; 
/*Intiliase linked list*/
LinkedList readyQ = LINKED_LIST_INITIALIZER;
LinkedList terminatedQ = LINKED_LIST_INITIALIZER; 
LinkedList pTable[SIZE_OF_PROCESS_TABLE];
LinkedList pagingQ = LINKED_LIST_INITIALIZER; 
LinkedList frameList = LINKED_LIST_INITIALIZER;

FrameEntry * frame;
//FrameEntry * frame = {PAGE_TABLE_ENTRY_INITIALIZER};
//LinkedList frameList[NUMBER_OF_FRAMES] = frame;


//FrameEntry * frame = (&(FrameEntry) PAGE_TABLE_ENTRY_INITIALIZER);
//LinkedList * frameList = LINKED_LIST_INITIALIZER;

/*defining semaphores*/
sem_t semGenerator, semSimulator ,semDaemon, semCount, runningp;
/*defining threads*/ 
pthread_t generatorThread, simulatorThread, daemonThread, pagingThread;
/*defining mutex*/
pthread_mutex_t mutexLock, mutexThread, mutexPTable, syncr, syncpf;
pthread_mutex_t processCounterLock, processPageLock;
/*definee counters & index i*/
int processCounter, pageFaultCounter, i; 

/*this method simulates admitted*/
void simAdmitted(Process * simProcess){
    printf ("ADMITTED: [PID = %d, Hash = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
    simProcess->iPID, simProcess->iHash, simProcess->iBurstTime, simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);

    pthread_mutex_lock(&mutexPTable);
	addLast(simProcess, &pTable[simProcess->iHash]); 
	pthread_mutex_unlock(&mutexPTable);
}
/*this method simulates ready*/
void simReady(Process * simProcess){
   printf ("READY: [PID = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
   simProcess->iPID,simProcess->iBurstTime,simProcess->iRemainingBurstTime,\
   simProcess->iLocality, simProcess->iWidth);

   pthread_mutex_lock(&mutexLock);
   addLast (simProcess,&readyQ);
   pthread_mutex_unlock(&mutexLock);
}

/*this method simulates terminated*/
void simTerminated(Process * simProcess){
    printf ("TERMINATED [PID = %d, RemainingBurstTime = %d]\n",simProcess->iPID, simProcess->iRemainingBurstTime);
 	pthread_mutex_lock(&mutexThread);
	addLast (simProcess,&terminatedQ);
	pthread_mutex_unlock(&mutexThread);	

}
/*this mehtod simulates page fault*/
void simPageFault(Process * simProcess){
    printf("PAGE_FAULTED:[PID = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d, Page = %d, Offset = %d]\n",\
    simProcess->iPID,simProcess->iBurstTime,simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);

	pthread_mutex_lock(&processPageLock);	
	addLast(simProcess,&pagingQ);
	pthread_mutex_unlock(&processPageLock);
}
/*this mehtod simulates cleared*/
void simCleared(Process * simProcess){
    pthread_mutex_unlock(&mutexThread);
    int response = getDifferenceInMilliSeconds(simProcess->oFirstTimeRunning,simProcess->oLastTimeRunning);
	int turnAround =getDifferenceInMilliSeconds(simProcess->oTimeCreated,simProcess->oLastTimeRunning);
	printf ("CLEARED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d, PageFaults = %d]\n",\
    simProcess->iPID, response, turnAround, simProcess->iPageFaults); 	
    free(simProcess);
	pthread_mutex_lock(&mutexPTable);
}
/**
 * This method loops through processes
 * locks mutex then add process to ready Queue then unlocks
 * unlocks semaphore for generator
 *  */
void * processGenerator (void * p) { 
	for (i = 0; i < NUMBER_OF_PROCESSES; i++) { 
		sem_wait(&semCount); 
		pthread_mutex_lock(&mutexLock);
        process = generateProcess(i); 
		addLast (process,&readyQ);
		pthread_mutex_unlock(&mutexLock);
        simAdmitted(process);
		sem_post(&semGenerator); 
	}
}
/**
 * This process tries to simulate ready queue
 * on condition processes ready, terminated and pagefault
*/
void * processSimulator (void * p) { 
	while (1) {
        if (processCounter >= NUMBER_OF_PROCESSES ) {
			pthread_exit(0);
		}
		sem_wait(&semGenerator);
		pthread_mutex_lock(&mutexLock);

		sP = getHead(readyQ)->pData; 
		runPreemptiveProcess(sP,1);
		removeFirst(&readyQ);
		pthread_mutex_unlock(&mutexLock);
		printf ("SIMULATING: [PID = %d, BurstTime = %d, RemainingBurstTime = %d]\n",\
        sP->iPID, sP->iBurstTime, sP->iRemainingBurstTime); 
	
		if (sP->iStatus == READY) {
			sem_post(&semGenerator);
            simReady(sP);
		}
		else if (sP->iStatus == PAGE_FAULTED) { 
			pageFaultCounter++;  
            simPageFault(sP);
			sem_post(&semDaemon);
		}
			else if (sP->iStatus == TERMINATED) {
			pthread_mutex_lock(&processCounterLock);
			processCounter++; 
			pthread_mutex_unlock(&processCounterLock);

            simTerminated(sP);	  
			sem_post(&semSimulator); 
			sem_post(&semCount);
		}
	}		
}

/*
* This method removes process with page fault from page fault queue
* adds the head to dummy frame
* then returns to ready state
*/
void * pagingDaemon (void * p ) {   
    while(1) {
	sem_wait(&semDaemon); 
		if (processCounter >= NUMBER_OF_PROCESSES ) {
		pthread_exit(0);
		}
		    pthread_mutex_lock(&processPageLock); 
			pP = getHead(pagingQ)->pData; 
			mapDummyFrame(pP); 
		    removeFirst(&pagingQ);
			pthread_mutex_unlock(&processPageLock); 

			pthread_mutex_lock(&mutexLock);
			pP->iStatus = READY;
			addLast (pP,&readyQ);
			pthread_mutex_unlock(&mutexLock);  
			sem_post(&semGenerator);  	

			//frame
			// loop thorugh frameList linked list
			// place initalised frame in every single slot
			// go through whole list at i put intilaised frame then go to next i. repeat
			int i;
			for(i =0; i < NUMBER_OF_FRAMES; i++) {
				FrameEntry * frame = (FrameEntry*) malloc(sizeof(FrameEntry));
				*frame = (FrameEntry) PAGE_TABLE_ENTRY_INITIALIZER;
				addLast((void*)frame, &frameList);
			}

            process = removeFirst(&pagingQ);
            frame = removeFirst(&frameList);
            reclaimFrame(frame);
            mapFrame(process, frame);
            addLast(frame, &frameList);
            pthread_mutex_unlock(&processPageLock);//cpf

            pthread_mutex_lock(&mutexLock);//cr
            process->iStatus = READY;
            addLast(process, &readyQ);
            pthread_mutex_unlock(&mutexLock);//cr

            sem_post(&semGenerator);

	}
}

/*
 * This method is called when process is added to terminated queue
 * removes process from queue 
*/
void * terminatingDaemon (void * p) {	
	while(1) {
		if (processCounter >= NUMBER_OF_PROCESSES && getHead(terminatedQ) == NULL) {
		sem_post(&semDaemon);
		pthread_exit(0);   
		}
			sem_wait(&semSimulator);
			pthread_mutex_lock(&mutexThread);
			tP = getHead(terminatedQ)->pData; 
			removeFirst(&terminatedQ );
            simCleared(tP);

			removeData(tP,&pTable[tP->iHash]);
			pthread_mutex_unlock(&mutexPTable);
	}
	sem_wait(&semSimulator);
free(tP->apPageTable[i]);
}

void initialiseSem(){
    sem_init(&semCount, 0, MAX_CONCURRENT_PROCESSES);  
	sem_init(&semGenerator, 0, 0);  
	sem_init(&semSimulator, 0, 0); 
	sem_init(&semDaemon, 0, 0);
}

int createThread(){
    pthread_create(&generatorThread, NULL, &processGenerator, NULL);
	pthread_create(&simulatorThread, NULL, &processSimulator, NULL);
	pthread_create (&pagingThread, NULL, &pagingDaemon, NULL) ;
	pthread_create (&daemonThread, NULL, &terminatingDaemon, NULL); 
}

void joinThread(){
    pthread_join(generatorThread, NULL);  
	pthread_join(simulatorThread, NULL);
	pthread_join(pagingThread, NULL); 
	pthread_join(daemonThread, NULL); 
}
/*erasing mutex and semaphores*/
void memoryClean(){
    pthread_mutex_destroy(&mutexLock); 
	pthread_mutex_destroy(&mutexThread);
	pthread_mutex_destroy(&processCounterLock);
	pthread_mutex_destroy(&mutexPTable); 
	pthread_mutex_destroy(&processPageLock);

	sem_destroy(&semCount); 
	sem_destroy(&semGenerator); 
	sem_destroy(&semSimulator); 
	sem_destroy(&semDaemon); 
}




int main () {
    initialiseSem();
	pthread_mutex_init(&mutexLock, NULL);
	pthread_mutex_init(&mutexThread, NULL);
	pthread_mutex_init(&processCounterLock, NULL);
	pthread_mutex_init(&mutexPTable, NULL);
	pthread_mutex_init(&processPageLock, NULL);
    createThread();
    joinThread();
	 
    float avgPageFault = (pageFaultCounter / NUMBER_OF_PROCESSES);

	printf("SIMULATION FINISHED: Total PageFaults = %d, Average PageFaults = %.2f\n",\
    pageFaultCounter, avgPageFault); 

memoryClean();
return 0;
}
