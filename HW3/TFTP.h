//
// Created by Jianyu Zuo on 10/27/17.
//

#ifndef ECEN602_WORKSPACE_TFTP_H
#define ECEN602_WORKSPACE_TFTP_H


#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4

static char* OCTET = "octet";
static char* NETASCII = "netascii";

int generateRRQ(char** rrqMsg, const char* filename, const char* mode);




const char *byte_to_binary(int x);


#endif //ECEN602_WORKSPACE_TFTP_H
