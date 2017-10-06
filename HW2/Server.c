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
//#include "SBCP.h"

#define BACKLOG 10 // how many pending connections queue will hold
#define PROTOCOL "echos"
#define MAXDATASIZE 1000 // max number of characters in a string we can send/get at once, including the '\n' char



void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// writen n chars to the socket
ssize_t writen(int fd, void *vptr, size_t n) {
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





int main(int argc, char** argv)
{
	int socket_fd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;


	char ip4[INET_ADDRSTRLEN]; 

	// For child process use
	ssize_t number_read;
	char buf[MAXDATASIZE+1];


	/*	This part checks the input format and record the assigned port number		*/
	if(argc != 3) {
		fprintf(stderr, "Number of arguments error, expected: 3, got: %d\n", argc);
		exit(1);
	} else if( strcmp(argv[1], PROTOCOL) != 0) {
		fprintf(stderr, "Unsupported protocol, expected: echos, got: %s\n", argv[1]);
		exit(1);
	}

	/* This part do the IP address look-up, creating socket, and bind the port process */

	memset(&hints, 0, sizeof hints);	// Reset to zeros
	hints.ai_family = AF_UNSPEC;			// use IPv4 for HW1
	hints.ai_socktype = SOCK_STREAM;	// TCP
	hints.ai_flags = AI_PASSIVE; 		// use my IP
	if ((rv = getaddrinfo(NULL, argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if(p->ai_family != AF_INET) {
			continue;
		}
		if ((socket_fd = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(socket_fd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	/* This part starts the listening process 	*/
	if (listen(socket_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	} else {
		// get_in_addr((struct sockaddr *)&their_addr)
		inet_ntop(AF_INET, get_in_addr(p->ai_addr) , ip4, INET_ADDRSTRLEN);
		printf("The server starts listening at %s, at port %s.\n", ip4, argv[2] );
	}

	/*	Multi-process settings */
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: max support characters(including enter char): %d. Waiting for connections...\n", MAXDATASIZE);

	// main accept() loop
	while(1) { 
		sin_size = sizeof their_addr;
		new_fd = accept(socket_fd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				  get_in_addr((struct sockaddr *)&their_addr),
				  s, sizeof s);
		printf("server: got connection from %s\n", s);
        if(!fork()) {
			close(socket_fd); // socket_fd is used later for the new connections, start with 1 client first
            while ((number_read = recv(new_fd, buf, sizeof(buf), 0)) > 0) {
//                printf("Received string length: %zu\n", strlen(buf));
                printf("Number Read: %zu\n", number_read);

                for(int i=0; i<number_read; i++) {
                    printf("Binary: %s\n", byte_to_binary(buf[i]));
                }

//		        if((int)writen(new_fd, buf, strlen(buf)) != strlen(buf)) {
//		            fprintf(stderr,
//		                "Encounter an error sending lines. Expected:%zi\n", number_read);
//		            printf("Closing connection...\n");
//					close(new_fd);
//					printf("Child process ended.\n");
//					exit(0);
//		        } else {
//		            printf("Sent input string back to the client.\n");
//		        }


                memset(&buf, 0, sizeof buf);
            }
            if (number_read < 0) {
                perror("receive");
            }
            printf("Closing connection...\n");
            close(new_fd);
            printf("Child process ended.\n");
            exit(0);
        }
	}
	return 0;
}


