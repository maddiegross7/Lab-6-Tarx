#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "dllist.h"
#include "jval.h"
#include "jrb.h"

int compareJvalLong(Jval a, Jval b) {
    if (a.l < b.l){ 
        return -1;
    }  
    if (a.l > b.l) {
        return 1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Usage: ./bin/tarc <tarfile name>");
        exit(1);
    }

    FILE *tarFile = fopen(argv[1], "rb");

    if(tarFile == NULL){
        fprintf(stderr, "Error opening file: %s", argv[1]);
    }
    
    struct stat tarFileStats;

    if(stat(argv[1], &tarFileStats) != 0){
        fprintf(stderr, "%s cannot be accessed", argv[1]);
        exit(1);
    }

    char *buffer = malloc(tarFileStats.st_size);
    fread(buffer, tarFileStats.st_size, 1, tarFile);
    fclose(tarFile);

    char *bufferPtr = buffer;
    char *endPtr = buffer + tarFileStats.st_size;

    JRB iNodes = make_jrb();

    while(bufferPtr < endPtr){
        int fileSize = *(int *)bufferPtr;
        printf("FileName Size: %i\n", fileSize);

        bufferPtr += sizeof(int);

        char *fileName = malloc(fileSize + 1);
        strncpy(fileName, bufferPtr, fileSize);
        fileName[fileSize + 1] = '\0';

        printf("FileName: %s\n", fileName);
        bufferPtr += fileSize;

        long iNode = *(long *)bufferPtr;
        printf("iNode: %li\n", iNode);
        bufferPtr += sizeof(long);  

        Jval iNodeValue = new_jval_l(iNode);
        if(jrb_find_gen(iNodes, iNodeValue, compareJvalLong)==0){
            printf("new Inode\n");
            jrb_insert_gen(iNodes, iNodeValue, new_jval_l(iNode), compareJvalLong);

            int fileMode = *(int *)bufferPtr;
            bufferPtr += sizeof(int);

            long secondsOfLastMod = *(long *)bufferPtr;
            bufferPtr += sizeof(long);

            if(S_ISREG(fileMode)){
                printf("this is a file\n");
                
                long fileSize = *(long *)bufferPtr;
                bufferPtr += sizeof(long);

                char *fileContents = malloc(fileSize + 1);
                strncpy(fileContents, bufferPtr, fileSize);
                fileContents[fileSize + 1] = '\0';
                bufferPtr += fileSize;
                printf("we have read the file\n");
            }
        }

    }
    

    return 0;
}