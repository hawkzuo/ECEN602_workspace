/*
** client.c -- a stream socket client demo
*/
// CLion cmake demo: https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "SBCP.h"

#define MAXDATASIZE 1000 // max number of characters in a string we can send/get at once, including the '\n' char
#define IDLETIME 10     // seconds for clients indicating the idle process

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Write chars to the socket */
ssize_t writen(int fd, void *vptr, size_t n) {
    size_t num_char_left;
    ssize_t num_char_written;
    char *ptr;

    ptr = vptr;
//    printf("converted version of msg: %s\n", ptr);
    num_char_left = n;
    while (num_char_left > 0) {
        // write is system call
        if( (num_char_written = write(fd, ptr, num_char_left)) <= 0) {
            // If error is EINTR, we try looping again
            if(num_char_written <0 && errno == EINTR) {
                num_char_written = 0;
            } else {
                // Other error types, will quit
                return -1;
            }
        }
        num_char_left -= num_char_written;
        ptr += num_char_written;
    }
    return n;
}

/* Binary View Helper */
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


int main(int argc, char *argv[])
{
    // socket_fd: The socket file descriptor
    int socket_fd=-1;

    // buf: User input buffer
    // MAXDATASIZE is the acceptable input size, including the '\n' char.
    char buf[MAXDATASIZE+1];

    // hints, *servinfo, *p: used for checking IP addresses
    struct addrinfo hints, *servinfo, *p;

    // rv: return value for system calls
    int rv;
    // s: server IP address
    char s[INET6_ADDRSTRLEN];


    int message_len;
    char bufRecv[MAXDATASIZE+1];


    // received_count: Count of chars received from the server
    ssize_t received_count;
    // message_type: the received type of message
    int message_type;


    // Client Username
    char* username;


    /* Input checking */
    if (argc != 4) {
        fprintf(stderr,"Number of arguments error, expected: 4, got: %d\n", argc);
        exit(1);
    } else  {
        username = argv[1];
        printf("User name is %s\n", username);
    }

    /* IP searching process */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    /* Client connecting process */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype,
             p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
        }
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL || socket_fd == -1) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

//     1st: Send JOIN to server.
    struct SBCPMessage *join_msg= (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
    int msg_length = generateJOIN(join_msg, username);
    if(msg_length == -1) {
        perror("client: username length");
        exit(1);
    }
    int join_rv = send_message(join_msg, msg_length, socket_fd);
    if(join_rv == -1) {
        perror("client: JOIN");
        exit(1);
    }
    free(join_msg);


    fd_set master;
    FD_ZERO(&master);
    struct timeval tv;
    tv.tv_sec = IDLETIME;

    while(1)  {
        FD_SET(0, &master);
        FD_SET(socket_fd, &master);
        int start_time_msec = (int) (clock() * 1000 / CLOCKS_PER_SEC);
        select(1+socket_fd, &master, NULL, NULL, &tv);


        if(FD_ISSET(0, &master)) {
            // You have something in the terminal
            buf[MAXDATASIZE] = '\0';

            // fgets includes the '\n' char
            // If an EOF char detected, we'll break the loop & exit the loop
            if(!fgets(buf, sizeof(buf), stdin)) {
                break;
            }

            printf("Input string (no space):%s", buf);
            struct SBCPMessage *msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
            message_len = generateSEND(msg, buf);
            if( message_len != -1) {
                int send_rv = send_message(msg, message_len, socket_fd);
                if(send_rv == -1) {
                    perror("client: send message");
                }
            }

            printf("Please continue entering the message:\n");

            free(msg);
            memset(&buf, 0, sizeof buf);
            tv.tv_sec = IDLETIME;


        } else if (FD_ISSET(socket_fd, &master)){
            // You have messages waiting to be displayed in the terminal
            received_count = recv(socket_fd, bufRecv, sizeof(bufRecv), 0);
            if(received_count <= 0) {
                if(received_count < 0) {
                    perror("client: recv");
                } else {
                    printf("Server closed connection. Program will end.\n");
                    break;
                }
            }


            // 1st: Check the number of bytes received is correct
            if(received_count >= 4 && (((uint8_t)bufRecv[2] * 256) + (uint8_t)bufRecv[3] == received_count) ) {
                message_type = bufRecv[1] & 0x7f;

//                printf("Number Bytes Recv: %zu\n", received_count);

                char * user;
                char * messageRecv;
                char * nakReason;
                char* users[MAXUSERCOUNT];
                memset(&users, 0, sizeof users);
                int client_count;
                switch(message_type) {
                    case ACK:
                        if( parseACK(bufRecv, &client_count, users) != -1) {
                            printf("ACK Received. Current users count: %d\n", client_count);
                            for(int i=0; i<client_count; i++) {
                                printf("\tACK: User '%s' is online. \n", users[i]);
                            }
                        }
                        break;
                    case FWD:
                        if( parseFWD(bufRecv, &messageRecv, &user) != -1 ) {
                            printf("FWD:\n\t User '%s' says: \n%s", user, messageRecv);
                            free(user);
                            free(messageRecv);
                        }
                        break;
                    case NAK:
                        if( parseNAK(bufRecv, &nakReason) != -1 ) {
                            printf("NAK:\n\tServer refused connection and says: \n%s\n", nakReason);
                            free(nakReason);
                        }
                        break;
                    case ONLINE:
                        if( parseONLINE(bufRecv, &user) != -1) {
                            printf("ONLINE:\n\t User '%s' is online.\n", user);
                            free(user);
                        }
                        break;
                    case OFFLINE:
                        if( parseOFFLINE(bufRecv, &user) != -1) {
                            printf("OFFLINE:\n\t User '%s' is offline.\n", user);
                            free(user);
                        }
                        break;
                    case IDLE:
                        if( parseSERVERIDLE(bufRecv, &user) != -1) {
                            printf("IDLE:\n\t User '%s' is idle.\n", user);
                            free(user);
                        }
                        break;
                    default:break;
                }
                if(message_type != NAK) {
                    printf("Please continue entering the message:\n");
                }
            }

            memset(&bufRecv, 0, sizeof bufRecv);



            int time_passed_msec = (int) (clock() * 1000 / CLOCKS_PER_SEC) - start_time_msec;
            tv.tv_sec = tv.tv_sec - time_passed_msec / 1000;
        } else {
            // Timeout
            printf("Input timeout. Will send IDLE to server.\n");
            struct SBCPMessage *idle_msg = (struct SBCPMessage*)malloc(sizeof(struct SBCPMessage));
            int idle_message_len = generateCLIENTIDLE(idle_msg);
            if( idle_message_len != -1) {
                int send_rv = send_message(idle_msg, idle_message_len, socket_fd);
                if(send_rv == -1) {
                    perror("client: send message");
                }
            }

            free(idle_msg);

            tv.tv_sec = IDLETIME;
        }

        if(socket_fd > 10000) {
            break;
        }
        FD_ZERO(&master);
    }

    /* End of Program */
//    printf("Out of loop. Program will end. \n");
    close(socket_fd);

    return 0;
}
