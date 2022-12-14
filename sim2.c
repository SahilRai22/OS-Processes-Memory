/**
 * @Author: Sahil Rai
*/
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "coursework.c"
#include "linkedlist.c"

Process * simProcess[NUMBER_OF_PROCESSES];

/**Initialise linked list*/
LinkedList terminatedQ = LINKED_LIST_INITIALIZER;
LinkedList readyQ = LINKED_LIST_INITIALIZER;

Process * sim; 

/**This method simulated Ready queue*/
void simReady(Process * simProcess){
    printf("READY: [PID = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
    simProcess->iPID, simProcess->iBurstTime, simProcess->iRemainingBurstTime, simProcess->iLocality,\
    simProcess->iWidth);

    removeData(simProcess, &readyQ);
    addLast(simProcess, &readyQ);
}

/**This method simulates Admitteedqueue*/
void simAdmitted(Process * simProcess){
    printf("ADMITTED: [PID = %d, Hash = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
    simProcess->iPID, simProcess->iHash, simProcess->iBurstTime, simProcess->iRemainingBurstTime,\
    simProcess->iLocality, simProcess->iWidth);
    
    addLast(simProcess, &readyQ);
}

/**This method simulates Terminated queue*/
void simTerminated(Process * simProcess){
    int response = getDifferenceInMilliSeconds(simProcess->oTimeCreated, simProcess->oFirstTimeRunning);
    int turnAround = getDifferenceInMilliSeconds(simProcess->oFirstTimeRunning, simProcess->oLastTimeRunning);
    printf("TERMINATED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d]\n",\
    simProcess->iPID, response, turnAround);
}


/**
 * Main() function:
 * 1) Uses pre defined number of process to simulate processes in round robin fashion
 * 2) Process in ready state added back to ready queue
 * 3) Process in terminated states added back to terminated queue
 * 4) Process freed from terminated queue
*/
int main(int argc, char *argv[]){
    int i;
    int endProcess = 0;

    for (i = 0; i < NUMBER_OF_PROCESSES; i++){
        simProcess[i] = generateProcess(i);
        simAdmitted(simProcess[i]);
    }

    while (endProcess != NUMBER_OF_PROCESSES){ 
        sim = getHead(readyQ) -> pData; 
        
        runPreemptiveProcess(sim, 0); 

        if (sim -> iStatus == TERMINATED) {    
            removeData(sim, &readyQ);
            addLast(sim, &terminatedQ);   
            simTerminated(sim);  
            endProcess++; 
            }  
            else if (sim -> iStatus == READY){ simReady(sim); }
    }

    while (endProcess!= 0){
        sim = getHead(terminatedQ)-> pData;
        removeData(sim, &terminatedQ);
        free(sim);

        endProcess--;
    }    
}


