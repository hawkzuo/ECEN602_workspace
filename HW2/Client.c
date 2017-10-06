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

#define MAXDATASIZE 256 // max number of characters in a string we can send/get at once, including the '\n' char


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
//    printf("converted version of msg: %s\n", ptr);
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


// Used to send SBCP Message to server. Support all message types
int send_message(struct SBCPMessage *message, int msg_length, int fd)
{

    char buffer[msg_length];
    int buffer_index = 0;
    buffer[buffer_index++] = (char) message->header[0];
    buffer[buffer_index++] = (char) message->header[1];
    buffer[buffer_index++] = (char) message->header[2];
    buffer[buffer_index++] = (char) message->header[3];

    struct SBCPAttribute firstAttr =message->payload[0];
    if(firstAttr.attr_header[1] == ATTRCOUNT) {
        // This case the message contains more than 1 attribute
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

    } else if ((message->header[1] & 0x7F) == FWD) {
//        msg->header[1] = (u_int8_t)(SEND | 0x80);
        // 2 frames needed to send


    } else {
        // Other types only 1 Attr is included
        buffer[4] = (char) message->payload->attr_header[0];
        buffer[5] = (char) message->payload->attr_header[1];
        buffer[6] = (char) message->payload->attr_header[2];
        buffer[7] = (char) message->payload->attr_header[3];

        for(int i=8;i<msg_length;i++) {
            buffer[i] = message->payload->attr_payload[i - 8];
        }
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


    int message_len;
    char bufRecv[MAXDATASIZE+1];


    // received_count: Count of chars received from the server
    ssize_t received_count;

    // Client Username
    char* username;


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

    // Client SBCP message

//     1st: Send JOIN to server.
    struct SBCPMessage *join_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    int msg_length = generateJOIN(join_msg, username);
    int join_rv = send_message(join_msg, msg_length, socket_fd);

//    printf("Real Size of SBCP Message: %zu", sizeof(msg));
//    printf("SBCP Message Payload size: %zu", msg->payload);


//     2nd: ACK message
//    char* usernames[MAXUSERCOUNT];
//    memset(&usernames, 0, sizeof usernames);
//    usernames[0] = "A";
//    usernames[1] = "J";
//    usernames[2] = "N";
//    struct SBCPMessage *ack_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
//    int ack_msg_len = generateACK(ack_msg, usernames);
//    int ack_rv = send_message(ack_msg, ack_msg_len, socket_fd);

//     3rd: NAK message
//    struct SBCPMessage *nak_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
//    printf("Here");
//    char *sampleReason = "123456789";
//    printf("before %s", sampleReason);
//    int nak_msg_len = generateNAK(nak_msg, sampleReason);
//    printf("after %s", sampleReason);
//    int nak_rv = send_message(nak_msg, nak_msg_len, socket_fd);



    fd_set read_fds; // temp file descriptor list for select()
    fd_set master;

    while(1)  {
        FD_SET(0, &master);
        FD_SET(socket_fd, &master);

        select(1+socket_fd, &master, NULL, NULL, NULL);
        if(FD_ISSET(0, &master)) {
            // You have something in the terminal
            buf[MAXDATASIZE] = '\0';

            // fgets includes the '\n' char
            fgets(buf, sizeof(buf), stdin);
//            message_len = strlen(buf) + 1;
            printf("Input string (no space):%s", buf);
            struct SBCPMessage *msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
            message_len = generateSEND(msg, buf);
            printf("Imessage_len:%d", message_len);
            int send_rv = send_message(msg, message_len, socket_fd);
            memset(&buf, 0, sizeof buf);

        } else {
            // You have messages waiting to be displayed in the terminal
            received_count = recv(socket_fd, bufRecv, sizeof(bufRecv), 0);
            printf("Number Recv: %zu\n", received_count);

            printf("Received:\n");
            fputs(bufRecv, stdout);

            for(int i=0; i<received_count; i++) {
                printf("Binary: %s\n", byte_to_binary(bufRecv[i]));
            }
            memset(&bufRecv, 0, sizeof bufRecv);
        }

        if(msg_length > 10000) {
            break;
        }

    }







//    select();

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

//    free(join_msg);
//    free(nameAttr);

    free(join_msg);
//    free(ack_msg);
//    free(nak_msg);

    return 0;
}




