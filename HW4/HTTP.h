//
// Created by Jianyu Zuo on 10/5/17.
//

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#ifndef ECEN602_WORKSPACE_HTTP_H
#define ECEN602_WORKSPACE_HTTP_H


// Maximum supported User Number
#define MAXUSERCOUNT 15
#define HTTPRECVBUFSIZE 256
#define IDLETIME 1

static char* HTTPPORT = "80";

int buildHTTPRequest(char** message, int* message_count,char* rawInput);
char* concat(const char *s1, const char *s2);
int parseHTTPRequest(char buffer[], ssize_t message_len, char **host, char **resource);
int receiveFromGET(char* host, char* resource, char* httpMessage);
ssize_t writen(int fd, void *vptr, size_t n);
const char *byte_to_binary(int x);


struct CSPair {
    int client_fd;
    int server_fd;
};


#endif //ECEN602_WORKSPACE_HTTP_H


