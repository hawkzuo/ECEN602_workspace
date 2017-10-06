//
// Created by Jianyu Zuo on 10/5/17.
//

#include <stdint.h>

#ifndef ECEN602_WORKSPACE_SBCP_H
#define ECEN602_WORKSPACE_SBCP_H


static uint8_t PROTOCOLVERSION=3;
static uint8_t JOIN=2;
static uint8_t SEND=4;
static uint8_t FWD=3;
static uint8_t ATTRREASON=1;
static uint8_t ATTRUSERNAME=2;
static uint8_t ATTRCOUNT=3;
static uint8_t ATTRMESSAGE=4;
// Bonus Parts:
static uint8_t ACK=7;
static uint8_t NAK=5;
static uint8_t ONLINE=8;
static uint8_t OFFLINE=6;
static uint8_t IDLE=9;

// Maximum supported User Number
static int MAXUSERCOUNT = 100;

struct SBCPAttribute {
    unsigned char attr_header[4];
    // Can init without given size
    char* attr_payload;
//    struct SBCPAttribute * next;
};

struct SBCPMessage {
    unsigned char header[4];
    struct SBCPAttribute payload[100];
};


struct SBCPAttribute * buildNameAttr(char* username, uint16_t name_attr_length);

struct SBCPAttribute * buildCountAttr(uint16_t client_count);

struct SBCPAttribute * buildReasonAttr(char* reason, uint16_t reason_attr_length);

struct SBCPAttribute * buildMessageAttr(char *message, uint16_t message_attr_length);

// These generate different types of SBCPATTR, the return value is the bytes of each type of attribute
int generateNAK(struct SBCPMessage *msg, char* reason);

int generateACK(struct SBCPMessage *msg, char* usernames[MAXUSERCOUNT]);

int generateJOIN(struct SBCPMessage *msg, char *username);

int generateSEND(struct SBCPMessage *msg, char *messages);

int generateFWD(struct SBCPMessage *msg);





#endif //ECEN602_WORKSPACE_SBCP_H


