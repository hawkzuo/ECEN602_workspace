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
#define MAXBUFLEN 513


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


int main(void)
{
	int sockfd=-1;
	int newfd;

	struct addrinfo hints, *servinfo, *p;
	int rv;
	ssize_t numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
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
	printf("server: waiting to recvfrom...\n");
	addr_len = sizeof their_addr;
	while(1) {
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
								 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		printf("server: got packet from %s\n",
			   inet_ntop(their_addr.ss_family,
						 get_in_addr((struct sockaddr *)&their_addr),
						 s, sizeof s));
		printf("server: packet is %zi bytes long\n", numbytes);
		if(numbytes >= MAXBUFLEN) {
			continue;
		}
		buf[numbytes] = '\0';

		char* filename;
		char* mode;
		if(parseRRQ(&filename, &mode, buf, numbytes) == 0) {
			printf("Filename: %s\n", filename);
			printf("Mode: %s\n", mode);
		} else {
			printf("Unacceptable RRQ packet.\n");
			continue;
		}

//		printf("Byte View:\n");
//		for(int i=0;i<numbytes;i++) {
//			printf("%s\n", byte_to_binary(buf[i]));
//		}

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
			if(strcmp(mode, OCTET) == 0) {
				// OCTET Mode:
				int fd = open(filename, O_RDONLY);
				char fileBuffer[MAXSENDBUFLEN+1];
				uint16_t seqNum = 1;
				ssize_t file_read_count = read(fd, fileBuffer, MAXSENDBUFLEN);
				while(file_read_count >= 0 ) {
					if(file_read_count == MAXSENDBUFLEN) {
						// Send A full DATA packet
						char dataMsg[file_read_count+4];
						if(generateDATA(dataMsg, fileBuffer, seqNum, file_read_count) == 0) {
							// Create a Helper Named SendWithRetry
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
																   (struct sockaddr *)&their_addr, &addr_len)) == -1 ||
										received_count != 4) {
										perror("recvfrom:ACK");
										exit(1);
									}
									if(parseACK(&received_seq_num, ackBuf, 4) == 0) {
										break;
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
						file_read_count = read(fd, fileBuffer, MAXSENDBUFLEN);
					} else {
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
																   (struct sockaddr *)&their_addr, &addr_len)) == -1 ||
										received_count != 4) {
										perror("recvfrom:ACK");
										exit(1);
									}
									if(parseACK(&received_seq_num, ackBuf, 4) == 0) {
										break;
									}
								}
							}
						}
						printf("Success at sending %d packets DATA.\n", seqNum);
						break;
					}
				}
			} else if(strcmp(mode, NETASCII) == 0) {

			} else {

			}


			printf("Will close connection \n");
			close(newfd);
			exit(0);
		}

		// Parent Process does not need this
		close(newfd);
	}

	close(sockfd);
	return 0;
}




