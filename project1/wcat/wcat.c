#include <stdio.h>
#include <stdlib.h>

void printFile(char *fileRef){
    FILE *file = fopen(fileRef, "r"); //open file
    if(file != NULL){ //if file is valid...
        char fileCharacter = fgetc(file); //get the first character of the file
        while(fileCharacter != EOF){ //while character is not the last character of file...
            putchar(fileCharacter); //append character to stdout
            fileCharacter = fgetc(file); //get next character
        }
        fclose(file); //once file end has been reached, close file
    }else{ //if file is not valid...
        printf("wcat: cannot open file\n");
        exit(1);
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){ //if there are less than 2 arguments, exit program
        exit(0);
    }
    for(int i = 1; i < argc; i++){ //for each file...
        printFile(argv[i]);
    }
    exit(0);
}
