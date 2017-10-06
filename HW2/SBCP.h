//
// Created by Jianyu Zuo on 10/5/17.
//

#include <ntsid.h>

#ifndef ECEN602_WORKSPACE_SBCP_H
#define ECEN602_WORKSPACE_SBCP_H

#endif //ECEN602_WORKSPACE_SBCP_H

u_int8_t PROTOCOLVERSION=3;
u_int8_t JOIN=2;
u_int8_t SEND=4;
u_int8_t FWD=3;
u_int8_t ATTRREASON=1;
u_int8_t ATTRUSERNAME=2;
u_int8_t ATTRCOUNT=3;
u_int8_t ATTRMESSAGE=4;


struct SBCPAttribute {
    unsigned char attr_header[4];
    // Can init without given size
    char* attr_payload;
//    struct SBCPAttribute * next;
};

struct SBCPMessage {
    unsigned char header[4];
    struct SBCPAttribute payload[5];
};