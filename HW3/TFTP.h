//
// Created by Jianyu Zuo on 10/27/17.
//

#ifndef ECEN602_WORKSPACE_TFTP_H
#define ECEN602_WORKSPACE_TFTP_H


#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4

#define MAXSENDBUFLEN 512

static char* OCTET = "octet";
static char* NETASCII = "netascii";

int generateRRQ(char** rrqMsg, const char* filename, const char* mode);
int parseRRQ(char** filename, char** mode, char buffer[], ssize_t dataSize);
int parseWRQ(char** filename, char** mode, char buffer[], ssize_t dataSize);
int generateDATA(char dataMsg[], const char fileBuffer[], uint16_t seqNum, ssize_t file_read_count);
int parseDATA(char** message, uint16_t* seqNum, const char buffer[], ssize_t dataSize);
int generateACK(char ackMsg[], uint16_t seqNum);
int parseACK(uint16_t* seqNum, const char buffer[], ssize_t dataSize);



const char *byte_to_binary(int x);


#endif //ECEN602_WORKSPACE_TFTP_H
