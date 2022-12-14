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
Process * simulatorHash; 
Process * simulator; 
Process * daemonHash; 
Process * daemonProcess; 
Process * generatorHash; 

int processCounter;
/*Initialise linked list*/
LinkedList readyQ = LINKED_LIST_INITIALIZER;
LinkedList terminatedQ= LINKED_LIST_INITIALIZER;

/*Defining semaphore operators*/
sem_t semGenerator, semSimulator, semDaemon; 
/*Defining threads*/
pthread_t generatorThread, simulatorThread, daemonThread;

pthread_mutex_t mutexLock; 
LinkedList hashTable[SIZE_OF_PROCESS_TABLE] = LINKED_LIST_INITIALIZER; 
/*this method simulates admitted*/
void simAdmitted(Process * simProcess){
    printf("ADMITTED: [PID = %d, Hash = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
    simProcess->iPID, simProcess->iHash, simProcess->iBurstTime,\
    simProcess->iRemainingBurstTime, simProcess->iLocality, simProcess->iWidth);

    addLast(simProcess, &readyQ);
    processCounter++;
}
/*this method simulates ready*/
void simReady(Process * simProcess){
    printf("READY: [PID = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
    simProcess->iPID, simProcess->iBurstTime, simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);

    removeData(simProcess, &readyQ);
    addLast(simProcess, &readyQ);
}
/*this method simulates terminated*/
void simTerminated(Process * simProcess){
    processCounter--;
    printf("TERMINATED: [PID = %d, RemainingBurstTime = %d]\n",\
    simProcess->iPID, simProcess->iRemainingBurstTime);

    removeData(simProcess, &terminatedQ);
    free(simProcess);
}
/**
 * This method loops through processes
 * Adds processes to ready queue if 
 * amount of process equals to max concurrent processes
*/
void * processGenerator(void * p) {
    int i; 
    pthread_mutex_lock(&mutexLock);

        for (i = 0; i< NUMBER_OF_PROCESSES; i++) {
            simProcess[i] = generateProcess(i);
                if (processCounter == MAX_CONCURRENT_PROCESSES) {
                    pthread_mutex_unlock(&mutexLock);
                    sem_post(&semGenerator);
                    sem_wait(&semDaemon);
                    pthread_mutex_lock(&mutexLock);
                }
        simAdmitted(simProcess[i]);
        addFirst(simProcess[i], &hashTable[simProcess[i]->iHash]);
        generatorHash = getHead(hashTable[simProcess[i]->iHash])->pData;
        printf("SIMULATING: [PID = %d, BurstTime = %d, RemainingTime = %d]\n", generatorHash->iPID, generatorHash->iBurstTime, generatorHash->iRemainingBurstTime);
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
void * processSimulator(void * p) {
    sem_wait(&semGenerator);
    pthread_mutex_lock(&mutexLock);

        while (processCounter != 0){
            simulator = getHead(readyQ)->pData;
        
            runPreemptiveProcess(simulator, 0); 

            if (simulator->iStatus == TERMINATED){                
                removeData(simulator, &readyQ);
                addLast(simulator, &terminatedQ);
                pthread_mutex_unlock(&mutexLock);
                sem_post(&semSimulator); 
                sem_wait(&semGenerator); 
                pthread_mutex_lock(&mutexLock);
            }
            else if (simulator->iStatus == READY){
                simulatorHash = getHead(hashTable[simulator->iHash])->pData;
                    while (simulatorHash->iPID != simulator->iPID){
                        simulatorHash = getHead(hashTable[simulator->iHash])->pNext->pData;
                    }
            printf("SIMULATING: [PID = %d, BurstTime = %d, RemainingTime = %d]\n",\
            simulatorHash->iPID, simulatorHash->iBurstTime, simulatorHash->iRemainingBurstTime);

            simReady(simulator);
            }

        }
    pthread_mutex_unlock(&mutexLock);
    sem_post(&semSimulator);

return NULL;
}
/**
 * This method is called when process is added to terminated queue
 * removes process from queue 
*/
void * terminatingDaemon(void * p) {
    sem_wait(&semSimulator);     
    pthread_mutex_lock(&mutexLock);

        while (processCounter != 0) {
            daemonProcess = getHead(terminatedQ)->pData;
            daemonHash = getHead(hashTable[daemonProcess->iHash])->pData;

            while (daemonHash->iPID != daemonProcess->iHash) {
                daemonHash = getHead(hashTable[daemonProcess->iHash])->pNext->pData;
            }
        int response = getDifferenceInMilliSeconds(daemonProcess->oTimeCreated, daemonProcess->oFirstTimeRunning);
        int turnAround = getDifferenceInMilliSeconds(daemonProcess->oFirstTimeRunning, daemonProcess->oLastTimeRunning);
        printf("CLEARED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d]\n", daemonHash->iPID, response, turnAround);

        removeData(daemonHash, &hashTable[daemonProcess->iHash]);
        simTerminated(daemonProcess);

        pthread_mutex_unlock(&mutexLock);
        sem_post(&semDaemon);
        sem_wait(&semSimulator);
        pthread_mutex_lock(&mutexLock);
        }

    pthread_mutex_unlock(&mutexLock);

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



