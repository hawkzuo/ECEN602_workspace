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
#include "readFast.h"
#define PORT "5432" // the port client will be connecting to
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

ssize_t readline(int fd, void *buffer, size_t n)
{
    int numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total bytes read so far */
    char *bf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }
    printf("readline\n");
    bf = buffer;                       /* No pointer arithmetic on "void *" */

    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1);
        if (numRead == -1) {
            printf("%d", -1);
            if (errno == EINTR) {
                continue;       /* Interrupted --> restart read() */
            } else {
                return -1;              /* Some other error */
            }        
        } else if (numRead == 0) {      /* EOF */
            printf("%d", 0);
            if (totRead == 0)           /* No bytes read; return 0 */
                return 0;
            else                        /* Some bytes read; add '\0' */
                break;

        } else {                        /* 'numRead' must be 1 if we get here */
            printf("%d", numRead);
            if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
                totRead++;
                *bf = ch;
                bf++;
            }
   
            if (ch == '\n')
                break;
        }
    }

    *bf = '\0';
    return totRead;
}








int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE+1];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    int len;
    char bufRecv[MAXDATASIZE+1];


    
    int tmp;
    
    
    

    if (argc != 4) {
        fprintf(stderr,"Number of arguments error, expected: 4, got: %d\n", argc);
        exit(1);
    } else if(strcmp(argv[1], PROTOCOL)) {
        fprintf(stderr, "Unsupported protocol, expected: echos, got: %s\n", argv[1]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
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

        buf[MAXDATASIZE] = '\0';
        len = strlen(buf) + 1;
        //  send(s, buf, len, 0);
        printf("Input string:'%s'.\n", buf);
        if((int)writen(sockfd, buf, len) != len) {
            fprintf(stderr, 
                "Encounter an error sending lines. Expected:%d\n", len);
        } else {
            printf("Sent string '%s' to the server.\n", buf);

        }
        
        printf("Begin receiving:\n");
        tmp = (int)readFast(sockfd, bufRecv, sizeof(bufRecv));
        printf("Number readFast returned: %d.\n", tmp);
        if(  tmp != len-1)  {
            printf("Received wrong string %s, with length: %lu, expected length: %d \n", bufRecv, strlen(bufRecv), len);
        } else {
            printf("Received:\n");
            fputs(bufRecv, stdout);
        }
        memset(&buf, 0, sizeof buf);
        memset(&bufRecv, 0, sizeof bufRecv);
    }

    printf("Out of loop.\n");
    close(sockfd);
    return 0;
}




