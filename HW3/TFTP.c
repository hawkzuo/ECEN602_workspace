//
// Created by Jianyu Zuo on 10/27/17.
//
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "TFTP.h"

int generateErrorMessage(char buffer[], char* errorMessage, int errorCode) {
   buffer[0]=(char)0;
   buffer[1]=(char)5;
   buffer[2]=(char)errorCode >> 8;
   buffer[3] = (char)errorCode & (uint8_t)0xFF;
   for(int i=0; i<strlen(errorMessage); i++) {
        buffer[4+i] = errorMessage[i];
   }
   return 0;

}

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
//        *ptr++ = (char)0;
        for(int i=0;i<strlen(mode);i++) {
            *ptr++ = *(mode+i);
        }
//        *ptr = (char)0;

        return (int) totalLen;


    } else {
        return -1;
    }
}

int parseRRQ(char** filename, char** mode, char buffer[], ssize_t dataSize)
{
    if(dataSize > 9 && buffer[0] == (char)0 && buffer[1] == (char)RRQ) {
        *mode = malloc(6*sizeof(char));
        strncpy(*mode, buffer+dataSize-6, 6);
        if(strcmp(*mode, OCTET) == 0) {
            // mode is correct
            *filename = malloc((dataSize-8)*sizeof(char));
            strncpy(*filename, buffer+2, dataSize-8);
        } else {
            *mode = malloc(9*sizeof(char));
            strncpy(*mode, buffer+dataSize-9, 9);
            if(strcmp(*mode, NETASCII) == 0) {
                // mode is correct
                *filename = malloc((dataSize-11)*sizeof(char));
                strncpy(*filename, buffer+2, dataSize-11);
            } else {
                return -1;
            }
        }
        return 0;
    } else {
        // Not an Acceptable RRQ
        return -1;
    }
}

int parseWRQ(char** filename, char** mode, char buffer[], ssize_t dataSize)
{
    if(dataSize > 9 && buffer[0] == (char)0 && buffer[1] == (char)WRQ) {
        *mode = malloc(6*sizeof(char));
        strncpy(*mode, buffer+dataSize-6, 6);
        if(strcmp(*mode, OCTET) == 0) {
            // mode is correct
            *filename = malloc((dataSize-8)*sizeof(char));
            strncpy(*filename, buffer+2, dataSize-8);
        } else {
            *mode = malloc(9*sizeof(char));
            strncpy(*mode, buffer+dataSize-9, 9);
            if(strcmp(*mode, NETASCII) == 0) {
                // mode is correct
                *filename = malloc((dataSize-11)*sizeof(char));
                strncpy(*filename, buffer+2, dataSize-11);
            } else {
                return -1;
            }
        }
        return 0;
    } else {
        // Not an Acceptable RRQ
        return -1;
    }
}



int generateDATA(char dataMsg[], const char fileBuffer[], uint16_t seqNum, ssize_t file_read_count)
{
    dataMsg[0] = (char)0;
    dataMsg[1] = (uint8_t)DATA;
    dataMsg[2] = (uint8_t)(seqNum >> 8);
    dataMsg[3] = (uint8_t)seqNum & (uint8_t)0xFF;
    for(int i=0; i<file_read_count; i++) {
        dataMsg[4+i] = fileBuffer[i];
    }
    return 0;
}

int parseDATA(char** message, uint16_t* seqNum, const char buffer[], ssize_t dataSize)
{
    if(dataSize >= 4 && buffer[0] == (uint8_t)0 && buffer[1] == (uint8_t)DATA) {
        *message = malloc((dataSize-4)*sizeof(char));
        //printf("buffer[2]:\n %s \n", byte_to_binary(buffer[2]));
        //printf("buffer[3]:\n %s \n", byte_to_binary(buffer[3]));
        *seqNum = (uint16_t) (((uint16_t)buffer[2] * 256) + (uint8_t)buffer[3]);
        // TODO: HERE must manually copy all the bytes, strncpy will not copy eacatly same characters
        // strncpy(*message, buffer+4, dataSize-4);
        for(int i=4;i<dataSize;i++) {
            *(*message+i-4) = buffer[i];
        }
        return 0;
    } else {
        return -1;
    }
}

int generateACK(char ackMsg[], uint16_t seqNum)
{
    ackMsg[0] = (uint8_t)0;
    ackMsg[1] = (uint8_t)ACK;
    ackMsg[2] = (uint8_t)(seqNum >> 8);
    ackMsg[3] = (uint8_t)(seqNum & 0xFF);
    return 0;
}

int parseACK(uint16_t* seqNum, const char buffer[], ssize_t dataSize)
{
    if(dataSize == 4 && buffer[0] == (uint8_t)0 && buffer[1] == (uint8_t)ACK ) {
        *seqNum = (uint16_t) (((uint16_t)buffer[2] * 256) + (uint8_t)buffer[3]);
//        *seqNum = ((uint16_t)buffer[2] << 8) + (uint16_t)buffer[3];
        return 0;
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
