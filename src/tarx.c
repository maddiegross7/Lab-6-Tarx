//Lab 6: Tarx
//Madelyn Gross
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

//comparing for the jrb tree
int compareJvalLong(Jval a, Jval b) {
    if (a.l < b.l){ 
        return -1;
    }  
    if (a.l > b.l) {
        return 1;
    }
    return 0;
}

//similar file struct to the one in tarc and functions
typedef struct file{
    int nameSize;
    char *name;
    long iNodeValue;
    int fileMode;
    long secondsOfLastMod;
    long fileSize;
    int isNewInode;
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
    file->fileContents = NULL;

    return file;
}

void freeFileItem(File *file){
    if(file == NULL){
        return;
    }
    if(file->name){
        free(file->name);
    }
    if(file->fileContents){
        free(file->fileContents);
    }
    free(file);
}

int main()
{
    struct stat tarFileStats;

    if(fstat(fileno(stdin), &tarFileStats) != 0){
        fprintf(stderr, "file cannot be accessed");
        exit(1);
    }

    //reading the whole thing into a buffer
    char *buffer = malloc(tarFileStats.st_size);
    fread(buffer, tarFileStats.st_size, 1, stdin);

    char *bufferPtr = buffer;
    char *endPtr = buffer + tarFileStats.st_size;

    JRB iNodes = make_jrb();//storing new inodes
    Dllist files = new_dllist();//storing all files

    while (bufferPtr < endPtr) {
        File *thisFile = initializeFileItem();
        //reading initial things about all filetypes
        thisFile->nameSize = *(int *)bufferPtr;
        bufferPtr += sizeof(int);
        
        thisFile->name = malloc(thisFile->nameSize + 1);
        if (!thisFile->name) {
            fprintf(stderr, "Memory allocation failed for fileName\n");
            exit(1);
        }
        strncpy(thisFile->name, bufferPtr, thisFile->nameSize);
        thisFile->name[thisFile->nameSize] = '\0';
        bufferPtr += thisFile->nameSize;

        thisFile->iNodeValue = *(long *)bufferPtr;
        bufferPtr += sizeof(long);

        if(!(bufferPtr <= endPtr)){
            fprintf(stderr, "Bad tarc file for 045/LaUZ1kGyr-caEdYGB/H2c2k.  Couldn't read inode\n");
            exit(1);
        }
        //is it a new inode? we need to get mode and last modification time
        Jval iNodeValue = new_jval_l(thisFile->iNodeValue);
        if (jrb_find_gen(iNodes, iNodeValue, compareJvalLong) == 0) {
            thisFile->fileMode = *(int *)bufferPtr;
            bufferPtr += sizeof(int);

            if(!(bufferPtr <= endPtr)){
                fprintf(stderr, "Bad tarc file for 045/LaUZ1kGyr-caEdYGB/H2c2k.  Couldn't read inode\n");
                exit(1);
            }

            thisFile->secondsOfLastMod = *(long *)bufferPtr;
            bufferPtr += sizeof(long);
            //is it a file? get the size and the contents
            if (S_ISREG(thisFile->fileMode)) {
                thisFile->fileSize = *(long *)bufferPtr;
                bufferPtr += sizeof(long);

                thisFile->fileContents = malloc(thisFile->fileSize + 1);
                if (!thisFile->fileContents) {
                    fprintf(stderr, "Memory allocation failed for file contents\n");
                    exit(1);
                }

                if (bufferPtr + thisFile->fileSize > endPtr) {
                    fprintf(stderr, "Bad tarc file: Expected %ld bytes for file contents, but reached EOF\n", thisFile->fileSize);
                    exit(1);
                }
                memcpy(thisFile->fileContents, bufferPtr, thisFile->fileSize);
                thisFile->fileContents[thisFile->fileSize] = '\0';

                bufferPtr += thisFile->fileSize;
                int result = open(thisFile->name, O_CREAT | O_WRONLY, 0777);
                chmod(thisFile->name, thisFile->fileMode);

                write(result, thisFile->fileContents, thisFile->fileSize);
                close(result);
                //setting the modification times 
                struct timeval times[2];
                times[0].tv_sec = thisFile->secondsOfLastMod;
                times[0].tv_usec = 0; 
                times[1].tv_sec = thisFile->secondsOfLastMod; 
                times[1].tv_usec = 0; 

                utimes(thisFile->name, times);//need this!!

            }else if(S_ISDIR(thisFile->fileMode)){
                mkdir(thisFile->name, 0777);
            }
            //inserting the inode into the tree
            jrb_insert_gen(iNodes, iNodeValue, new_jval_v(thisFile), compareJvalLong);
            thisFile->isNewInode = 1;
        }
        dll_append(files, new_jval_v((void *)thisFile));
    }

    //this is linking the iNodes that are not the first ones to the ones that are otgether
    Dllist ptr;
    dll_traverse(ptr, files){
        File *file = (File *)jval_v(ptr->val);
        if(file->isNewInode == 0){
            File *temp = (File *)jrb_find_gen(iNodes, new_jval_l(file->iNodeValue), compareJvalLong)->val.v;
            
            if(link(temp->name, file->name) == -1){
                printf(strerror(errno));
            }
        }
    }
    //setting the modifcation tiems to the directories
    //and then freeing the files
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
        freeFileItem(file);
    }
    //freeing the rest of the stuff
    free_dllist(files);
    jrb_free_tree(iNodes);
    free(buffer);
    return 0;
}