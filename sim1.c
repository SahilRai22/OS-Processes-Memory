/**
 * @Author: Sahil Rai
*/
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "coursework.c"

/**
 * Main() function:
 * 1) genereates a process (simProcess)
 * 2) Prints out Admitted, Ready & Terminated state
 * 3) Ready state has condition check of READY
 * 4) As guided runPreemptiveProcess does not simulate page fault
**/
int main(){
    Process * simProcess;
    simProcess = generateProcess(1);

    printf("ADMITTED: [PID = %d, Hash = %d, BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d] \n",\
    simProcess->iPID, simProcess->iHash, simProcess->iBurstTime, simProcess->iRemainingBurstTime, simProcess-> iLocality, simProcess->iWidth);
    
    while (simProcess -> iStatus == READY){
        runPreemptiveProcess(simProcess, false);

        if(simProcess->iRemainingBurstTime > 0){
            printf("READY: [PID = %d , BurstTime = %d, RemainingBurstTime = %d, Locality = %d, Width = %d]\n",\
            simProcess->iPID,simProcess->iBurstTime, simProcess->iRemainingBurstTime, \
            simProcess->iLocality, simProcess->iWidth); 
        }
    }

    int response = getDifferenceInMilliSeconds(simProcess->oTimeCreated, simProcess->oFirstTimeRunning);
    int turnAround = getDifferenceInMilliSeconds(simProcess->oFirstTimeRunning, simProcess->oLastTimeRunning);
    printf("TERMINATED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d]\n",\
    simProcess->iPID, response, turnAround);
    /*frees process heap*/
    free(simProcess);

return 0;

}


