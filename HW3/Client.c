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

#define MAXBUFLEN 513


#define SERVERPORT "4950" // the port users will be connecting to
int main(int argc, char *argv[])
{
    int sockfd=-1;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    ssize_t numbytes;

    char buf[MAXBUFLEN];
    ssize_t recvcount;

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;

    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
// loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    if(sockfd == -1) {
        return 3;
    }


    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    freeaddrinfo(servinfo);
    printf("talker: sent %zi bytes to %s\n", numbytes, argv[1]);

    while(1) {
        if ((recvcount = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        if(recvcount >= MAXBUFLEN) {
            continue;
        }

        printf("Received:\n%s\n", buf);
        memset(&buf, 0, sizeof buf);

    }


    close(sockfd);
    return 0;
}