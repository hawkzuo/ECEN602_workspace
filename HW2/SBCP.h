//
// Created by Jianyu Zuo on 10/5/17.
//

#include <stdint.h>

#ifndef ECEN602_WORKSPACE_SBCP_H
#define ECEN602_WORKSPACE_SBCP_H


static uint8_t PROTOCOLVERSION=3;
//static uint8_t JOIN=2;
//static uint8_t SEND=4;
//static uint8_t FWD=3;
static uint8_t ATTRREASON=1;
static uint8_t ATTRUSERNAME=2;
static uint8_t ATTRCOUNT=3;
static uint8_t ATTRMESSAGE=4;
// Max Bytes Constraint for each Attr.
static int ATTRREASONMAX=32;
static int ATTRUSERNAMEMAX=16;
static int ATTRMESSAGEMAX=512;

// Bonus Parts:
#define JOIN 2
#define SEND 4
#define FWD 3
#define ACK 7
#define NAK 5
#define ONLINE 8
#define OFFLINE 6
#define IDLE 9
//static uint8_t ACK=7;
//static uint8_t NAK=5;
//static uint8_t ONLINE=8;
//static uint8_t OFFLINE=6;
//static uint8_t IDLE=9;

// Maximum supported User Number
#define MAXUSERCOUNT 101

struct SBCPAttribute {
    unsigned char attr_header[4];
    // Can init without given size
    char* attr_payload;
//    struct SBCPAttribute * next;
};

struct SBCPMessage {
    unsigned char header[4];
    struct SBCPAttribute payload[MAXUSERCOUNT];
};


struct SBCPAttribute * buildNameAttr(char* username, uint16_t name_attr_length);

struct SBCPAttribute * buildCountAttr(uint16_t client_count);

struct SBCPAttribute * buildReasonAttr(char* reason, uint16_t reason_attr_length);

struct SBCPAttribute * buildMessageAttr(char *message, uint16_t message_attr_length);

// These generate different types of SBCPATTR, the return value is the bytes of each type of attribute
int generateCLIENTIDLE(struct SBCPMessage *msg);

int generateSERVERIDLE(struct SBCPMessage *msg, char *username);

int generateNAK(struct SBCPMessage *msg, char* reason);

int generateJOIN(struct SBCPMessage *msg, char *username);

int generateSEND(struct SBCPMessage *msg, char *messages);

int generateONLINE(struct SBCPMessage *msg, char *username);

int generateOFFLINE(struct SBCPMessage *msg, char *username);

int generateFWD(struct SBCPMessage *msg, char *username, char *messages);

int generateACK(struct SBCPMessage *msg, char* usernames[MAXUSERCOUNT]);


// Generate the raw data into buffer Given A SBCPMessage
int createRawData(char buffer[], struct SBCPMessage *message, int msg_length);


// Parse Data
int parseSERVERIDLE(char buffer[], char **username);
int parseNAK(char buffer[], char **reason);
int parseJOIN(char buffer[], char **username);
int parseSEND(char buffer[], char **messages);
int parseONLINE(char buffer[], char **username);
int parseOFFLINE(char buffer[], char **username);
int parseFWD(char buffer[], char **message, char **username);
int parseACK(char buffer[], int * client_count, char* usernames[MAXUSERCOUNT]);


#endif //ECEN602_WORKSPACE_SBCP_H


