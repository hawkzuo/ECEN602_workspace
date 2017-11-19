/*
** server.c -- a stream socket server demo
*/

// Test for www.example.com first
// Then test for www.tamu.edu/index.html
#include "HTTP.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


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
    int i,rv;

    struct addrinfo hints, *servinfo, *p;

    // HW4

    // LRU data structure
    struct LRU_node cache[MAXUSERCOUNT];
    int valid_LRU_node_count = 0;
    int64_t global_LRU_priority_value = 0;

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
    fprintf(stdout, "Maximum cache file count is: %d\n", MAXCACHECOUNT);


    memset(&buf, 0, sizeof buf);
    // Main loop
    while(1) {
        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(listener, &read_fds)) {
            // handle new connections

            addrlen = sizeof remoteaddr;
            newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
            if (newfd == -1) {
                perror("accept");
            } else {
                FD_SET(newfd, &master); // add to master set
                if (newfd > fdmax) { // keep track of the max
                    fdmax = newfd;
                }
                printf("Proxy: new connection from %s on "
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
                    // got error or connection closed by client clean up resources
                    if(received_count < 0) {
                        perror("Proxy: recvFromClient");
                    }
                } else if(newfd == i) {
                    // Version 1: Send GET request & receive data
                    char* host;
                    char* resource;
                    if(parseHTTPRequest(buf, received_count, &host, &resource) != 0) {
                        perror("Proxy: parseHTTP");
                    }
                    // Print Out Host & Resource
                    printf("Host:%s\n", host);
                    printf("Resource:%s\n", resource);
                    // Check Cache 1st, then send GET

                    char* desiredCacheFilename = concat_host_res(host, resource);
                    struct LRU_node desiredNode;
                    int cached = 0;
                    int staledCacheIndex = -1;
                    for(int k=0; k<valid_LRU_node_count; k++) {
                        if(strcmp(desiredCacheFilename, cache[k].filename) == 0) {
                            // Check for Time Conditions
                            time_t curtime;
                            time(&curtime);

                            if( ((cache[k].expires_date != NULL && mktime(cache[k].expires_date) - curtime > 0) ||
                                (cache[k].receive_date != NULL && cache[k].modified_date != NULL &&
                                 curtime - mktime(cache[k].receive_date) <= ONEDAY &&
                                 curtime - mktime(cache[k].modified_date) <= ONEMONTH))  ) {
                                // cache is valid
                                desiredNode = cache[k];
                                cached = 1;
                                // Refresh Priority
                                desiredNode.priority = global_LRU_priority_value;
                                global_LRU_priority_value += 1;
                                fprintf(stdout, "Cache is valid for this request, will send cached data back.\n");
                            } else {
                                // cache is 'stale', remove it
                                staledCacheIndex = k;
                                fprintf(stdout, "Cache is stored for this request, but is stale, will send GET request.\n");
                                if (remove(cache[staledCacheIndex].filename) == 0) {
                                    fprintf(stdout, "Removed old cache file '%s'\n  Reason: stale.\n", cache[staledCacheIndex].filename);
                                }
                            }

                            break;
                        }
                    }
                    int receiveFromGETFlag = 0;
                    // Not Cached locally, send GET request
                    if (cached == 0) {
                        receiveFromGETFlag = receiveFromGET(host, resource, cache, &valid_LRU_node_count, &global_LRU_priority_value, staledCacheIndex);
                        if(receiveFromGETFlag < 0) {
                            perror("Proxy: receiveGET");
                        }
//                        desiredNode = cache[valid_LRU_node_count-1];
                    }

                    // open the file and send back
                    char fileBuffer[HTTPRECVBUFSIZE];

                    // At this time, no matter read from cache or after sending GET request,
                    // desiredCacheFilename should always be present at file system
                    int cached_file_fd = open(desiredCacheFilename, O_RDONLY);
                    ssize_t file_read_count	= read(cached_file_fd, fileBuffer, HTTPRECVBUFSIZE);
                    if(file_read_count < 0) {
                        perror("Proxy: readCachedFile");
                    }
                    while(file_read_count >= 0 ) {
                        ssize_t send_count = writen(newfd, fileBuffer, file_read_count);
                        if(send_count < 0) {
                            perror("Proxy: writenToClient");
                        }
                        memset(&fileBuffer, 0, sizeof fileBuffer);
                        file_read_count	= read(cached_file_fd, fileBuffer, HTTPRECVBUFSIZE);
                        if(file_read_count == 0) {
                            break;
                        }
                    }
                    // Close the cached file
                    close(cached_file_fd);

                    // Check for "Proxy will not store cache" case
                    if(receiveFromGETFlag == 11) {
                        remove(desiredCacheFilename);
                    }


                }
                // Clean Up resources on Server
//                sleep(1);
                close(i); // bye!
                FD_CLR(i, &master); // remove from master set
                printf("Proxy: finished sending file and closed connection on socket %d\n\n", i);
                newfd = 0;
                memset(&buf, 0, sizeof buf);
            }
        }
    }
}


