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
#include "readFast.h"

#define MAXDATASIZE 10 // max number of characters in a string we can send/get at once, including the '\n' char
#define PROTOCOL "echos"

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

    /* Input checking */
    if (argc != 4) {
        fprintf(stderr,"Number of arguments error, expected: 4, got: %d\n", argc);
        exit(1);
    } else if(strcmp(argv[1], PROTOCOL) != 0) {
        fprintf(stderr, "Unsupported protocol, expected: echos, got: %s\n", argv[1]);
        exit(1);
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
    while (fgets(buf, sizeof(buf), stdin)) {

        buf[MAXDATASIZE] = '\0';
        if(strlen(buf) == sizeof(buf)-1 && buf[MAXDATASIZE-1] != '\n') {
            printf("Input size exceed the maximum acceptable size, will quit. \n");
            break;
        }
        len = strlen(buf) + 1;

        printf("Input string (no space):%s", buf);
        if((int)writen(socket_fd, buf, len) != len) {
            fprintf(stderr, 
                "Encounter an error sending lines. Expected:%zu\n", len);
        } else {
        }

        received_count = (int)readFast(socket_fd, bufRecv, sizeof(bufRecv));

        if(received_count != len-1)  {
            printf("Received wrong string %s, with length: %lu, expected length: %zu \n", bufRecv, strlen(bufRecv), len);
        } else {
            printf("Received:\n");
            fputs(bufRecv, stdout);
        }
        memset(&buf, 0, sizeof buf);
        memset(&bufRecv, 0, sizeof bufRecv);
    }

    /* End of Program */
    printf("Out of loop.\n");
    close(socket_fd);
    return 0;
}




