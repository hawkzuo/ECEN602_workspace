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


/**
 * Perform retrieving resources from given URI via sending GET request
 * @param host 		                -Host Address, example: "www.example.com"
 * @param resource  		        -Resource Address, example: "index.html"
 * @param cache[MAXUSERCOUNT]	    -Data Structure for LRU cache
 * @param valid_LRU_node_count		-Count of current valid LRU nodes
 * @param LRU_counter               -Global Counter used for Priority Management
 * @param staledCacheIndex          -Indicate if this request is an update for a staled cache
 * @return	0 on success, -1 on getaddrinfo error, -2 on connect error,
 *          -3 on send error, -4 on file read error,
 */
int receiveFromGET(char* host,
                   char* resource,
                   struct LRU_node cache[MAXUSERCOUNT],
                   int* valid_LRU_node_count,
                   int64_t* LRU_counter,
                   int staledCacheIndex)
{
    // Initial some useful variables
    struct addrinfo hints, *serverinfo, *p;
    int rv;                     // Short for Return Value
    ssize_t received_count;
    int socket_fd = -1;
    char receive_buffer[HTTPRECVBUFSIZE];

    // Generate GET request body
    char httpMessage[128];
    memset(&httpMessage, 0, sizeof(httpMessage));
    sprintf(httpMessage,"GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", resource, host);
    // Generate BONUS GET request body
    char httpMessageWithHeader[256];

    // Open cache file
    int read_fd = open(concat_host_res(host, resource), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(read_fd == -1) {
        perror("receiveFromGET: createCacheFile");
        return -4;
    }

    /* Connection Establishment */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, HTTPPORT, &hints, &serverinfo)) != 0) {
        fprintf(stderr, "receiveFromGET-getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
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

    /* Send the GET request message to the remote Host  */
    ssize_t send_count = writen(socket_fd, httpMessage, strlen(httpMessage));
    if(send_count < 0) {
        perror("receiveFromGET: sendToHost");
        return -3;
    }

    /* Read Data & Store Cache */
    fd_set master;                      // master file descriptor list
    fd_set read_fds;                    // temp file descriptor list for select()
    int totalBytes = 0;                 // Used to identify header
    int cacheRequired = 0;              // Flag on cache storing
    struct LRU_node node;               // New LRU Node
    struct timeval tv;                  // Timeout time_value
    tv.tv_sec = IDLETIME;
    tv.tv_usec = 0;
    FD_ZERO(&master);                   // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &master);

    while(1) {
        read_fds = master;              // copy it

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
            // Save to the file first
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

            // Clear buffer for the next loop
            memset(&receive_buffer, 0, sizeof receive_buffer);
            totalBytes += received_count;
            if(received_count == 0) {
                // EOF detected, data receive process is finished
                break;
            }
        } else {
            // TimeOut
            break;
        }
    }

    // Check if this node is qualified to be stored
    if(node.modified_date == NULL && node.expires_date == NULL) {
        cacheRequired = 0;
    } else {
        cacheRequired = 1;
    }

    // Check if this action is requested by staled cache refreshment
    if(staledCacheIndex != -1) {
        // New node should be stored, so replace old stale node
        if(cacheRequired == 1) {
            // Step 1: create the node
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;
            // Step 2: update node
            cache[staledCacheIndex] = node;
        } else {
            // New node is not needed, so delete old stale node
            cache[staledCacheIndex] = cache[(*valid_LRU_node_count-1)];
            memset(&cache[(*valid_LRU_node_count-1)], 0, sizeof(cache[(*valid_LRU_node_count-1)]));
            // Reduce valid_LRU_node_count by 1
            *(valid_LRU_node_count) -= 1;
        }
    } else {
        if(*valid_LRU_node_count == MAXCACHECOUNT && cacheRequired == 1) {
            // Imp LRU shifting
            // Find the lowest priority one and replace it. RT: O(n)

            // Step1: Find the lowest priority node
            int64_t leastPriorityIndex = 0;
            int64_t leastPriorityValue = cache[0].priority;
            for(int i=1; i<MAXCACHECOUNT; i++) {
                if(cache[i].priority < leastPriorityValue) {
                    leastPriorityIndex = i;
                }
            }

            // Step2: Construct full node
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;

            // Step3: Swap & cleanup Node
            if (remove(cache[leastPriorityIndex].filename) == 0) {
                fprintf(stdout, "Removed old cache file '%s'\n  Reason: MaxCacheCount, delete least recently used.\n", cache[leastPriorityIndex].filename);
            } else {
                perror("receiveFromGET: removeFile");
            }

            cache[leastPriorityIndex] = node;

        } else if (cacheRequired == 1) {
            // Not Full, simply add this node to cache Array
            node.priority = *LRU_counter;
            node.filename = concat_host_res(host, resource);
            *(LRU_counter) += 1;
            cache[*valid_LRU_node_count] = node;
            *(valid_LRU_node_count) += 1;
        } else {
            // cache node does not require storing
        }
    }
    // closing
    fprintf(stdout, "Successfully finished retrieving resource from host.\n");
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


