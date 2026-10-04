#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>

/*A lot of this code sucks and is inefficient, sorry*/

void handleInterrupt(){
    printf("\n");
    exit(0);
}

int terminal(char *splitBufferInput, char *buffer, size_t size, char **historyArray, int lastIndex){ //splitBufferInput depends, if no || or &&, whole buffer, else, just current section of buffer split on || or &&
    char *splitBuffer = strtok(splitBufferInput, " "); //split line on spaces and get first word in split line
    int error = 0;
    bool nlFlag = true;
    if(strstr(buffer, " > ") != NULL){
        nlFlag = false;
    }
    if(splitBuffer != NULL && strcmp(splitBuffer, "echo") == 0){ //echo code
        splitBuffer = strtok(NULL, " ");
        int firstWord = 1; 
        if(nlFlag){
            printf("\n");
        }
        while(splitBuffer != NULL){ //while next word in split line isnt null
            if(!firstWord) { //makes sure only non-first words get spaces before the word
                printf(" ");
            }
            printf("%s", splitBuffer);
        
            firstWord = 0;
            splitBuffer = strtok(NULL, " ");
        }
        printf("\n");
    }
    else if(splitBuffer != NULL && strcmp(splitBuffer, "cd") == 0){ //cd code
        char *newDirectory = NULL;
        splitBuffer = strtok(NULL, " ");
        if(splitBuffer == NULL || strlen(splitBuffer) == 0){ //if null cd prompt
            char *homeDir = strdup(getenv("HOME")); //get home env variable
            if (chdir(homeDir) == -1) { 
                exit(1);
            }
            printf("\n");
            fflush(stdout);
            free(homeDir);
        }
        else{
            //print new directory
            newDirectory = malloc(sizeof(splitBuffer));
            newDirectory = splitBuffer;
            if((splitBuffer = strtok(NULL, " ")) != NULL){
                printf("\nwshell: cd: too many arguments\n");
                fflush(stdout);
                return 2;
            }
            else if(chdir(newDirectory) == -1){
                printf("\nwshell: no such directory: %s\n", newDirectory);
                fflush(stdout);
                return 2;
            }
            else{
                printf("\n");
                fflush(stdout);
            }
        }
    }
    else if(splitBuffer != NULL && strcmp(splitBuffer, "pwd") == 0){
        printf("\n%s\n", getcwd(buffer, size)); //print current directory
        fflush(stdout);
    }
    else if(splitBuffer != NULL && strcmp(splitBuffer, "history") == 0){
        printf("\n");
        fflush(stdout);
        int i = 0;
        while(i < lastIndex && historyArray[i] != NULL){
            printf("%s\n", historyArray[i]);
            fflush(stdout);
            i++;
        }
    }
    else{ //if not one of non-exec processes...
        if(splitBuffer != NULL){
            if(strstr(splitBuffer, "/usr/bin/true") != NULL || strstr(splitBuffer, "/usr/bin/false") != NULL){
                nlFlag = false;
            }

            if(nlFlag){
                printf("\n");
            }
            fflush(stdout);
            int rc = fork();
            if(rc == -1){
                exit(1);
            }
            else if(rc == 0){
                //Child process
                int arraySize = 0;
                int capacity = 1;
                char **args = malloc(arraySize * sizeof(char *));
                while(splitBuffer != NULL){ //collect args from string
                    if(arraySize == capacity){
                        capacity *= 2;
                        args = realloc(args, capacity * sizeof(char *));
                        if(args == NULL){
                            exit(1);
                        }
                    }
                    args[arraySize] = splitBuffer;
                    arraySize += 1;
                    splitBuffer = strtok(NULL, " ");
                }
                args[arraySize] = NULL;
                execvp(args[0], args); //run current portion with args

                exit(1); //should not reach here unless there is an error
            }
            else if (rc > 0){
                //Parent process (put in catch for exit(1))
                int childStatus;
                pid_t pidReturn = waitpid(rc, &childStatus, 0);
                if(pidReturn == -1){
                    exit(1);
                }
                if(WIFEXITED(childStatus)){ //used https://stackoverflow.com/questions/33489653/determine-whether-or-not-execvp-succeeded-or-failed to help here to look for the "EXIT" functions
                    childStatus = WEXITSTATUS(childStatus);
                    if(childStatus == 1){
                        error = 1;
                        return error;
                    }
                }
            }
        }
        else{
            error = 1;
            return error;
        }
    }
    return error;
}

void adjustArray(char** historyArray, int* lastIndex, char* input){
    if(*lastIndex < 10){
        historyArray[*lastIndex] = strdup(input); //accesses value at dereferenced index
        (*lastIndex)++;
        historyArray[*lastIndex] = NULL;
    }else{
        free(historyArray[0]); //replace last entry and move everything back one index
        for (int i = 1; i < *lastIndex; i++){
            historyArray[i - 1] = historyArray[i];
        }
        historyArray[9] = strdup(input); //set most recent entry
    }
}

int main(int argc, char *argv[]){ //TODO: for piping and redirection, may need to pass new pointer
    signal(SIGINT, handleInterrupt);
    char *buffer = NULL; //buffer for reading in data
    size_t size = 1024; //size of buffer
    ssize_t lineRead; //size of line read
    char **historyArray = malloc(11 * sizeof(char *)); //create history array
    int lastIndex = 0;
    historyArray[lastIndex] = NULL;
    int error = 0;
    /*This looks bad but didnt feel like making a hash table*/
    size_t originalOpArray[5] = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX}; //0 = orOp, 1 = andOp, 2 = redirOp, 3 = addRedirOp, 4 = pipeOp
    size_t opArray[5];
    char *splitBuffer; //first part of string
    char *bufferCopy; //copied string
    char *delim; //string delimiter
    int childProc; //child process
    int childProc2; //second child process
    int childStatus; //child status
    pid_t pidReturn; //return of process
    char *remainingString;
    
    while(1){
        memcpy(opArray, originalOpArray, sizeof(originalOpArray));
        buffer = realloc(buffer, size);
        if(getcwd(buffer, size) == NULL){ //get current working directory and put that into buffer of size size (dynamically allocates)
            exit(1);
        }
        printf("%s$ ", basename(buffer)); //print what is stored in buffer
        fflush(stdout); //flush everything in printf buffer to stdout
        lineRead = getline(&buffer, &size, stdin); //get next line in stdin by reading it into buffer
        if(lineRead == -1){ //if line isnt null
            exit(1);
        }
        if(buffer[lineRead - 1] == '\n'){ 
            buffer[lineRead - 1] = '\0'; //replace newline with null character
        }

        if(!isatty(fileno(stdin))){ //if input from file
            printf("%s", buffer);
            fflush(stdout); //print input
        }

        if(strcmp(buffer, "exit") == 0){ //if exit is in the line...
            printf("\n");
            fflush(stdout);
            exit(0); //exit out of parent process
        }

        adjustArray(historyArray, &lastIndex, buffer);
        //0 = || Op, 1 = && Op, 2 = > Op, 3 = >> Op, 4 = | Op
        if(strstr(buffer, " || ") != NULL){
            opArray[0] = strstr(buffer, " || ") - buffer; //get substring
            delim = " || "; //set delim
        }
        else if(strstr(buffer, " && ") != NULL){
            opArray[1] = strstr(buffer, " && ") - buffer;
            delim = " && ";
        }
        else if(strstr(buffer, " > ") != NULL){
            opArray[2] = strstr(buffer, " > ") - buffer;
            delim = " > ";
        }
        else if(strstr(buffer, " >> ") != NULL){
            opArray[3] = strstr(buffer, " >> ") - buffer;
            delim = " >> ";
        }
        else if(strstr(buffer, " | ") != NULL){
            opArray[4] = strstr(buffer, " | ") - buffer;
            delim = " | ";
        }

        if(opArray[0] == SIZE_MAX && opArray[1] == SIZE_MAX && opArray[2] == SIZE_MAX && opArray[3] == SIZE_MAX && opArray[4] == SIZE_MAX){ //if no operators used...
            error = terminal(buffer, buffer, size, historyArray, lastIndex); //run terminal and check for error

            if(error == 1){ //if error, print this message
                printf("wshell: could not execute command: %s\n", buffer);
                fflush(stdout);
            }
        }
        else{
            bufferCopy = strdup(buffer); //copy string so history still works
            if(strcmp(delim, " || ") == 0){
                splitBuffer = strndup(bufferCopy, opArray[0]); //set splitBuffer to first portion of string before delimitter
            }
            else if(strcmp(delim, " && ") == 0){
                splitBuffer = strndup(bufferCopy, opArray[1]);
            }
            else if(strcmp(delim, " > ") == 0){
                splitBuffer = strndup(bufferCopy, opArray[2]);
            }
            else if(strcmp(delim, " >> ") == 0){
                splitBuffer = strndup(bufferCopy, opArray[3]);
            }
            else if(strcmp(delim, " | ") == 0){
                splitBuffer = strndup(bufferCopy, opArray[4]);
            }

            if(strcmp(delim, " | ") != 0){ //if not a pipe op
                childProc = fork();
                if(childProc == -1){
                    exit(1);
                }
                else if(childProc == 0){
                    //Child process
                    FILE *file;
                    if(strcmp(delim, " > ") == 0 || strcmp(delim, " >> ") == 0){  //if file op, redirect stdout to file
                        fclose(stdout);
                        if(strcmp(delim, " > ") == 0){
                            file = fopen(strstr(bufferCopy, delim) + strlen(delim), "w");
                        }
                        else{
                            file = fopen(strstr(bufferCopy, delim) + strlen(delim), "a");
                        }
                        if(file == NULL){
                            exit(1);
                        }
                    }
                    error = terminal(splitBuffer, buffer, size, historyArray, lastIndex); //run terminal and retrieve if terminal returned error

                    if(file && (strcmp(delim, " > ") == 0 || strcmp(delim, " >> ") == 0)){ //close file if file op
                        fclose(file);
                    }
                    

                    if(error != 0){
                        exit(1);
                    }
                    else{
                        exit(0);
                    }
                }
                else if (childProc > 0){
                    //Parent process (put in catch for exit(1))
                    pidReturn = waitpid(childProc, &childStatus, 0);
                    if(pidReturn == -1){
                        exit(1);
                    }
                    if(WIFEXITED(childStatus)){ //used https://stackoverflow.com/questions/33489653/determine-whether-or-not-execvp-succeeded-or-failed to help here to look for the "EXIT" functions
                        childStatus = WEXITSTATUS(childStatus);
                        if((strcmp(delim, " || ") == 0) || (strcmp(delim, " && ") == 0)){
                            if(childStatus == 1){ //if child did not succeed
                                //printf("wshell: command not found: %s\n", splitBuffer);
                                if(opArray[0] != SIZE_MAX){ //if not or op
                                    remainingString = strstr(bufferCopy, delim) + strlen(delim);

                                    error = terminal(remainingString, buffer, size, historyArray, lastIndex);

                                    if(error == 1){
                                        printf("wshell: command not found: %s\n", remainingString);
                                        fflush(stdout);
                                    }
                                }
                                else{
                                    printf("\n");
                                    fflush(stdout);
                                }
                            }
                            else{ //if child succeeded
                                if(opArray[1] != SIZE_MAX){ //if not and op
                                    remainingString = strstr(bufferCopy, delim) + strlen(delim);

                                    error = terminal(remainingString, buffer, size, historyArray, lastIndex);

                                    if(error == 1){
                                        printf("wshell: command not found: %s\n", remainingString);
                                        fflush(stdout);
                                    }
                                }
                                else if(opArray[0] != SIZE_MAX){
                                    printf("\n");
                                    fflush(stdout);
                                }
                            }
                        }
                        else{
                            printf("\n");
                            fflush(stdout);
                        }
                    }
                }
            }
            else{
                int p[2];
                pipe(p); //create pipe
                
                printf("\n");
                fflush(stdout);
                
                childProc = fork();
                if(childProc == 0){
                    close(p[0]); //close read end of pipe
                    dup2(p[1], STDOUT_FILENO); //make write end of pipe replace stdout
                    close(p[1]); //close write end of pipe
                    
                    char *tkn = strtok(splitBuffer, " ");
                    char **args = malloc(10 * sizeof(char *));
                    int i = 0;
                    while (tkn){ //get args
                        args[i++] = tkn;
                        tkn = strtok(NULL, " ");
                    }

                    args[i] = NULL;
                    execvp(args[0], args); //execute first portion of line
                    exit(1);
                }
                //repeat for second fork just opposite for reading from stdin
                childProc2 = fork();
                if(childProc2 == 0){
                    close(p[1]);
                    dup2(p[0], STDIN_FILENO);
                    close(p[0]);

                    char *remainingString = strdup(strstr(bufferCopy, delim) + strlen(delim));
                    char *tkn = strtok(remainingString, " ");
                    char **args = malloc(10 * sizeof(char *));
                    int i = 0;
                
                    while (tkn){
                        args[i++] = tkn;
                        tkn = strtok(NULL, " ");
                    }

                    args[i] = NULL;
                    execvp(args[0], args);
                    exit(1);
                }
                
                close(p[0]);
                close(p[1]);
                
                waitpid(childProc, NULL, 0);
                waitpid(childProc2, NULL, 0);
            }
            free(splitBuffer);
            free(bufferCopy);
        }
    }
    free(buffer);
    free(historyArray);
    return 0;
}