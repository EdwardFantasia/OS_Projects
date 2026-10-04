#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>

struct job {
    int id;
    int length;
    int arrival;
    int startTime;
    struct job *next;
    bool accessed;
    //time completed = startTime + length
    int rrRemaining; //time completed when doing round robin
};

struct job* enqueue(int jobId, int runTime) {
    struct job *nextJob = malloc(sizeof(struct job));
    if (!nextJob) {
        printf("malloc fail");
        return NULL;
    }
    nextJob->id = jobId;
    nextJob->length = runTime;
    nextJob->next = NULL;
    return nextJob;
}

void npeAnalyze(struct job* curJob, int *totalRespTPtr, int *totalTurnTPtr, int *totalWaitTPtr){ //TODO: figure out why this doesnt work in FIFO and SJF functions
    printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curJob->id, (curJob->startTime - curJob->arrival), ((curJob->startTime - curJob->length) - curJob->arrival) * -1, (curJob->startTime - curJob->arrival));
    fflush(stdout);
    (*totalRespTPtr) += curJob->startTime - curJob->arrival;
    (*totalTurnTPtr) += curJob->arrival - (curJob->startTime - curJob->length);
    (*totalWaitTPtr) += curJob->startTime - curJob->arrival;
}

void fifoScheduler(char *filen, char* analysis) {
    int arrivalTime, runTime, id = 0;
    FILE *file = fopen(filen, "r");
    if (!file) {
        printf("error");
        return;
    }
    struct job *rearJob = NULL;
    struct job *frontJob = NULL;
    struct job *curJob = NULL;
    int totalTime = 0;
    printf("Execution trace with FIFO:\n");
    fflush(stdout);
    while (fscanf(file, "%d,%d", &arrivalTime, &runTime) == 2) { // Enqueue
        curJob = enqueue(id, runTime);
        if (!curJob) {
            fclose(file);
            return;
        }
        curJob->arrival = arrivalTime;
        curJob->startTime = totalTime;
        if (frontJob == NULL) {
            frontJob = curJob;
            rearJob = curJob;
        } else {
            rearJob->next = curJob;
            rearJob = curJob;
        }
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", totalTime, curJob->id, curJob->arrival, curJob->length);
        fflush(stdout);
        totalTime += curJob->length;
        id++;
    }
    fclose(file);
    printf("End of execution with FIFO.\n");
    fflush(stdout);

    if(strcmp(analysis, "1") == 0){
        printf("Begin analyzing FIFO:\n");
        fflush(stdout);
        int totalRespT, totalTurnT, totalWaitT = 0;
        int jobsRemaining = 0;
        curJob = frontJob;
        while(curJob != NULL){
            printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curJob->id, curJob->startTime - curJob->arrival, (curJob->startTime + curJob->length) - curJob->arrival, curJob->startTime - curJob->arrival);
            totalRespT += curJob->startTime - curJob->arrival;
            totalTurnT += (curJob->startTime + curJob->length) - curJob->arrival;
            totalWaitT += curJob->startTime - curJob->arrival;
            curJob = curJob->next;
            jobsRemaining++;
        }
        printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\nEnd analyzing FIFO.\n", round(totalRespT) / jobsRemaining, round(totalTurnT) / jobsRemaining, round(totalWaitT) / jobsRemaining);
    }
    //TODO: free memory
}

void sjfScheduler(char *filen, char *analysis) {
    int arrivalTime, runTime, id = 0;
    FILE *file = fopen(filen, "r");
    if (!file) {
        printf("Error opening file\n");
        return;
    }

    struct job *listHead, *listTail = NULL;

    while (fscanf(file, "%d,%d", &arrivalTime, &runTime) == 2) {
        struct job *curJob = malloc(sizeof(struct job));
        curJob->id = id;
        curJob->length = runTime;
        curJob->arrival = arrivalTime;
        curJob->next = NULL;
        curJob->accessed = false;

        if (listHead == NULL) {
            listHead = listTail = curJob;
        } else {
            listTail->next = curJob;
            listTail = curJob;
        }
        id++;
    }
    fclose(file);

    int totalTime = 0;
    int jobsRemaining = id;
    
    printf("Execution trace with SJF:\n");

    struct job *shortestJob;
    struct job *curJob;
    while (jobsRemaining > 0) {
        shortestJob = NULL;
        curJob = listHead;
        int lowestLength = INT_MAX;
        while (curJob != NULL) {
            if ((curJob->arrival <= totalTime && curJob->length < lowestLength) && !curJob->accessed) {
                shortestJob = curJob;
                lowestLength = curJob->length;
            }
            curJob = curJob->next;
        }

        if (shortestJob == NULL) {
            totalTime++;
            continue;
        }
        shortestJob->startTime = totalTime;
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", shortestJob->startTime, shortestJob->id, shortestJob->arrival, shortestJob->length);
        totalTime += shortestJob->length;
        shortestJob->accessed = true;
        jobsRemaining--;
    }
    printf("End of execution with SJF.\n");

    if(strcmp(analysis, "1") == 0){
        curJob = listHead;
        int totalRespT, totalTurnT, totalWaitT = 0;
        printf("Begin analyzing SJF:\n");
        fflush(stdout);
        while(curJob != NULL){
            printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curJob->id, curJob->startTime - curJob->arrival, (curJob->startTime + curJob->length) - curJob->arrival, curJob->startTime - curJob->arrival);
            totalRespT += curJob->startTime - curJob->arrival;
            totalTurnT += (curJob->startTime + curJob->length) - curJob->arrival;
            totalWaitT += curJob->startTime - curJob->arrival;
            curJob = curJob->next;
            jobsRemaining++;
        }
        printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\nEnd analyzing SJF.\n", round(totalRespT) / jobsRemaining, round(totalTurnT) / jobsRemaining, round(totalWaitT) / jobsRemaining);
    }
    //TODO: free memory
}

typedef struct rrJob {
    int id;
    int arrival;
    int length;
    int remaining;
    struct rrJob* next;
    bool enqueued;
    int totalWaitTime; //if enqueued and if reenqueued, increase wait time
    int lastRunInt;
    int completionTime;
    int start;
    bool waiting;
} rrJob;

typedef struct Queue {
    rrJob* front;
    rrJob* rear;
} Queue;

rrJob* createJob(int id, int arrival, int length) {
    rrJob* newJob = (rrJob*)malloc(sizeof(rrJob));
    newJob->id = id;
    newJob->arrival = arrival;
    newJob->length = length;
    newJob->remaining = length;
    newJob->next = NULL;
    newJob->enqueued = false;
    newJob->start = INT_MIN;
    newJob->totalWaitTime = 0;
    newJob->waiting = false;
    
    return newJob;
}

void rrenqueue(Queue* q, rrJob* job) {
    job->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = job;
    } else {
        q->rear->next = job;
        q->rear = job;
    }
    job->enqueued = true;
}

rrJob* rrdequeue(Queue* q) {
    if (q->front == NULL) return NULL;
    rrJob* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    return temp;
}

void rrscheuler(char* filename, int timeSlice, char* analysis){
    Queue jobQueue = {NULL, NULL}; //establish queue
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file.\n");
        return;
    }

    int arrivalTime, runTime;
    int totalTime = 0;
    int totalJobs = 0;
    rrJob* jobs[20];

    //put jobs into array in while loop
    while (fscanf(file, "%d,%d", &arrivalTime, &runTime) == 2) { 
        jobs[totalJobs] = createJob(totalJobs, arrivalTime, runTime);
        totalJobs++;
    }
    fclose(file);

    int completedJobs = 0;
    rrJob* curJob = NULL;
    rrJob* prevJob;

    printf("Execution trace with RR:\n");
    fflush(stdout);

    while(completedJobs < totalJobs){
        for(int i = 0; i < totalJobs; i++){
            if(jobs[i]->arrival <= totalTime && jobs[i]->remaining > 0 && jobs[i]->enqueued == false){
                rrenqueue(&jobQueue, jobs[i]);
            }
        }

        if(jobQueue.front == NULL){
            totalTime++;
            continue;
        }

        prevJob = curJob;

        curJob = rrdequeue(&jobQueue);

        if(curJob->waiting){
            curJob->totalWaitTime += totalTime - curJob->lastRunInt;
            curJob->waiting = false;
        }

        int runTime = 0;
        if(curJob->remaining > timeSlice){
            runTime = timeSlice;
        }
        else{
            runTime = curJob->remaining;
        }

        if(curJob->start == INT_MIN){
            curJob->start = totalTime;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", totalTime, curJob->id, curJob->arrival, runTime);
        fflush(stdout);

        curJob->remaining -= runTime;
        totalTime += runTime;

        Queue newQ = {NULL, NULL};

        for(int i = 0; i < totalJobs; i++){
            if(jobs[i]->arrival <= totalTime && jobs[i]->remaining > 0 && jobs[i]->enqueued == false){
                rrenqueue(&newQ, jobs[i]);
            }
        }

        if(newQ.front != NULL){
            if(jobQueue.front != NULL){
                newQ.rear->next = jobQueue.front;
                jobQueue.front = newQ.front;
            }
            else{
                jobQueue = newQ;
            }
        }

        if(curJob->remaining > 0){
            if(!curJob->waiting && prevJob != curJob){
                curJob->lastRunInt = totalTime; //when job finished and started waiting
                curJob->waiting = true; //job now waiting
            }
            rrenqueue(&jobQueue, curJob);
        }else{
            completedJobs++;
            curJob->waiting = false;
            curJob->completionTime = totalTime;
        }
    }

    printf("End of execution with RR.\n");

    if(strcmp(analysis, "1") == 0){
        int totalRespT, totalTurnT, totalWaitT = 0;
        printf("Begin analyzing RR:\n");
        fflush(stdout);
        for(int i = 0; i < totalJobs; i++){
            printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", jobs[i]->id, jobs[i]->start - jobs[i]->arrival, jobs[i]->completionTime - jobs[i]->arrival, (jobs[i]->start - jobs[i]->arrival) + jobs[i]->totalWaitTime);
            totalRespT += jobs[i]->start - jobs[i]->arrival; //good
            totalTurnT += jobs[i]->completionTime - jobs[i]->arrival; //good
            totalWaitT += (jobs[i]->start - jobs[i]->arrival) + jobs[i]->totalWaitTime; //(start - arrival) + waitTime
        }
        printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\nEnd analyzing RR.\n", round(totalRespT) / totalJobs, round(totalTurnT) / totalJobs, round(totalWaitT) / totalJobs);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("not enough arguments");
        return 1;
    }

    char *analysis = argv[1];
    char *scheduler = argv[2];
    char *workFile = argv[3];
    char *timeslice = argv[4];

    if(strcmp(scheduler, "FIFO") == 0){
        fifoScheduler(workFile, analysis);
    }
    else if(strcmp(scheduler, "SJF") == 0){
        sjfScheduler(workFile, analysis);
    }
    else if(strcmp(scheduler, "RR") == 0){
        int slice = atoi(timeslice);
        rrscheuler(workFile, slice, analysis);
    }
    return 0;
}
