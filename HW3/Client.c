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
#include "TFTP.h"

#define MAXRECVBUFLEN 517


#define SERVERPORT "4950" // the port users will be connecting to
int main(int argc, char *argv[])
{
    int sockfd=-1;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    ssize_t numbytes;

    char recvBuf[MAXRECVBUFLEN];
    ssize_t recvcount;

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;

    if (argc != 4) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }
    char* mode;
    if(strcmp(argv[3], OCTET) == 0) {
        mode = OCTET;
    } else if (strcmp(argv[3], "netascii") == 0) {
        mode = NETASCII;
    } else {
        perror("client:mode");
        return 4;
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

    char* rrqMsg;

    int rrqSize = generateRRQ(&rrqMsg, argv[2], mode);

//    printf("Byte View:\n");
//    for(int i=0;i<rrqSize;i++) {
//        printf("%s\n", byte_to_binary(*(rrqMsg+i)));
//    }


    if ((numbytes = sendto(sockfd, rrqMsg, (size_t) rrqSize, 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    freeaddrinfo(servinfo);
    printf("talker: sent %zi bytes to %s\n", numbytes, argv[1]);

    uint16_t desiredSeq = 1;
    uint16_t seqNum = 0;

    while(1) {
        if ((recvcount = recvfrom(sockfd, recvBuf, MAXRECVBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        if(recvcount >= MAXRECVBUFLEN) {
            continue;
        }

//        printf("Received Byte View:\n");
//        for(int i=0;i<recvcount;i++) {
//            printf("%s\n", byte_to_binary(recvBuf[i]));
//        }

        char* msg;
        char ackMsg[4];

        if(parseDATA(&msg, &seqNum, recvBuf, recvcount) == 0) {
            printf("Data received for Packet NO %d, with content:\n%s\n", seqNum,msg);
            if(desiredSeq == seqNum) {
                // Success Print on screen & send ACK back
                if(generateACK(ackMsg, seqNum) == 0) {
                    // Send to Server
                    sendto(sockfd, ackMsg, (size_t) 4, 0,
                           (struct sockaddr *)&their_addr, addr_len);
                }

                if(recvcount-4 != MAXDATALEN) {
                    // Last frame
                    break;
                } else {
                    if(desiredSeq == 65535) {
                        desiredSeq = 0;
                    } else {
                        desiredSeq ++;
                    }
                }

            }
        }
        memset(&recvBuf, 0, sizeof recvBuf);
    }


    close(sockfd);
    return 0;
}
