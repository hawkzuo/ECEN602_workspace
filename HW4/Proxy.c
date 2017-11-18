/*
** server.c -- a stream socket server demo
*/

// Test for www.example.com first
// Then test for www.tamu.edu/index.html

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
#include "HTTP.h"

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


int main(int argc, char** argv)
{


    fd_set master;                      // master file descriptor list
    fd_set read_fds;                    // temp file descriptor list for select()
    int fdmax;                          // maximum file descriptor number
    int listener = -1;                  // listening socket descriptor
    int newfd = 0;                      // newly accept()ed socket descriptor

    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[MAXDATASIZE+1]; // buffer for client data
    char remoteIP[INET6_ADDRSTRLEN];
    ssize_t received_count;

    int yes=1;
    int i,j,rv;

    struct addrinfo hints, *servinfo, *p;


    // These data structures are used to store Connected users in the chat room.
    char* usernames[MAXUSERCOUNT];
    int current_user_count = 0;
    int userstatus[MAXUSERCOUNT];
    int fdtable[MAXUSERCOUNT];

    // HW4
    // Used for storing read/write fd for the same client
    // As long as receive from server_fd, send data to client_fd, then close these 2
    struct CSPair csPairs[MAXUSERCOUNT];
    // stores the fd value of each Client
    int client_fd_table[MAXUSERCOUNT];


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
    printf("Server started, Proxy is open.\n");


    memset(&buf, 0, sizeof buf);
    // Main loop
    while(1) {
        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(listener, &read_fds)) {
            // handle new connections, namely:

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
        }

        // run through the existing connections looking for data to read
        // Version 1: Try to recv all server results on one trigger event   =>  Success
        for(i = 0; i<= fdmax; i++) {
            if (FD_ISSET(i, &read_fds) && i != listener) {
                received_count = recv(i, buf, sizeof buf, 0);

                if(received_count <= 0) {
                    // got error or connection closed by client

                } else if(newfd == i) {
                    // Version 1: Send GET request & receive data
                    char* host;
                    char* resource;
                    if(parseHTTPRequest(buf, received_count, &host, &resource) != 0) {
                        perror("server: parseHTTP");
                    }
                    // Do something
                    printf("Host:%s\n", host);
                    printf("Resource:%s\n", resource);
                    char httpMessage[1000];
                    memset(&httpMessage, 0, sizeof(httpMessage));

                    sprintf(httpMessage,"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",resource,host);
//
                    if(receiveFromGET(host, resource, httpMessage) != 0) {
                        perror("server: receiveGET");
                    }
                    exit(16);
                    // Always send the cached version back to the client
                    // Update Cache



                    // can clean up too
                    newfd = 0;
                }

                memset(&buf, 0, sizeof buf);

            }
        }
    }
}


