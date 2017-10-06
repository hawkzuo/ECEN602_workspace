/*
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "SBCP.h"

#define MAXDATASIZE 10 // max number of characters in a string we can send/get at once, including the '\n' char


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Write chars to the socket
ssize_t writen(int fd, void *vptr, size_t n) {
    size_t num_char_left;
    ssize_t num_char_written;
    char *ptr;

    ptr = vptr;
    printf("converted version of msg: %s\n", ptr);
    num_char_left = n;
    while (num_char_left > 0) {
        // write is system call
        if( (num_char_written = write(fd, ptr, num_char_left)) <= 0) {
            // If error is EINTR, we try looping again
            if(num_char_written <0 && errno == EINTR) {
                num_char_written = 0;
            } else {
                // Other error types, will quit
                return -1;
            }
        }
        num_char_left -= num_char_written;
        ptr += num_char_written;
    }
    return n;
}



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

// Used to send SBCP Message to server. Will support all message types
int send_message(struct SBCPMessage *message, u_int16_t msg_length, int fd) {

    char buffer[msg_length];
    buffer[0] = (char) message->header[0];
    buffer[1] = (char) message->header[1];
    buffer[2] = (char) message->header[2];
    buffer[3] = (char) message->header[3];
    buffer[4] = (char) message->payload->attr_header[0];
    buffer[5] = (char) message->payload->attr_header[1];
    buffer[6] = (char) message->payload->attr_header[2];
    buffer[7] = (char) message->payload->attr_header[3];

    for(int i=8;i<msg_length;i++) {
        buffer[i] = message->payload->attr_payload[i - 8];
    }

//    for(int i=0;i<8;i++) {
//        printf("Byte: %s", byte_to_binary(buffer[i]));
//    }
//    printf("size of buffer: %zi\n", sizeof(buffer));

    ssize_t send_count = writen(fd, buffer, sizeof(buffer));
    if(send_count != sizeof(buffer)) {
        printf("wrong size sent: %zi", send_count);
        return -1;
    } else {
        printf("Successfully sent %zi bytes.\n", send_count);
    }
    return 0;
}

u_int16_t generateJoin(struct SBCPMessage *msg, struct SBCPAttribute *joinAttr, char* username) {

    // Generate JOIN Message
    // Required: 'username' field

    // Setup SBCP message attribute
    joinAttr->attr_payload = malloc((strlen(username)+1)*sizeof(char));
    strcpy(joinAttr->attr_payload, username);

    u_int16_t joinAttr_attr_length = (u_int16_t) (1 + strlen(joinAttr->attr_payload) + 4);
    joinAttr->attr_header[2] = (u_int8_t) (joinAttr_attr_length >> 8);
    joinAttr->attr_header[3] = (u_int8_t) (joinAttr_attr_length & 0xFF);
    joinAttr->attr_header[0] = 0;
    joinAttr->attr_header[1] = (u_int8_t) ATTRUSERNAME;

//    printf("send_join Attr Length: %hu\n", joinAttr_attr_length);
//    printf("size of char * %zu", sizeof(joinAttr->attr_payload));
//    printf("JOIN Header 0: %s\n", byte_to_binary(joinAttr->attr_header[0]));
//    printf("JOIN Header 1: %s\n", byte_to_binary(joinAttr->attr_header[1]));
//    printf("JOIN Header 2: %s\n", byte_to_binary(joinAttr->attr_header[2]));
//    printf("JOIN Header 3: %s\n", byte_to_binary(joinAttr->attr_header[3]));
//    printf("Size of JOIN attr struct: %zu\n", sizeof(joinAttr));

    // Setup SBCP message
    msg->payload[0] = *joinAttr;
    u_int16_t msg_length = (u_int16_t) (joinAttr_attr_length + 4);
    msg->header[2] = (u_int8_t)(msg_length >> 8);
    msg->header[3] = (u_int8_t)(msg_length & 0xFF);
    msg->header[0] = PROTOCOLVERSION >> 1;
    msg->header[1] = (u_int8_t)(JOIN | 0x80);

//    printf("msg length : %hu\n", msg_length);
//    printf("Message Header 0: %s\n", byte_to_binary(msg->header[0]));
//    printf("Message Header 1: %s\n", byte_to_binary(msg->header[1]));
//    printf("Message Header 2: %s\n", byte_to_binary(msg->header[2]));
//    printf("Message Header 3: %s\n", byte_to_binary(msg->header[3]));

    return msg_length;
}



int main(int argc, char *argv[])
{
    // socket_fd: The socket file descriptor
    int socket_fd;

    // buf: User input buffer
    // MAXDATASIZE is the acceptable input size, including the '\n' char.
    char buf[MAXDATASIZE+1];

    // hints, *servinfo, *p: used for checking IP addresses
    struct addrinfo hints, *servinfo, *p;

    // rv: return value for system calls
    int rv;
    // s: server IP address
    char s[INET6_ADDRSTRLEN];

    size_t len;
    char bufRecv[MAXDATASIZE+1];


    // received_count: Count of chars received from the server
    int received_count;

    // Client Username
    char* username;
    // Client SBCP message
    struct SBCPMessage *msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    struct SBCPAttribute *joinAttr = (struct SBCPAttribute*)malloc(sizeof(struct SBCPAttribute));


    /* Input checking */
    if (argc != 4) {
        fprintf(stderr,"Number of arguments error, expected: 4, got: %d\n", argc);
        exit(1);
    } else  {
        username = argv[1];
        printf("User name is %s\n", username);
    }

    /* IP searching process */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }



    /* Client connecting process */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype,
             p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
        }
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure


    /* Now connected
     * Main task: Read the user input and send to the server, and then receive from the server and
     * print out the string
     */

    // 1st: Send JOIN to server.
    u_int16_t msg_length = generateJoin(msg, joinAttr, username);
    int join_rv = send_message(msg, msg_length, socket_fd);




//    while (fgets(buf, sizeof(buf), stdin)) {
//
//        buf[MAXDATASIZE] = '\0';
//        if(strlen(buf) == sizeof(buf)-1 && buf[MAXDATASIZE-1] != '\n') {
//            printf("Input size exceed the maximum acceptable size, will quit. \n");
//            break;
//        }
//        len = strlen(buf) + 1;
//
//        printf("Input string (no space):%s", buf);
//        if((int)writen(socket_fd, buf, len) != len) {
//            fprintf(stderr,
//                "Encounter an error sending lines. Expected:%zu\n", len);
//        } else {
//        }
//
//        received_count = (int)readFast(socket_fd, bufRecv, sizeof(bufRecv));
//
//        if(received_count != len-1)  {
//            printf("Received wrong string %s, with length: %lu, expected length: %zu \n", bufRecv, strlen(bufRecv), len);
//        } else {
//            printf("Received:\n");
//            fputs(bufRecv, stdout);
//        }
//        memset(&buf, 0, sizeof buf);
//        memset(&bufRecv, 0, sizeof bufRecv);
//    }

    /* End of Program */
    printf("Out of loop.\n");
    close(socket_fd);

    free(msg);
    free(joinAttr);

    return 0;
}




