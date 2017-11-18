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

int receiveFromGET(char* host, char* resource, char* httpMessage)
{
    // Strcture declaration for initialization of connection.
    struct addrinfo hints, *serverinfo, *p;
    int rv; // Short for Return Value
    ssize_t received_count;
    int socket_fd = -1;
    char receive_buffer[HTTPRECVBUFSIZE];

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
        perror("receiveFromGET: openfile");
        return -1;
    }


    // Use strlen here
    ssize_t send_count = writen(socket_fd, httpMessage, strlen(httpMessage));
    printf("Send Count: %zi", send_count);

    fd_set master;                      // master file descriptor list
    fd_set read_fds;                    // temp file descriptor list for select()
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &master);
    struct timeval tv;
    tv.tv_sec = IDLETIME;
    int totalBytes = 0;

    while(1) {
        read_fds = master; // copy it

        if (select(socket_fd+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
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
            if (totalBytes == 0) {
                // First Frame, parse the Header
//                parseHTTPHeader(receive_buffer, received_count, );
            }

            write(read_fd, receive_buffer, (size_t) (received_count));
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

//    do{
//        received_count = recv(socket_fd, receive_buffer, HTTPRECVBUFSIZE , 0);
//        if(received_count == HTTPRECVBUFSIZE) {
//
//        } else {
//            retry_times ++;
//        }
//        if(received_count <= 0){
//            fprintf(stdout, "%zi bytes received\n", received_count);
//        }
//        write(read_fd, receive_buffer, (size_t) (received_count));
//        memset(&receive_buffer, 0, sizeof receive_buffer);
//    }while(received_count == HTTPRECVBUFSIZE && retry_times <=3);

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



