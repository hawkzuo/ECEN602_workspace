//
// Created by Jianyu Zuo on 10/5/17.
//


#include "HTTP.h"


int buildHTTPRequest(char** message, int* message_count,char* rawInput)
{
    size_t len = strlen(rawInput);
    *message_count = (int) len;
    if(len <=5 ) {
        return -1;
    } else {
        if(memcmp(rawInput, "http", 4) != 0 && memcmp(rawInput, "https", 5) != 0) {
            // Unsupported HTTP request
            return -2;
        }
        *message = malloc((len+1)*sizeof(char));
        strncpy(*message, rawInput, len);
        return 0;
    }
}

int parseHTTPRequest(char buffer[], ssize_t message_len, char **host, char **resource)
{
    int hostStartIndex;
    int hostEndIndex = -1;
    int resourceStartIndex = -1;
    if(memcmp(buffer, "http://", 7) == 0) {
        hostStartIndex = 7;
    } else if(memcmp(buffer, "https://", 8) == 0) {
        hostStartIndex = 8;
    } else {
        return -1;
    }

    for(int i=hostStartIndex; i<message_len; i++) {
        if(buffer[i] == '/') {
            hostEndIndex = i-1;
            resourceStartIndex = i+1;
            break;
        }
    }

    if(hostEndIndex == -1 || resourceStartIndex == -1) {
        return -1;
    }

    *host = malloc((hostEndIndex-hostStartIndex+2)*sizeof(char));
    *(*host + hostEndIndex - hostStartIndex + 1) ='\0';
    *resource = malloc((message_len-resourceStartIndex+1)*sizeof(char));
    *(*resource + message_len - resourceStartIndex) ='\0';

    for(int i=hostStartIndex; i<=hostEndIndex; i++) {
        *(*host + i - hostStartIndex) = buffer[i];
    }
    strncpy(*resource, buffer+resourceStartIndex, message_len-resourceStartIndex);

    return 0;

}

int receiveFromGET(char* host,
                   char* resource,
                   struct LRU_node cache[MAXUSERCOUNT],
                   int* valid_LRU_node_count,
                   int64_t* LRU_counter,
                   int staledCacheIndex)
{
    // Strcture declaration for initialization of connection.
    struct addrinfo hints, *serverinfo, *p;
    int rv; // Short for Return Value
    ssize_t received_count;
    int socket_fd = -1;
    char receive_buffer[HTTPRECVBUFSIZE];

    char httpMessage[128];
    memset(&httpMessage, 0, sizeof(httpMessage));
    sprintf(httpMessage,"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", resource, host);


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, HTTPPORT, &hints, &serverinfo)) != 0) {
        fprintf(stderr, "receiveFromGET-getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = serverinfo; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("receiveFromGET: socket");
            continue;
        }
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("receiveFromGET: connect");
            continue;
        }
        break;
    }

    if (p == NULL || socket_fd == -1) {
        fprintf(stderr, "receiveFromGET: failed to connect\n");
        return -2;
    }

    // Create file to store
    int read_fd = open(concat_host_res(host, resource), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(read_fd == -1) {
        perror("receiveFromGET: createCacheFile");
        return -1;
    }

    // Use strlen here
    ssize_t send_count = writen(socket_fd, httpMessage, strlen(httpMessage));
    if(send_count < 0) {
        perror("eceiveFromGET: sendToHost");
        return -1;
    }

    fd_set master;                      // master file descriptor list
    fd_set read_fds;                    // temp file descriptor list for select()
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &master);
    struct timeval tv;
    tv.tv_sec = IDLETIME;
    tv.tv_usec = 0;
    int totalBytes = 0;
    int cacheRequired = 0;
    // Build A Node
    struct LRU_node node;

    while(1) {
        read_fds = master; // copy it

        if (select(socket_fd+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("receiveFromGET: select");
            exit(4);
        }
        // Read data:
        // Notice that here must use this implementation
        // That's because server can send data less than 256 Bytes

        if(FD_ISSET(socket_fd, &read_fds)) {
            received_count = recv(socket_fd, receive_buffer, HTTPRECVBUFSIZE , 0);
            if(received_count <= 0){
                fprintf(stdout, "%zi bytes received\n", received_count);
            }

            write(read_fd, receive_buffer, (size_t) (received_count));
            if (totalBytes == 0) {
                // First Frame, parse the Header
                const char* ptr = receive_buffer;
                while (ptr[0]) {
                    char buffer[100];
                    int n;
                    sscanf(ptr, " %99[^\r\n]%n", buffer, &n); // note space, to skip white space
                    ptr += n; // advance to next \n or \r, or to end (nul), or to 100th char
                    // ... process buffer
                    if ( memcmp(buffer, "Date: ", 6) == 0) {
                        // Parse date first
                        size_t date_len = strlen(buffer) + 1 - 6;
                        char* date = malloc((date_len)*sizeof(char));
                        *(date+date_len-1) = '\0';
                        strncpy(date, buffer+6, date_len-1);
                        node.receive_date = malloc(sizeof(struct tm));
                        strptime(date, "%a, %d %b %Y %X %Z", node.receive_date);
                    } else if ( memcmp(buffer, "Expires: ", 9) == 0) {
                        size_t date_len = strlen(buffer) + 1 - 9;
                        char* date = malloc((date_len)*sizeof(char));
                        *(date+date_len-1) = '\0';
                        strncpy(date, buffer+9, date_len-1);
                        node.expires_date = malloc(sizeof(struct tm));
                        strptime(date, "%a, %d %b %Y %X %Z", node.expires_date);
                    } else if ( memcmp(buffer, "Last-Modified: ", 15) == 0) {
                        size_t date_len = strlen(buffer) + 1 - 15;
                        char* date = malloc((date_len)*sizeof(char));
                        *(date+date_len-1) = '\0';
                        strncpy(date, buffer+15, date_len-1);
                        node.modified_date = malloc(sizeof(struct tm));
                        strptime(date, "%a, %d %b %Y %X %Z", node.modified_date);
                    }
                }
            }

            memset(&receive_buffer, 0, sizeof receive_buffer);
            totalBytes += received_count;
            if(received_count == 0) {
                break;
            }
        } else {
            // TimeOut
            break;
        }
    }

    if(node.modified_date == NULL && node.expires_date == NULL) {
        cacheRequired = 0;
    } else {
        cacheRequired = 1;
    }


    if(staledCacheIndex != -1) {
        // Change Staled Node
        if(cacheRequired == 1) {
            // Replace the node
            // Step 1: create the node
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;
            // Step 2: update node
            cache[staledCacheIndex] = node;
        } else {
            // Delete the node
            cache[staledCacheIndex] = cache[(*valid_LRU_node_count-1)];
            memset(&cache[(*valid_LRU_node_count-1)], 0, sizeof(cache[(*valid_LRU_node_count-1)]));
            *(valid_LRU_node_count) -= 1;
        }
    } else {
        if(*valid_LRU_node_count == MAXCACHECOUNT && cacheRequired == 1) {
            // Imp LRU shifting
            // Find the lowest priority one and replace it. RT: O(n)

            // Step1: Find the lowest priority node Or assign it to 'stale' node

            int64_t leastPriorityIndex = 0;
            int64_t leastPriorityValue = cache[0].priority;
            for(int i=1; i<MAXCACHECOUNT; i++) {
                if(cache[i].priority < leastPriorityValue) {
                    leastPriorityIndex = i;
                }
            }

            // Step2: Create a Node at offset=MAXCACHECOUNT
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;

            // Step3: Swap & cleanup Node at offset=MAXCACHECOUNT
            if (remove(cache[leastPriorityIndex].filename) == 0) {
                printf("Removed old cache file %s, due to max cache count reached.\n", cache[leastPriorityIndex].filename);
            } else {
                perror("receiveFromGET: removeFile");
            }

            cache[leastPriorityIndex] = node;

        } else if (cacheRequired == 1) {
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;
            cache[*valid_LRU_node_count] = node;
            *(valid_LRU_node_count) += 1;
        } else {
            // Do not store cache
        }
    }


    // closing
    close(read_fd);
    close(socket_fd);

    return 0;
}

// writen n chars to the socket
ssize_t writen(int fd, void *vptr, size_t n)
{
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

char* concat_host_res(const char *host, const char *res)
{
    char *result = malloc(strlen(host)+strlen(res)+2);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, host);
    strcat(result, "_");
    strcat(result, res);
    return result;
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
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


// LRU Rule: Least Recently Used
/*
 * For rules of keeping/removing caches
 * Encoming URI:
 *   If Cached.hasKey(filename):
 *      If has Expires && not yet expired => Send Back Directly
 *      Elseif {
 *          Date is within 24 hours && Last-Modified is within 1 month => Send Back Directly
 *      }
 *   Else:
 *      Fetch the file immediately
 *      Do not cache files missing both Expires & Last-Modified Header
 *  Move Entry to 1st priority
 */

//                    char timeBuffer[80];
//                    time_t curtime;
//                    struct tm *info;
//                    time(&curtime);
//                    info = gmtime(&curtime );
//                    strftime(timeBuffer,80,"%c", info);
//                    printf("Current time = %s", timeBuffer);
//
//                    // Reverse Process
//                    struct tm tm;
//                    char tmBuf[80];
//
//                    memset(&tm, 0, sizeof(struct tm));
//                    strptime("Sat Nov 18 05:13:15 2017", "%c", &tm);
//                    strftime(tmBuf,80,"%c", &tm);
//                    printf("Current time = %s", tmBuf);
//
//                    double diff = difftime(mktime(info), mktime(&tm));
//                    printf("\nDiff Seconds = %f", diff);


