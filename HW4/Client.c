/*
** client.c -- a stream socket client demo
*/
// CLion cmake demo: https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "HTTP.h"

#define MAXDATASIZE 1000 // max number of characters in a string we can send/get at once, including the '\n' char

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Send HTTP Message to fd */
int send_message(const char *message, int msg_length, int fd)
{
    char buffer[msg_length+1];
    buffer[msg_length] ='\0';

    for(int i=0;i<msg_length;i++) {
        buffer[i] = *(message+i);
    }

    ssize_t send_count = writen(fd, buffer, sizeof(buffer));
    if(send_count != sizeof(buffer)) {
        perror("client:send_message:writen");
        return -1;
    } else {
//        printf("Successfully sent %zi bytes.\n", send_count);
        return 0;
    }
}

int main(int argc, char *argv[])
{
    // socket_fd: The socket file descriptor
    int socket_fd=-1;

    // buf: User input buffer
    // MAXDATASIZE is the acceptable input size, including the '\n' char.
    char buf[MAXDATASIZE+1];

    // hints, *servinfo, *p: used for checking IP addresses
    struct addrinfo hints, *servinfo, *p;

    // rv: return value for system calls
    int rv;
    // s: server IP address
    char s[INET6_ADDRSTRLEN];

    // Recv Message Len
    int recv_message_len;
    char bufRecv[MAXDATASIZE+1];


    // received_count: Count of chars received from the server
    ssize_t received_count;
    // message_type: the received type of message
    int message_type;


    // Client Username
    char* username;


    /* Input checking */
    if (argc != 4) {
        // Usage: ./client ip port url
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

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
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

    if (p == NULL || socket_fd == -1) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

//     1st: Send HTTP request to server.
    char* http_message;
    int http_message_len;
    int rv_status = buildHTTPRequest(&http_message, &http_message_len, argv[3]);
    if (rv_status != 0) {
        perror("client: buildRequest");
        return 3;
    }
    rv_status = send_message(http_message, http_message_len, socket_fd);
    if(rv_status != 0) {
        perror("client: SEND REQUEST");
        exit(1);
    }
    free(http_message);

//      2nd: Receive file from server.
//  Client side 'select' is not needed
    time_t curtime;
    time(&curtime);
    char filename[128];
    memset(&filename, 0, sizeof(filename));
    sprintf(filename,"%ld_sample.html",curtime);

    int read_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(read_fd == -1) {
        perror("client: openfile");

    }
    while(1) {
        received_count = recv(socket_fd, bufRecv, sizeof(bufRecv), 0);
        if(received_count <= 0) {
            if(received_count < 0) {
                perror("client: recv");
            } else {
                printf("Server closed connection. Program will end.\n");
                break;
            }
        }
        // Create a file first
        write(read_fd, bufRecv, (size_t) (received_count));
        memset(&bufRecv, 0, sizeof bufRecv);

        // Dummy for IDE setting
        if(socket_fd > 10000) {
            break;
        }
    }

    close(read_fd);
    printf("Closing The Program. \n");
    close(socket_fd);

    return 0;
}
