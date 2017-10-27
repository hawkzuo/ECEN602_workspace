//
// Created by Jianyu Zuo on 10/27/17.
//
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "TFTP.h"

int generateRRQ(char** rrqMsg, const char* filename, const char* mode)
{

    if(strcmp(mode, OCTET) == 0) {
        size_t totalLen = strlen(OCTET) + strlen(filename) + 4 + 2;
        *rrqMsg = malloc(totalLen*sizeof(char));
        char * ptr = *rrqMsg;
//        int buffer_index = 0;

        *ptr++ = (char)0;
        *ptr++ = (char)RRQ;
        for(int i=0;i<=strlen(filename);i++) {
            *ptr++ = *(filename+i);
        }
        *ptr++ = (char)0;
        for(int i=0;i<strlen(mode);i++) {
            *ptr++ = *(mode+i);
        }
        *ptr++ = (char)0;

        return (int) totalLen;


    } else if(strcmp(mode, NETASCII) == 0) {
        size_t totalLen = strlen(NETASCII) + strlen(filename) + 4 + 2;
        *rrqMsg = malloc(totalLen*sizeof(char));
        int buffer_index = 0;

        *rrqMsg[buffer_index++] = (char)0;
        *rrqMsg[buffer_index++] = (char)RRQ;
        for(int i=0;i<strlen(filename);i++) {
            *rrqMsg[buffer_index++] = *(filename+i);
        }
        *rrqMsg[buffer_index++] = (char)0;
        for(int i=0;i<strlen(mode);i++) {
            *rrqMsg[buffer_index++] = *(mode+i);
        }
        *rrqMsg[buffer_index++] = (char)0;

        if(buffer_index == totalLen) {
            return buffer_index;
        } else {
            return -1;
        }

    } else {
        return -1;
    }
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
