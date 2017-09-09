/*
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define PORT "3333" // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        // write is system call
        if( (nwritten = write(fd, ptr, nleft)) <= 0) {
            // If error is EINTR, we try looping again
            if(nwritten <0 && errno == EINTR) {
                nwritten = 0;
            } else {
                // Other error types, will quit
                return -1;
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
     ssize_t n, rc;
     char    c, *ptr;
     ptr = vptr;
    for (n = 1; n < maxlen; n++) {
      again:
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;          /* newline is stored, like fgets() */
        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1);     /* EOF, n - 1 bytes were read */
        } else {
            if (errno == EINTR)
                goto again;
            return (-1);        /* error, errno set by read() */
        }
    }
    *ptr = 0;                   /* null terminate like fgets() */
    return (n);
}








int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    int len;




    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
             p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
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


    /* Now connected */
    while (fgets(buf, sizeof(buf), stdin)) {

        buf[MAXDATASIZE-1] = '\0';
        len = strlen(buf) + 1;
        //  send(s, buf, len, 0);
        printf("Begin sending\n");
        if((int)writen(sockfd, buf, len) != len) {
            fprintf(stderr, 
                "Encounter an error sending lines. Expected:%d\n", len);
        } else {
            printf("Sent input string %s to the server.\n", buf);

        }
        buf[0] = '\0';
    }

    printf("Out of loop.\n");
    close(sockfd);
    return 0;
}




