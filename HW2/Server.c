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
#include "SBCP.h"

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1000 // max number of characters in a string we can send/get at once, including the '\n' char



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

/* Send Message to fd, all types of msg supported*/
int send_message(struct SBCPMessage *message, int msg_length, int fd)
{
    char buffer[msg_length];
    if(createRawData(buffer, message, msg_length) != 0) {
        perror("Send Message: raw data");
        return -1;
    }

    ssize_t send_count = writen(fd, buffer, sizeof(buffer));
    if(send_count != sizeof(buffer)) {
        perror("Send Message: writen");
        return -1;
    } else {
//        printf("Successfully sent %zi bytes.\n", send_count);
    }
    return 0;
}



int main(int argc, char** argv)
{


    fd_set master;                  // master file descriptor list
    fd_set read_fds;                // temp file descriptor list for select()
    int fdmax;                      // maximum file descriptor number
    int listener = -1;                   // listening socket descriptor
    int newfd = 0;                      // newly accept()ed socket descriptor

    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[MAXDATASIZE+1]; // buffer for client data
    char remoteIP[INET6_ADDRSTRLEN];
    ssize_t received_count;
    int message_type = -1;  // Dummy init value

    int yes=1;
    int i,j,rv;

    struct addrinfo hints, *servinfo, *p;


    // These data structures are used to store Connected users in the chat room.
    char* usernames[MAXUSERCOUNT];
    int current_user_count = 0;
    int userstatus[MAXUSERCOUNT];
    int fdtable[MAXUSERCOUNT];

    // These are pre-defined NAK reasons
    char *nak_full = "Room is Full. \n";
    char *nak_duplicate = "Username already exists. \n";

    // These are names of each message type.
    char *message_types[10] = {"","","JOIN","FWD","SEND","NAK","OFFLINE","ACK","ONLINE","IDLE"};

    /* Main program starts here */

    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);

	/*	This part checks the input format and record the assigned port number		*/
	if(argc != 4) {
		fprintf(stderr, "Number of arguments error, expected: 4, got: %d\n", argc);
		exit(1);
	}

	/* This part do the IP address look-up, creating socket, and bind the port process */

	memset(&hints, 0, sizeof hints);	// Reset to zeros
	hints.ai_family = AF_UNSPEC;			// use IPv4 for HW1
	hints.ai_socktype = SOCK_STREAM;	// TCP
	hints.ai_flags = AI_PASSIVE; 		// use my IP
	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	int max_user = atoi(argv[3]);

    if(max_user > MAXUSERCOUNT - 1) {
        printf("The server can only support at most %d users. Please reenter the user count.\n", MAXUSERCOUNT-1);
        exit(10);
    }

	printf("Maximum number of user: %d. \n", max_user);
	

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL || listener == -1) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	/* This part starts the listening process 	*/
	if (listen(listener, BACKLOG) == -1) {
		perror("listen");
		exit(3);
	}

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // Basic Prompt:
    printf("Server started, chat room is open.\n");

    // Main loop
    while(1) {
        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *) &remoteaddr,
                                   &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                                       "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr *) &remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);
                    }

                } else {
                    // handle error case
                    received_count = recv(i, buf, sizeof buf, 0);
                    if (received_count <= 0) {
                        // got error or connection closed by client
                        if (received_count == 0) {
                            // connection closed
                            // Require extra checking here
                            int index = -1;
                            // 1. Find the index
                            for(int ii=0; ii<current_user_count;ii++) {
                                if(fdtable[ii] == i) {
                                    index = ii;
                                    break;
                                }
                            }
                            // 2. Remove index element & swap the last element & reduce count by 1
                            struct SBCPMessage *offline_msg = (struct SBCPMessage *) malloc(
                                    sizeof(struct SBCPMessage));
                            int offline_msg_len = generateOFFLINE(offline_msg, usernames[index]);

                            usernames[index] = usernames[current_user_count-1];
                            userstatus[index] = userstatus[current_user_count-1];
                            fdtable[index] = fdtable[current_user_count-1];
                            usernames[current_user_count-1] = NULL;
                            userstatus[current_user_count-1] = 0;
                            fdtable[current_user_count-1] = 0;
                            current_user_count --;

                            for (j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j != listener && j != i) {
                                        if (send_message(offline_msg, offline_msg_len, j) == -1) {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            free(offline_msg);

//                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        if(newfd == i) {
                            newfd = 0;
                        }

                        printf("selectserver: closed connection on socket %d\n", i);

                    } else if(newfd == i) {
                        // Meaning newly JOIN message should be received
                        
                        // 1st: Check the number of bytes received is correct
                        if(received_count >= 4 && (((uint8_t)buf[2] * 256) + (uint8_t)buf[3] == received_count) ) {
                            message_type = buf[1] & 0x7f;
                        }

                        printf("Number Bytes Recv: %zu. \tMessage Type: %s.\n", received_count, message_types[message_type]);

                        if(message_type != JOIN) {
                            // The client does not follow rules, close connection
                            close(i); // bye!
                            FD_CLR(i, &master); // remove from master set
                            printf("selectserver: closed connection on socket %d\n", i);

                        } else {
                            char * user;
                            if ( parseJOIN(buf, &user) != -1) {
                                if(current_user_count < max_user) {

                                    // Check Username dup first
                                    int hasDuplicate = 0;

                                    for(int ii=0; ii<current_user_count; ii++) {
                                        if(strcmp(usernames[ii], user) == 0) {
                                            hasDuplicate = 1;
                                            break;
                                        }
                                    }

                                    if(!hasDuplicate) {
                                        // Accept & send ACK back
                                        usernames[current_user_count] = user;
                                        userstatus[current_user_count] = ONLINE;
                                        fdtable[current_user_count] = i;
                                        current_user_count ++;

                                        // Send ACK back to this client
                                        if(current_user_count >= 1) {
                                            struct SBCPMessage *ack_msg = (struct SBCPMessage *) malloc(
                                                    sizeof(struct SBCPMessage));
                                            int ack_msg_len = generateACK(ack_msg, usernames);
                                            if ( send_message(ack_msg, ack_msg_len, i) == -1) {
                                                perror("send");
                                            }
                                            free(ack_msg);
                                        }

                                        // Send ONLINE to any other client
                                        struct SBCPMessage *online_msg = (struct SBCPMessage *) malloc(
                                                sizeof(struct SBCPMessage));
                                        int online_msg_len = generateONLINE(online_msg, user);

                                        for (j = 0; j <= fdmax; j++) {
                                            // send to everyone!
                                            if (FD_ISSET(j, &master)) {
                                                // except the listener and ourselves
                                                if (j != listener && j != i) {
                                                    if (send_message(online_msg, online_msg_len, j) == -1) {
                                                        perror("send");
                                                    }
                                                }
                                            }
                                        }
                                        free(online_msg);


                                    } else {
                                        // Reject & send NAK back
                                        struct SBCPMessage *nak_msg = (struct SBCPMessage *) malloc(
                                                sizeof(struct SBCPMessage));
                                        int nak_msg_len = generateNAK(nak_msg, nak_duplicate);
                                        if ( send_message(nak_msg, nak_msg_len, i) == -1) {
                                            perror("send");
                                        }
                                        free(nak_msg);
                                        close(i); // bye!
                                        FD_CLR(i, &master); // remove from master set
                                        printf("selectserver: closed connection on socket %d\n", i);
                                    }

                                } else {
                                    // Send NAK back & close connection
                                    struct SBCPMessage *nak_msg = (struct SBCPMessage *) malloc(
                                            sizeof(struct SBCPMessage));
                                    int nak_msg_len = generateNAK(nak_msg, nak_full);
                                    if ( send_message(nak_msg, nak_msg_len, i) == -1) {
                                        perror("send");
                                    }
                                    free(nak_msg);
                                    close(i); // bye!
                                    FD_CLR(i, &master); // remove from master set
                                    printf("selectserver: closed connection on socket %d\n", i);
                                }
                            }
                        }
                        newfd = 0;
                    } else {
                        // we got some data from a client
                        // 1st: Check the number of bytes received is correct
                        if(received_count >= 4 && (((uint8_t)buf[2] * 256) + (uint8_t)buf[3] == received_count) ) {
                            message_type = buf[1] & 0x7f;
                        }
                        printf("Number Bytes Recv: %zu. \tMessage Type: %s.\n", received_count, message_types[message_type]);

//                        printf("Received:\n");
//                        for(int ii=0; ii<received_count; ii++) {
//                            printf("Binary: %s\n", byte_to_binary(buf[ii]));
//                        }

                        char * sender;
                        char * messages;

                        // figure out which client send this message first
                        for(int ii=0; ii<current_user_count; ii++) {
                            if(fdtable[ii] == i) {
                                sender = usernames[ii];
                                break;
                            }
                        }

                        switch(message_type) {
                            case SEND:
                                if(parseSEND(buf, &messages) != -1) {
                                    struct SBCPMessage *fwd_msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
                                    int fwd_msg_len = generateFWD(fwd_msg, sender, messages);
                                    for (j = 0; j <= fdmax; j++) {
                                        // send to everyone!
                                        if (FD_ISSET(j, &master)) {
                                            // except the listener and ourselves
                                            if (j != listener && j != i) {
                                                if (send_message(fwd_msg, fwd_msg_len, j) == -1) {
                                                    perror("send");
                                                }
                                            }
                                        }
                                    }
                                    free(fwd_msg);
                                }
                                break;
                            case IDLE: {
                                struct SBCPMessage *sidle_msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
                                int sidle_msg_len = generateSERVERIDLE(sidle_msg, sender);
                                for (j = 0; j <= fdmax; j++) {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master)) {
                                        // except the listener and ourselves
                                        if (j != listener && j != i) {
                                            if (send_message(sidle_msg, sidle_msg_len, j) == -1) {
                                                perror("send");
                                            }
                                        }
                                    }
                                }
                                free(sidle_msg);
                            }
                                break;
                            default:break;
                        }
                    }


                    memset(&buf, 0, sizeof buf);

                }
            }
        }
    }
}


