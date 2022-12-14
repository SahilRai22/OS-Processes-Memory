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

Process * simProcess[NUMBER_OF_PROCESSES];
Process * process; 

int processCounter;
/*Initialise linked list*/
LinkedList readyQ = LINKED_LIST_INITIALIZER;
LinkedList terminatedQ= LINKED_LIST_INITIALIZER;

/*Defining semaphores*/
sem_t semGenerator, semSimulator, semDaemon;  
/*Defining threads*/
pthread_t generatorThread, simulatorThread, daemonThread;

pthread_mutex_t mutexLock; 

void simAdmitted(Process * simProcess){
    printf("ADMITTED: [PID = %d, Hash = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d] \n",\
    simProcess->iPID, simProcess->iHash, simProcess->iBurstTime, simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);

    addLast(simProcess, &readyQ);
    processCounter++;
}

void simReady(Process * simProcess){
    printf("READY: [PID = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d] \n",\
    simProcess->iPID, simProcess->iBurstTime, simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);

    removeData(simProcess, &readyQ );
    addLast(simProcess, &readyQ );
}

void simTerminated(Process * simProcess){
    int response= getDifferenceInMilliSeconds(simProcess->oTimeCreated, simProcess->oFirstTimeRunning);
    int turnAround = getDifferenceInMilliSeconds(simProcess->oFirstTimeRunning, simProcess->oLastTimeRunning);
    printf("TERMINATED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d] \n",\
    simProcess->iPID, response, turnAround);

    processCounter--;
    removeData(simProcess, &terminatedQ);

free(simProcess);
}
/**
 * This method loops through processes
 * Adds processes to ready queue if 
 * amount of process equals to max concurrent processes
*/
void * processGenerator(){
    int i;
    pthread_mutex_lock(&mutexLock);
    
        for (i=0; i < NUMBER_OF_PROCESSES; i++) {
            simProcess[i] = generateProcess(i);

            if (processCounter == MAX_CONCURRENT_PROCESSES) {
                pthread_mutex_unlock(&mutexLock);
                sem_post(&semGenerator);
                sem_wait(&semDaemon);
                pthread_mutex_lock(&mutexLock);
                }
            simAdmitted(simProcess[i]);
        }

        pthread_mutex_unlock(&mutexLock);
        sem_post(&semGenerator);
    
        while (processCounter != 0) {
            pthread_mutex_lock(&mutexLock);
            pthread_mutex_unlock(&mutexLock);
        
            sem_wait(&semDaemon);
            sem_post(&semGenerator);
        }
    return NULL;    
}
/**
 * This method waits for process to be added to ready queue
 * Then wakes up and runs preemptive process
*/
void * processSimulator(){
    sem_wait(&semGenerator);
    pthread_mutex_lock(&mutexLock);
    
        while (processCounter != 0) {
            process = getHead(readyQ)->pData;
            runPreemptiveProcess(process, 0);
            if (process->iStatus == TERMINATED) {                
                removeData(process, &readyQ);
                addLast(process, &terminatedQ);
                pthread_mutex_unlock(&mutexLock);
                sem_post(&semSimulator); 
                sem_wait(&semGenerator); 
                pthread_mutex_lock(&mutexLock);
            }
            else if (process->iStatus == READY) { simReady(process); } 
        }

    pthread_mutex_unlock(&mutexLock);
    sem_post(&semSimulator);

return NULL;
}

/**
 * This method is called when process is added to terminated queue
 * removes process from queue 
*/
void * terminatingDaemon() {
    sem_wait(&semSimulator);
    pthread_mutex_lock(&mutexLock);

        while (processCounter != 0) {
            process = getHead(terminatedQ)->pData;
            simTerminated(process);

            pthread_mutex_unlock(&mutexLock);
            sem_post(&semDaemon);
            sem_wait(&semSimulator);
            pthread_mutex_lock(&mutexLock);
    }
    pthread_mutex_unlock(&mutexLock);
    sem_post(&semDaemon);
return NULL;
}

void initialiseSem(){
    sem_init(&semGenerator, 0, 0);
    sem_init(&semSimulator, 0, 0);
    sem_init(&semDaemon, 0, 0);
}

void createThread(){
    pthread_create(&generatorThread, NULL, processGenerator, NULL);
    pthread_create(&simulatorThread , NULL, processSimulator, NULL);
    pthread_create(&daemonThread, NULL, terminatingDaemon, NULL);
}

void joinThread(){
    pthread_join(generatorThread, NULL);
    pthread_join(simulatorThread , NULL);
    pthread_join(daemonThread, NULL);
}


int main(int argc, char *argv[]){
    pthread_mutex_init(&mutexLock, NULL);
    initialiseSem();
    createThread();
    joinThread();

return 0;
}


