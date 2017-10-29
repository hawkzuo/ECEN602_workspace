/*
** server.c -- a stream socket server demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include "TFTP.h"

#define MYPORT "4950" // the port users will be connecting to



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int readable_timeo(int fd, int sec) {
    fd_set rset;
    struct timeval tv;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return select(fd+1, &rset, NULL,NULL, &tv);
}

//Global nextchar
char nextchar;

int readViaASCII(FILE *fp, char *ptr, int maxnbytes)
{
    for(int count=0; count < maxnbytes; count++) {
        if(nextchar >= 0) {
            *ptr++ = nextchar;
            nextchar = -1;
            continue;
        }

        int c = getc(fp);
        if(c == EOF) {
            if(ferror(fp)) {
                perror("read err on getc");
            }
            return count;
        } else if (c == '\n') {
            c = '\r';
            nextchar = '\n';
        } else if (c == '\r') {
            nextchar = '\0';
        } else {
            nextchar = -1;
        }
        *ptr++ = (char) c;
    }
    return maxnbytes;
}


void sendFileViaTFTP(const char *filename,
                     int newfd,
                     struct sockaddr_storage their_addr,
                     socklen_t addr_len,
                     const char *mode)
{
    // Initialize file descriptors
    int fd;
    FILE* fp;
    nextchar = -1;
    if(strcmp(mode, OCTET) == 0) {
        fd = open(filename, O_RDONLY);
        fp = NULL;
    } else if(strcmp(mode, NETASCII) == 0) {
        fp = fopen(filename, "r");
        fd = -1;
    } else {
        // This case should not happen
        return;
    }

    // Read the First Packet
    char fileBuffer[MAXDATALEN+1];
    uint16_t seqNum = 1;
    ssize_t file_read_count;
    if(strcmp(mode, OCTET) == 0) {
        file_read_count	= read(fd, fileBuffer, MAXDATALEN);
    } else {
        file_read_count = readViaASCII(fp, fileBuffer, MAXDATALEN);
    }

    // Loop Over All Left
    while(file_read_count >= 0 ) {

        // Send A full DATA packet
        char dataMsg[file_read_count+4];
        if(generateDATA(dataMsg, fileBuffer, seqNum, file_read_count) == 0) {
            int retries = 0;
            sendto(newfd, dataMsg, sizeof dataMsg, 0,
                   (struct sockaddr *)&their_addr, addr_len);
            // Retry Logic
            while(retries <= 10 ) {
                if(readable_timeo(newfd, 1) <= 0) {
                    // Timeout
                    if(retries >= 10) {
                        printf("Retry Times is maximum. Will close connection\n");
                        close(newfd);
                        exit(0);
                    }
                    retries ++;
                    printf("Retry Times: %d\n", retries);
                    sendto(newfd, dataMsg, sizeof dataMsg, 0,
                           (struct sockaddr *)&their_addr, addr_len);
                } else {
                    // We might receive an ACK
                    ssize_t received_count;
                    uint16_t received_seq_num;
                    char ackBuf[5];
                    if ((received_count = recvfrom(newfd, ackBuf, 4 , 0,
                                                   (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                        perror("recvfrom:ACK");
                        exit(1);
                    }

                    if(received_count == 4 && parseACK(&received_seq_num, ackBuf, 4) == 0) {
//						printf("ReceivedSeq: %d\n", received_seq_num);
//						printf("SeqNum: %d\n", seqNum);
                        if(received_seq_num == seqNum) {
                            break;
                        }
                    }
                }
            }
            // Wrap-Around
            if(seqNum == 65535) {
                // Wrap is needed
                seqNum = 0;
            } else {
                seqNum ++;
            }
        }
        memset(&fileBuffer, 0, sizeof fileBuffer);
        if(file_read_count == MAXDATALEN) {
            // Keep Sending
            if(strcmp(mode, OCTET) == 0) {
                file_read_count	= read(fd, fileBuffer, MAXDATALEN);
            } else {
                file_read_count = readViaASCII(fp, fileBuffer, MAXDATALEN);
            }
        } else {
            // Last Frame
            printf("Succeeded sending %d packets DATA.\n", seqNum-1);
            break;
        }

    }
    // Reset this global helper Character after sending each file
    nextchar = -1;
}

/**
 * Perform reading to file task
 * @param filename 		-Absolute filename written to
 * @param new_fd  		-The Socket to Recvfrom
 * @param their_addr	-Their IP address under protocol UDP
 * @param addr_len		-Size of their_addr
 * @return	total number of bytes read, -1 on open file error, -2 on socket read error
 */
int64_t receiveFileViaTFTP(const char *filename,
                           int new_fd,
                           struct sockaddr_storage their_addr,
                           socklen_t addr_len)
{
    int read_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    int totalBytesRead = 0;
    if(read_fd < 0) {
        // No such file Should Later ERROR packet
        return -1;
    }
    char receiveBuf[MAXDATALEN+4+1];
    uint16_t desiredSeq = 1;
    uint16_t seqNum = 0;
    ssize_t receiveCount;

    // send the first ACK:
    char ackFirst[4];
    if(generateACK(ackFirst, seqNum) == 0) {
        sendto(new_fd, ackFirst, (size_t) 4, 0,
               (struct sockaddr *)&their_addr, addr_len);
    }

    while(1) {
        if ((receiveCount = recvfrom(new_fd, receiveBuf, MAXDATALEN+4+1, 0,
                                  (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            return -2;
        }

        if(receiveCount > MAXDATALEN+4 || receiveCount < 5) {
            printf("Discard Wrong Packet.\n");
            continue;
        }

        char* data_msg;
        char ackMsg[4];

        if(parseDATA(&data_msg, &seqNum, receiveBuf, receiveCount) == 0) {
            //printf("DesiredSeq: %d\n", desiredSeq);
            //printf("SeqNum: %d\n", seqNum);
            if(desiredSeq == seqNum) {
                // Success Print on screen & send ACK back
                write(read_fd, data_msg, (size_t) (receiveCount - 4));

                if(generateACK(ackMsg, seqNum) == 0) {
                    // Send to Server
                    sendto(new_fd, ackMsg, (size_t) 4, 0,
                           (struct sockaddr *)&their_addr, addr_len);
                }

                totalBytesRead += receiveCount-4;

                if(receiveCount-4 != MAXDATALEN) {
                    // Last frame
                    break;
                } else {
                    if(desiredSeq == 65535) {
                        printf("Current Read Bytes: %d\n", totalBytesRead);
                        desiredSeq = 0;
                    } else {
                        desiredSeq ++;
                    }
                }

            }
        }
        free(data_msg);
        memset(&receiveBuf, 0, sizeof receiveBuf);
    }
    close(read_fd);
    return totalBytesRead;
}


int main(void)
{
    int sockfd=-1;
    int newfd;

    struct addrinfo hints, *servinfo, *p;
    int rv;
    ssize_t numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXRECVBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    if(sockfd == -1) {
        return 3;
    }
    freeaddrinfo(servinfo);
	printf("server: started.\nlistening on port: %s\nwaiting for connections...\n\n", MYPORT);
    addr_len = sizeof their_addr;

    // Server starts here
    while(1) {
        if ((numbytes = recvfrom(sockfd, buf, MAXRECVBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        printf("server: got packet from %s\n",
               inet_ntop(their_addr.ss_family,
                         get_in_addr((struct sockaddr *)&their_addr),
                         s, sizeof s));
        printf("server: packet is %zi bytes long\n", numbytes);
        if(numbytes >= MAXRECVBUFLEN) {
            continue;
        }
        buf[numbytes] = '\0';

        char* filename;
        char* mode;
        int isRRQ;

        if(parseRRQ(&filename, &mode, buf, numbytes) == 0) {
            printf("Filename: %s\n", filename);
            printf("Mode: %s\n", mode);
            printf("Type: RRQ\n");
            isRRQ = 1;
        } else if(parseWRQ(&filename, &mode, buf, numbytes) == 0) {
            printf("Filename: %s\n", filename);
            printf("Mode: %s\n", mode);
            printf("Type: WRQ\n");
            isRRQ = 0;
        } else {
            printf("Unacceptable RRQ/WRQ packet.\n");
            continue;
        }

        newfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (!fork()) { // this is the child process
            // Child process
            close(sockfd); // child doesn't need the listener

            struct sockaddr_in my_addr;
            my_addr.sin_family = AF_INET;
            my_addr.sin_port = htons(0); // short, network byte order
            my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
            memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
            bind(newfd, (struct sockaddr *)&my_addr, sizeof my_addr);

            // Two Modes
            if(isRRQ == 1) {
                if( strcmp(mode, OCTET) == 0 || strcmp(mode, NETASCII) == 0 ) {
                    // Two Supported Modes: OCTET & NETASCII Mode
                    sendFileViaTFTP(filename, newfd, their_addr, addr_len, mode);
                } else {
                    // Should send an ERROR packet "Unsupported mode"
                }
                printf("File sent.\n");
            } else {

                int64_t fileSize = receiveFileViaTFTP(filename, newfd, their_addr, addr_len);
                if (fileSize == -1) {
                    // Send "No Such File to Client"
                } else if (fileSize == -2) {
                    printf("Encounter Error while trying to recvfrom.\n");
                } else {
                    printf("File received. Total file size is %lli bytes. \n", fileSize);
                }
            }
            printf("Closing Connection...\n");
            close(newfd);
            exit(0);
        }
        // Parent Process does not need this
        close(newfd);
    }

    close(sockfd);
    return 0;
}

