//
// Created by Jianyu Zuo on 10/5/17.
//
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "SBCP.h"

// Reason why attr_len is the input is because outer program will need this value

// Build an attribute of type ATTRUSERNAME
struct SBCPAttribute * buildNameAttr(char* username, uint16_t name_attr_length)
{
    struct SBCPAttribute *nameAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    nameAttr->attr_payload = malloc((strlen(username)+1)*sizeof(char));

    strcpy(nameAttr->attr_payload, username);

    nameAttr->attr_header[0] = 0;
    nameAttr->attr_header[1] = (uint8_t) ATTRUSERNAME;
    nameAttr->attr_header[2] = (uint8_t) (name_attr_length >> 8);
    nameAttr->attr_header[3] = (uint8_t) (name_attr_length & 0xFF);

    return nameAttr;
}

// Returns the next available index of buffer
int readNameAttr(const char buffer[], int begin, char** username)
{
    if((uint8_t)buffer[begin] != 0 || (uint8_t)buffer[begin+1] != (uint8_t)ATTRUSERNAME) {
        return -1;
    }
    int name_attr_length = (uint8_t)buffer[begin+2]*256 + (uint8_t)buffer[begin+3];
    *username = malloc((name_attr_length-4)*sizeof(char));
    strncpy(*username, buffer+begin+4, name_attr_length-4);
    return begin+name_attr_length;
}

// Build an attribute of type ATTRCOUNT
struct SBCPAttribute * buildCountAttr(uint16_t client_count)
{
    struct SBCPAttribute *countAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    countAttr->attr_payload = malloc(2*sizeof(char));
    countAttr->attr_payload[0] = (char) (client_count >> 8);
    countAttr->attr_payload[1] = (char) (client_count & 0xFF);

    countAttr->attr_header[0] = 0;
    countAttr->attr_header[1] = (uint8_t) ATTRCOUNT;
    countAttr->attr_header[2] = (uint8_t) (6 >> 8);
    countAttr->attr_header[3] = (uint8_t) (6 & 0xFF);

    return countAttr;
}

// Returns the next available index of buffer
int readCountAttr(const char buffer[], int begin, int *count)
{
    if((uint8_t)buffer[begin] != 0 || (uint8_t)buffer[begin+1] != (uint8_t)ATTRCOUNT) {
        return -1;
    }
    *count = (uint8_t)buffer[begin+4] * 256 + (uint8_t)buffer[begin+5];

    return begin+6;
}

// Build an attribute of type ATTRREASON
struct SBCPAttribute * buildReasonAttr(char* reason, uint16_t reason_attr_length)
{
    struct SBCPAttribute *reasonAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    reasonAttr->attr_payload = malloc((strlen(reason)+1)*sizeof(char));
    
    strcpy(reasonAttr->attr_payload, reason);

    reasonAttr->attr_header[0] = 0;
    reasonAttr->attr_header[1] = (uint8_t) ATTRREASON;
    reasonAttr->attr_header[2] = (uint8_t) (reason_attr_length >> 8);
    reasonAttr->attr_header[3] = (uint8_t) (reason_attr_length & 0xFF);

    return reasonAttr;
}

// Returns the next available index of buffer
int readReasonAttr(const char buffer[], int begin, char** reason)
{
    if((uint8_t)buffer[begin] != 0 || (uint8_t)buffer[begin+1] != (uint8_t)ATTRREASON) {
        return -1;
    }
    int reason_attr_length = (uint8_t)buffer[begin+2]*256 + (uint8_t)buffer[begin+3];
    *reason = malloc((reason_attr_length-4)*sizeof(char));
    strncpy(*reason, buffer+begin+4, reason_attr_length-4);
    return begin+reason_attr_length;
}

// Build an attribute of type ATTRMESSAGE
struct SBCPAttribute * buildMessageAttr(char *message, uint16_t message_attr_length)
{
    struct SBCPAttribute *messageAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));
    messageAttr->attr_payload = malloc((strlen(message)+1)*sizeof(char));

    strcpy(messageAttr->attr_payload, message);

    messageAttr->attr_header[0] = 0;
    messageAttr->attr_header[1] = (uint8_t) ATTRMESSAGE;
    messageAttr->attr_header[2] = (uint8_t) (message_attr_length >> 8);
    messageAttr->attr_header[3] = (uint8_t) (message_attr_length & 0xFF);

    return messageAttr;
}

// Returns the next available index of buffer
int readMessageAttr(const char buffer[], int begin, char** message)
{
    if((uint8_t)buffer[begin] != 0 || (uint8_t)buffer[begin+1] != (uint8_t)ATTRMESSAGE) {
        return -1;
    }
    int message_attr_length = (uint8_t)buffer[begin+2]*256 + (uint8_t)buffer[begin+3];
    *message = malloc((message_attr_length-4)*sizeof(char));
    strncpy(*message, buffer+begin+4, message_attr_length-4);
    return begin+message_attr_length;
}



int generateCLIENTIDLE(struct SBCPMessage *msg)
{
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(IDLE | 0x80);
    msg->header[2] = (uint8_t)(4 >> 8);
    msg->header[3] = (uint8_t)(4 & 0xFF);

    return 4;
}

int generateSERVERIDLE(struct SBCPMessage *msg, char *username)
{
    if(strlen(username) + 1 > ATTRUSERNAMEMAX) {
        return -1;
    }

    uint16_t name_attr_length = (uint16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);

    // Setup SBCP message
    msg->payload[0] = *nameAttr;
    uint16_t msg_length = (uint16_t) (name_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(IDLE | 0x80);
    msg->header[2] = (uint8_t)(msg_length >> 8);
    msg->header[3] = (uint8_t)(msg_length & 0xFF);

    free(nameAttr);
    return msg_length;
}

// Return -1 in case of error
int parseSERVERIDLE(char buffer[], char **username) { return readNameAttr(buffer, 4, username); }

int generateNAK(struct SBCPMessage *msg, char* reason)
{
    if(strlen(reason) + 1 > ATTRREASONMAX) {
        return -1;
    }

    uint16_t reason_attr_length = (uint16_t) (1 + strlen(reason) + 4);
    struct SBCPAttribute *reasonAttr = buildReasonAttr(reason, reason_attr_length);
    msg->payload[0] = *reasonAttr;
    uint16_t total_Bytes = (uint16_t) (4 + reason_attr_length);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(NAK | 0x80);
    msg->header[2] = (uint8_t)(total_Bytes >> 8);
    msg->header[3] = (uint8_t)(total_Bytes & 0xFF);

    free(reasonAttr);
    return total_Bytes;
}

// Return -1 in case of error
int parseNAK(char buffer[], char **reason) { return readReasonAttr(buffer, 4, reason); }

// Generate JOIN Message
int generateJOIN(struct SBCPMessage *msg, char *username)
{
    // @Required: 'username' field

    // 1st: Check the message length, if overflow, return -1
    if(strlen(username) + 1 > ATTRUSERNAMEMAX) {
        return -1;
    }

    // Setup SBCP message attribute
    uint16_t name_attr_length = (uint16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);

    // Setup SBCP message
    msg->payload[0] = *nameAttr;
    uint16_t msg_length = (uint16_t) (name_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(JOIN | 0x80);
    msg->header[2] = (uint8_t)(msg_length >> 8);
    msg->header[3] = (uint8_t)(msg_length & 0xFF);

    free(nameAttr);
    return msg_length;
}

int generateSEND(struct SBCPMessage *msg, char *messages)
{
    if(strlen(messages) + 1 > ATTRMESSAGEMAX) {
        return -1;
    }

    uint16_t message_attr_length = (uint16_t) (1+strlen(messages) + 4);
    struct SBCPAttribute *messageAttr = buildMessageAttr(messages, message_attr_length);

    msg->payload[0] = *messageAttr;
    uint16_t msg_length = (uint16_t) (message_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(SEND | 0x80);
    msg->header[2] = (uint8_t)(msg_length >> 8);
    msg->header[3] = (uint8_t)(msg_length & 0xFF);

    free(messageAttr);
    return msg_length;
}

// Generate ONLINE Message
int generateONLINE(struct SBCPMessage *msg, char *username)
{

    // Required: 'username' field
    if(strlen(username) + 1 > ATTRUSERNAMEMAX) {
        return -1;
    }

    // Setup SBCP message attribute
    uint16_t name_attr_length = (uint16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);

    // Setup SBCP message
    msg->payload[0] = *nameAttr;
    uint16_t msg_length = (uint16_t) (name_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(ONLINE | 0x80);
    msg->header[2] = (uint8_t)(msg_length >> 8);
    msg->header[3] = (uint8_t)(msg_length & 0xFF);

    free(nameAttr);
    return msg_length;
}

// Return -1 in case of error
int parseONLINE(char buffer[], char **username) { return readNameAttr(buffer, 4, username); }

// Generate OFFLINE Message
int generateOFFLINE(struct SBCPMessage *msg, char *username)
{

    // Required: 'username' field
    if(strlen(username) + 1 > ATTRUSERNAMEMAX) {
        return -1;
    }

    // Setup SBCP message attribute
    uint16_t name_attr_length = (uint16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);

    // Setup SBCP message
    msg->payload[0] = *nameAttr;
    uint16_t msg_length = (uint16_t) (name_attr_length + 4);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(OFFLINE | 0x80);
    msg->header[2] = (uint8_t)(msg_length >> 8);
    msg->header[3] = (uint8_t)(msg_length & 0xFF);

    free(nameAttr);
    return msg_length;
}

// Return -1 in case of error
int parseOFFLINE(char buffer[], char **username) { return readNameAttr(buffer, 4, username); }

int generateFWD(struct SBCPMessage *msg, char *username, char *messages)
{
    if(strlen(username) + 1 > ATTRUSERNAMEMAX) {
        return -1;
    } else if(strlen(messages) + 1 > ATTRMESSAGEMAX) {
        return -1;
    }

    // Format: nameAttr followed by messageAttr
    int total_Bytes = 4;

    uint16_t name_attr_length = (uint16_t) (1 + strlen(username) + 4);
    struct SBCPAttribute *nameAttr = buildNameAttr(username, name_attr_length);
    msg->payload[0] = *nameAttr;
    total_Bytes += name_attr_length;

    uint16_t message_attr_length = (uint16_t) (1+strlen(messages) + 4);
    struct SBCPAttribute *messageAttr = buildMessageAttr(messages, message_attr_length);
    msg->payload[1] = *messageAttr;
    total_Bytes += message_attr_length;

    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(FWD | 0x80);
    msg->header[2] = (uint8_t)(total_Bytes >> 8);
    msg->header[3] = (uint8_t)(total_Bytes & 0xFF);

    free(nameAttr);
    free(messageAttr);
    return total_Bytes;
}

// Return -1 in case of error
int parseFWD(char buffer[], char **message, char **username)
{
    int cursor = readNameAttr(buffer, 4, username);
    if(cursor != -1) {
        return readMessageAttr(buffer, cursor, message);
    }
    return -1;
}

// For now, do not check for this method, assume server will store all valid usernames
int generateACK(struct SBCPMessage *msg, char* usernames[MAXUSERCOUNT])
{

    // Format: countAttr followed by nameAttr

    uint16_t counter = 0;
    int total_Bytes = 4;
    for(int i=0; i< MAXUSERCOUNT; i++) {

        char* stepUsername = usernames[i];
//        printf("%s\n", stepUsername);
        if(stepUsername == NULL) {
            counter = (uint16_t) i;
            break;
        }
        // Create a username Attr.
        uint16_t name_attr_length = (uint16_t) (1 + strlen(stepUsername) + 4);
        struct SBCPAttribute *nameAttr = buildNameAttr(stepUsername, name_attr_length);
        // Assign to the msg Structure
        msg->payload[i+1] = *nameAttr;
        total_Bytes += name_attr_length;
        free(nameAttr);

    }

    // Create the client count Attribute
    uint16_t count_attr_length = (uint16_t) (2 + 4);
    struct SBCPAttribute *countAttr = buildCountAttr(counter);
    msg->payload[0] = *countAttr;

    // Finally create the Message
    total_Bytes += count_attr_length;
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (uint8_t)(ACK | 0x80);
    msg->header[2] = (uint8_t)(total_Bytes >> 8);
    msg->header[3] = (uint8_t)(total_Bytes & 0xFF);

    free(countAttr);
    return total_Bytes;
}

int parseACK(char buffer[], int * client_count, char* usernames[MAXUSERCOUNT])
{
    int cursor = readCountAttr(buffer, 4, client_count);
    if(cursor ==  -1){  return -1; }
    for(int i=0; i<*client_count; i++) {
        cursor = readNameAttr(buffer, cursor, &usernames[i]);
        if(cursor ==  -1){  return -1; }
    }
    return 0;
}




int createRawData(char buffer[], struct SBCPMessage *message, int msg_length)
{
    int buffer_index = 0;
    buffer[buffer_index++] = (char) message->header[0];
    buffer[buffer_index++] = (char) message->header[1];
    buffer[buffer_index++] = (char) message->header[2];
    buffer[buffer_index++] = (char) message->header[3];

    if((message->header[1] & 0x7F) == ACK) {
        // This case the message contains more than 1 attribute
        struct SBCPAttribute firstAttr =message->payload[0];
        buffer[buffer_index++] = (char) firstAttr.attr_header[0];
        buffer[buffer_index++] = (char) firstAttr.attr_header[1];
        buffer[buffer_index++] = (char) firstAttr.attr_header[2];
        buffer[buffer_index++] = (char) firstAttr.attr_header[3];
        buffer[buffer_index++] = firstAttr.attr_payload[0];
        buffer[buffer_index++] = firstAttr.attr_payload[1];

        int num_of_users = (firstAttr.attr_payload[0] << 8) + firstAttr.attr_payload[1];
        for(int i=0; i<num_of_users; i++) {
            struct SBCPAttribute nameAttr = message->payload[1+i];
            buffer[buffer_index++] = nameAttr.attr_header[0];
            buffer[buffer_index++] = nameAttr.attr_header[1];
            buffer[buffer_index++] = nameAttr.attr_header[2];
            buffer[buffer_index++] = nameAttr.attr_header[3];
            int username_length = (nameAttr.attr_header[2] << 8) + nameAttr.attr_header[3] - 4;
            for(int k=0;k<username_length;k++) {
                buffer[buffer_index++] = nameAttr.attr_payload[k];
            }
        }

        if(buffer_index != msg_length) {
//            printf("sth. wrong in buffer size \n");
            return -1;
        }
    } else if ((message->header[1] & 0x7F) == FWD) {
        // 2 frames needed to send
        struct SBCPAttribute nameAttr = message->payload[0];
        buffer[buffer_index++] = nameAttr.attr_header[0];
        buffer[buffer_index++] = nameAttr.attr_header[1];
        buffer[buffer_index++] = nameAttr.attr_header[2];
        buffer[buffer_index++] = nameAttr.attr_header[3];
        int username_length = (nameAttr.attr_header[2] << 8) + nameAttr.attr_header[3] - 4;
        for(int k=0;k<username_length;k++) {
            buffer[buffer_index++] = nameAttr.attr_payload[k];
        }

        struct SBCPAttribute messageAttr = message->payload[1];
        buffer[buffer_index++] = messageAttr.attr_header[0];
        buffer[buffer_index++] = messageAttr.attr_header[1];
        buffer[buffer_index++] = messageAttr.attr_header[2];
        buffer[buffer_index++] = messageAttr.attr_header[3];
        int messageAttr_length = (messageAttr.attr_header[2] << 8) + messageAttr.attr_header[3] - 4;
        for(int k=0; k<messageAttr_length;k++) {
            buffer[buffer_index++] = messageAttr.attr_payload[k];
        }

        if(buffer_index != msg_length) {
//            printf("sth. wrong in buffer size \n");
            return -1;
        }
    } else {
        // Other types only 1 Attr is included
        if(buffer_index < msg_length) {
            buffer[4] = (char) message->payload->attr_header[0];
            buffer[5] = (char) message->payload->attr_header[1];
            buffer[6] = (char) message->payload->attr_header[2];
            buffer[7] = (char) message->payload->attr_header[3];

            for(int i=8;i<msg_length;i++) {
                buffer[i] = message->payload->attr_payload[i - 8];
            }
        }
    }
    return 0;
}


/*  Sample Message Calls

    //     2nd: ACK message
    char* usernames[MAXUSERCOUNT];
    memset(&usernames, 0, sizeof usernames);
    usernames[0] = "A";
    usernames[1] = "J";
    usernames[2] = "N";
    struct SBCPMessage *ack_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    int ack_msg_len = generateACK(ack_msg, usernames);
    //    int ack_rv = send_message(ack_msg, ack_msg_len, socket_fd);

    free(ack_msg);

    //     3rd: NAK message
    struct SBCPMessage *nak_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    char *sampleReason = "123456789";
    int nak_msg_len = generateNAK(nak_msg, sampleReason);
    //    int nak_rv = send_message(nak_msg, nak_msg_len, socket_fd);

    free(nak_msg);

    //      4th: FWD message
    struct SBCPMessage *fwd_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    char *fwdUser = "fwd";
    char *fwdMessage = "msg";
    int fwd_msg_len = generateFWD(fwd_msg, fwdUser, fwdMessage);
    //    int fwd_rv = send_message(fwd_msg, fwd_msg_len, socket_fd);

    free(fwd_msg);

    //      5th: CLIENTIDLE/SERVERIDLE message
    struct SBCPMessage *cidle_msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    int cidle_msg_len = generateCLIENTIDLE(cidle_msg);
    //    int cidle_rv = send_message(cidle_msg, cidle_msg_len, socket_fd);

    free(cidle_msg);

    struct SBCPMessage *sidle_msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    char *sidleUser = "id";
    int sidle_msg_len = generateSERVERIDLE(sidle_msg, sidleUser);
    //    int sidle_rv = send_message(sidle_msg, sidle_msg_len, socket_fd);

    free(sidle_msg);


    //      6th: ONLINE/OFFLINE message
    struct SBCPMessage *on_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    struct SBCPMessage *off_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    char *onUser = "onn";
    char *offUser = "off";
    int on_msg_len = generateONLINE(on_msg, onUser);
    int off_msg_len = generateOFFLINE(off_msg, offUser);
    //    int on_rv = send_message(on_msg, on_msg_len, socket_fd);
    //    int off_rv = send_message(off_msg, off_msg_len, socket_fd);

    free(on_msg);
    free(off_msg);
*/


