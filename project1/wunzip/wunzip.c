#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void unzipFile(FILE *zipFile){
    uint8_t buffer[5]; //set aside 5 bytes of memory
    while(fread(buffer, sizeof(uint8_t), 5, zipFile) == 5){ //grab next 5 bytes of binary in file
        uint32_t count = 0; //set aside memory for a 32 bit int
        for(int i = 0; i < 4; i++){ 
            count = count | buffer[i] << (i * 8); //insert bytes into count memory space
        }
        unsigned char currentCharacter = buffer[4]; //grab last byte and insert that as character
        for(uint32_t i = 0; i < count; i++){
            putchar(currentCharacter); //print character as for amount of count
        }
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){ //if enough arguments
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }
    for(int i = 1; i < argc; i++){
        FILE *curFile = fopen(argv[i], "rb"); //open ith file and reading the binary
        if(curFile == NULL){
            printf("error opening");
            exit(1);
        }
        unzipFile(curFile);
        fclose(curFile);
    }
    
    exit(0);
}
