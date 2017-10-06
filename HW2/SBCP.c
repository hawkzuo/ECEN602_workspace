//
// Created by Jianyu Zuo on 10/5/17.
//
#include <stdlib.h>
#include <memory.h>
#include "SBCP.h"

// Reason why attr_len is the input is because outer program will need this value

// Build an attribute of type ATTRUSERNAME
struct SBCPAttribute * buildNameAttr(char* username, u_int16_t name_attr_length)
{
    struct SBCPAttribute *nameAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    nameAttr->attr_payload = malloc((strlen(username)+1)*sizeof(char));

    strcpy(nameAttr->attr_payload, username);

    nameAttr->attr_header[0] = 0;
    nameAttr->attr_header[1] = (u_int8_t) ATTRUSERNAME;
    nameAttr->attr_header[2] = (u_int8_t) (name_attr_length >> 8);
    nameAttr->attr_header[3] = (u_int8_t) (name_attr_length & 0xFF);

    return nameAttr;
}

// Build an attribute of type ATTRCOUNT
struct SBCPAttribute * buildCountAttr(u_int16_t client_count)
{
    struct SBCPAttribute *countAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    countAttr->attr_payload = malloc(2*sizeof(char));
    countAttr->attr_payload[0] = (char) (client_count >> 8);
    countAttr->attr_payload[1] = (char) (client_count & 0xFF);

    countAttr->attr_header[0] = 0;
    countAttr->attr_header[1] = (u_int8_t) ATTRCOUNT;
    countAttr->attr_header[2] = (u_int8_t) (6 >> 8);
    countAttr->attr_header[3] = (u_int8_t) (6 & 0xFF);

    return countAttr;
}

// Build an attribute of type ATTRREASON
struct SBCPAttribute * buildReasonAttr(char* reason, u_int16_t reason_attr_length)
{
    struct SBCPAttribute *reasonAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    reasonAttr->attr_payload = malloc((strlen(reason)+1)*sizeof(char));

    strcpy(reasonAttr->attr_payload, reason);

    reasonAttr->attr_header[0] = 0;
    reasonAttr->attr_header[1] = (u_int8_t) ATTRREASON;
    reasonAttr->attr_header[2] = (u_int8_t) (reason_attr_length >> 8);
    reasonAttr->attr_header[3] = (u_int8_t) (reason_attr_length & 0xFF);

    return reasonAttr;
}

// Build an attribute of type ATTRMESSAGE
struct SBCPAttribute * buildMessageAttr(char *message, u_int16_t message_attr_length)
{
    struct SBCPAttribute *messageAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    messageAttr->attr_payload = malloc((strlen(message)+1)*sizeof(char));

    strcpy(messageAttr->attr_payload, message);

    messageAttr->attr_header[0] = 0;
    messageAttr->attr_header[1] = (u_int8_t) ATTRMESSAGE;
    messageAttr->attr_header[2] = (u_int8_t) (message_attr_length >> 8);
    messageAttr->attr_header[3] = (u_int8_t) (message_attr_length & 0xFF);

    return messageAttr;
}



