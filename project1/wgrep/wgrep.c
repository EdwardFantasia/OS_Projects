#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
    if(argc < 2){ //if not enough arguments...
        printf("wgrep: searchterm [file ...]\n");
        exit(1); //exit
    }
    char *line = NULL; //setup char pointer
    size_t bufferLen = 0; //setup buffer for line input
    if(argc == 2){ //if reading from stdin
        while(getline(&line, &bufferLen, stdin) != -1){ //get line from stdin and keep going if it's still a valid line
            if(strstr(line, argv[1]) != NULL){ //if there is a case sensitive match, print line
                printf("%s", line);
            }
        }
    }else{
        for(int i = 2; i < argc; i++){
            FILE *file = fopen(argv[i], "r");
            if(file == NULL){
                printf("wgrep: cannot open file\n");
                exit(1);
            }

            while(getline(&line, &bufferLen, file) != -1){ //get line from file if it's still a valid line
                if(strstr(line, argv[1]) != NULL){ //if there is a case sensitive match, print line
                    printf("%s", line);
                }
            }
            fclose(file);
            free(line);
        }
    }

    exit(0);
}
