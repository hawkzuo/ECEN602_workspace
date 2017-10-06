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

int generateNAK(struct SBCPMessage *msg, char* reason)
{
    u_int16_t reason_attr_length = (u_int16_t) (1 + strlen(reason) + 4);
    struct SBCPAttribute *reasonAttr = buildReasonAttr(reason, reason_attr_length);
    msg->payload[0] = *reasonAttr;
    free(reasonAttr);
    u_int16_t total_Bytes = (u_int16_t) (4 + reason_attr_length);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (u_int8_t)(NAK | 0x80);
    msg->header[2] = (u_int8_t)(total_Bytes >> 8);
    msg->header[3] = (u_int8_t)(total_Bytes & 0xFF);

    return total_Bytes;
}

int generateACK(struct SBCPMessage *msg, char* usernames[MAXUSERCOUNT])
{

    u_int16_t counter = 0;
    int total_Bytes = 4;
    for(int i=0; i< MAXUSERCOUNT; i++) {

        char* stepUsername = usernames[i];
//        printf("%s\n", stepUsername);
        if(stepUsername == NULL) {
            counter = (u_int16_t) i;
            break;
        }
        // Create a username Attr.
        u_int16_t name_attr_length = (u_int16_t) (1 + strlen(stepUsername) + 4);
        struct SBCPAttribute *nameAttr = buildNameAttr(stepUsername, name_attr_length);
        // Assign to the msg Structure
        msg->payload[i+1] = *nameAttr;
        total_Bytes += name_attr_length;
        free(nameAttr);

    }

    // Create the client count Attribute
    u_int16_t count_attr_length = (u_int16_t) (2 + 4);
    struct SBCPAttribute *countAttr = buildCountAttr(counter);
    msg->payload[0] = *countAttr;
    free(countAttr);

    // Finally create the Message
    total_Bytes += count_attr_length;
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (u_int8_t)(ACK | 0x80);
    msg->header[2] = (u_int8_t)(total_Bytes >> 8);
    msg->header[3] = (u_int8_t)(total_Bytes & 0xFF);

    return total_Bytes;
}

int generateJOIN(struct SBCPMessage *msg, char *username)
{

    // Generate JOIN Message
    // Required: 'username' field
    // Setup SBCP message attribute
    u_int16_t name_attr_length = (u_int16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);

    // Setup SBCP message
    msg->payload[0] = *nameAttr;
    u_int16_t msg_length = (u_int16_t) (name_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (u_int8_t)(JOIN | 0x80);
    msg->header[2] = (u_int8_t)(msg_length >> 8);
    msg->header[3] = (u_int8_t)(msg_length & 0xFF);


//    printf("msg length : %hu\n", msg_length);
//    printf("Message Header 0: %s\n", byte_to_binary(msg->header[0]));
//    printf("Message Header 1: %s\n", byte_to_binary(msg->header[1]));
//    printf("Message Header 2: %s\n", byte_to_binary(msg->header[2]));
//    printf("Message Header 3: %s\n", byte_to_binary(msg->header[3]));

    return msg_length;

}

int generateSEND(struct SBCPMessage *msg, char *messages)
{
    u_int16_t message_attr_length = (u_int16_t) (1+strlen(messages) + 4);
    struct SBCPAttribute *messageAttr = buildMessageAttr(messages, message_attr_length);

    msg->payload[0] = *messageAttr;
    u_int16_t msg_length = (u_int16_t) (message_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (u_int8_t)(SEND | 0x80);
    msg->header[2] = (u_int8_t)(msg_length >> 8);
    msg->header[3] = (u_int8_t)(msg_length & 0xFF);

    return msg_length;
}


int generateFWD(struct SBCPMessage *msg) {

    return 0;
}



