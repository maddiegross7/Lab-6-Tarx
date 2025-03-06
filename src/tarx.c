#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include "fields.h"
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

typedef struct file{
    int nameSize;
    char *name;
    long iNodeValue;
    int fileMode;
    long secondsOfLastMod;
    long fileSize;
    int isNewInode;
    char *relativePath;//because this is annoying
    char *fileContents;
} File;



File* initializeFileItem(){
    File *file = malloc(sizeof(File));

    file->nameSize = 0;
    file->name = NULL;
    file->iNodeValue = 0;
    file->fileMode = 0;
    file->secondsOfLastMod = 0;
    file->fileSize = 0;
    file->isNewInode = 0;
    file->relativePath = NULL;
    file->fileContents = NULL;

    return file;
}

void printFileContents(File *thisFile) {
    if (!thisFile || !thisFile->name) {
        fprintf(stderr, "Invalid file structure\n");
        return;
    }

    printf("\n--- File Info ---\n");
    printf("File Name: %s\n", thisFile->name);
    printf("iNode: %ld\n", thisFile->iNodeValue);
    printf("File Mode: %o\n", thisFile->fileMode);  // Print mode in octal
    printf("Last Modified: %ld\n", thisFile->secondsOfLastMod);
    printf("File Size: %ld bytes\n", thisFile->fileSize);

    if (S_ISREG(thisFile->fileMode)) {  // Check if it's a regular file
        printf("\n--- File Contents ---\n");
        printf("hello");
        if (thisFile->fileSize > 0) {
            printf("file Contents: %s\n", thisFile->fileContents);
        } else {
            printf("[Empty File]\n");
        }
    } else if (S_ISDIR(thisFile->fileMode)) {
        printf("\n[This is a directory]\n");
    } else {
        printf("\n[Unknown file type]\n");
    }
}


int main(int argc, char const *argv[])
{

    struct stat tarFileStats;

    if(fstat(fileno(stdin), &tarFileStats) != 0){
        fprintf(stderr, "file cannot be accessed");
        exit(1);
    }

    char *buffer = malloc(tarFileStats.st_size);
    fread(buffer, tarFileStats.st_size, 1, stdin);

    char *bufferPtr = buffer;
    char *endPtr = buffer + tarFileStats.st_size;

    JRB iNodes = make_jrb();
    Dllist files = new_dllist();

    while (bufferPtr < endPtr) {
        File *thisFile = initializeFileItem();

        // Read nameSize
        thisFile->nameSize = *(int *)bufferPtr;
        bufferPtr += sizeof(int);
        //printf("FileName Size: %i\n", thisFile->nameSize);

        // Read fileName
        thisFile->name = malloc(thisFile->nameSize + 1);
        if (!thisFile->name) {
            fprintf(stderr, "Memory allocation failed for fileName\n");
            exit(1);
        }
        strncpy(thisFile->name, bufferPtr, thisFile->nameSize);
        thisFile->name[thisFile->nameSize] = '\0';
        bufferPtr += thisFile->nameSize;

        //printf("FileName: %s\n", thisFile->name);

        // Read iNodeValue
        thisFile->iNodeValue = *(long *)bufferPtr;
        bufferPtr += sizeof(long);

        if(!(bufferPtr <= endPtr)){
            fprintf(stderr, "Bad tarc file for 045/LaUZ1kGyr-caEdYGB/H2c2k.  Couldn't read inode\n");
            exit(1);
        }
        //printf("iNode: %li\n", thisFile->iNodeValue);

        Jval iNodeValue = new_jval_l(thisFile->iNodeValue);
        if (jrb_find_gen(iNodes, iNodeValue, compareJvalLong) == 0) {
            //printf("New Inode\n");
            

            thisFile->fileMode = *(int *)bufferPtr;
            bufferPtr += sizeof(int);

            thisFile->secondsOfLastMod = *(long *)bufferPtr;
            bufferPtr += sizeof(long);

            if (S_ISREG(thisFile->fileMode)) {
                //printf("This is a file\n");

                // Read fileSize
                thisFile->fileSize = *(long *)bufferPtr;
                bufferPtr += sizeof(long);

                // Read file contents (optional)
                thisFile->fileContents = malloc(thisFile->fileSize + 1);
                if (!thisFile->fileContents) {
                    fprintf(stderr, "Memory allocation failed for file contents\n");
                    exit(1);
                }
                memcpy(thisFile->fileContents, bufferPtr, thisFile->fileSize);
                thisFile->fileContents[thisFile->fileSize] = '\0';

                bufferPtr += thisFile->fileSize;

                //printf("File contents: %s\n", thisFile->fileContents);
                //printf("We have read the file\n");

                //printf("File Mode: %ul\n", thisFile->fileMode);

                int result = open(thisFile->name, O_CREAT | O_WRONLY, 0777);

                
                // struct stat resultStats;
                // fstat(result, &resultStats);
                // printf("result mode: %ul\n", resultStats.st_mode);
                chmod(thisFile->name, thisFile->fileMode);

                write(result, thisFile->fileContents, thisFile->fileSize);
                
                close(result);

                struct timeval times[2];
                times[0].tv_sec = thisFile->secondsOfLastMod;
                times[0].tv_usec = 0; 
                times[1].tv_sec = thisFile->secondsOfLastMod; 
                times[1].tv_usec = 0; 

                utimes(thisFile->name, times);

            }else if(S_ISDIR(thisFile->fileMode)){
                mkdir(thisFile->name, 0777);
            }

            jrb_insert_gen(iNodes, iNodeValue, new_jval_v(thisFile), compareJvalLong);
            thisFile->isNewInode = 1;
        }
        dll_append(files, new_jval_v((void *)thisFile));
        //printFileContents(thisFile);
    }

    Dllist ptr;
    dll_traverse(ptr, files){
        File *file = (File *)jval_v(ptr->val);
        if(file->isNewInode == 0){
            //printf("We have an Inode we have already seen\n");
            // int result = open(file->name, O_CREAT | O_WRONLY, 0777);
            // close(result);

            File *temp = (File *)jrb_find_gen(iNodes, new_jval_l(file->iNodeValue), compareJvalLong)->val.v;
            //printf("temp: %s, file: %s\n", temp->name, file->name);
            if(link(temp->name, file->name) == -1){
                printf(strerror(errno));
            }
        }
    }

    dll_traverse(ptr, files){
        File *file = (File *)jval_v(ptr->val);
        if(S_ISDIR(file->fileMode) && file->isNewInode == 1){
            chmod(file->name, file->fileMode);
            struct timeval times[2];
            times[0].tv_sec = file->secondsOfLastMod;
            times[0].tv_usec = 0; 
            times[1].tv_sec = file->secondsOfLastMod; 
            times[1].tv_usec = 0; 

            utimes(file->name, times);
        }

    }

    return 0;
}