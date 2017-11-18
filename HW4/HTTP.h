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
#define HTTPRECVBUFSIZE 512
#define IDLETIME 1

static char* HTTPPORT = "80";
struct CSPair {
    int client_fd;
    int server_fd;
};

struct LRU_node {
    char* filename;
    struct tm* receive_date;
    struct tm* expires_date;
    struct tm* modified_date;
    int64_t priority;
    struct LRU_node* next;
};

int buildHTTPRequest(char** message, int* message_count,char* rawInput);
int parseHTTPRequest(char buffer[], ssize_t message_len, char **host, char **resource);
int receiveFromGET(char* host, char* resource, struct LRU_node cache[MAXUSERCOUNT], int* valid_LRU_node_count, int64_t* LRU_counter);

ssize_t writen(int fd, void *vptr, size_t n);
char* concat_host_res(const char *host, const char *res);
char* concat(const char *s1, const char *s2);
const char *byte_to_binary(int x);




#endif //ECEN602_WORKSPACE_HTTP_H


