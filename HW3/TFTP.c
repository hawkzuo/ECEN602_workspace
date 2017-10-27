//
// Created by Jianyu Zuo on 10/27/17.
//
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "TFTP.h"

int generateRRQ(char** rrqMsg, const char* filename, const char* mode)
{
    if(strcmp(mode, OCTET) == 0 || strcmp(mode, NETASCII) == 0) {
        size_t totalLen = strlen(filename) + 1 + 6 + 4;
        *rrqMsg = malloc(totalLen*sizeof(char));
        char * ptr = *rrqMsg;

        *ptr++ = (char)0;
        *ptr++ = (char)RRQ;
        for(int i=0;i<=strlen(filename);i++) {
            *ptr++ = *(filename+i);
        }
        *ptr++ = (char)0;
        for(int i=0;i<strlen(mode);i++) {
            *ptr++ = *(mode+i);
        }
        *ptr = (char)0;

        return (int) totalLen;


    } else {
        return -1;
    }
}

int parseRRQ(char** filename, char** mode, char buffer[], ssize_t dataSize)
{
    if(dataSize > 10 && buffer[0] == (char)0 && buffer[1] == (char)RRQ) {
        *filename = malloc((dataSize-10)*sizeof(char));
        *mode = malloc(6*sizeof(char));
        strncpy(*filename, buffer+2, dataSize-10);
        strncpy(*mode, buffer+dataSize-7, 6);
        return 0;
    } else {
        return -1;
    }
}

int generateDATA()
{

}




/* Binary View Helper */
const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
