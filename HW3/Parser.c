#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>


//Global prevChar
char prevChar;
int eof;
// \r\n => \n
// \r\0 => \r
// If I encounter \r I won't read


int convertASCII(FILE *fp, char *ptr, int maxnbytes)
{
    int buffer_size = 0;

    for(int count=0; count < maxnbytes; count++) {
        int c = getc(fp);
        if(prevChar == '\r') {
            if (c == '\n') {
                c = '\n';
                prevChar = -1;
            } else if (c == '\0') {
                c = '\r';
                prevChar = -1;
            } else {
                // \r must have either \n or \0 behind, if not, the file is in error format
                return -1;
            }
        } else {
            // Previous char is not \r
            if(c == EOF) {
                if(ferror(fp)) {
                    perror("read err on getc");
                }
                eof = 1;
                return buffer_size;
            } else if (c == '\r') {
                // Special char encountered, record to prevChar and continue loop
                prevChar = '\r';
                continue;
            } else {
                // Normal char do nothing
            }
        }
        *ptr++ = (char) c;
        buffer_size ++;
    }
    return buffer_size;
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: Parser filename converted_filename \n");
        fprintf(stderr,"Number of arguments error, expected: 3, got: %d\n", argc);
        exit(1);
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(2);
    }

    char* old_fullname = concat(concat(cwd, "/"), argv[1]);

    FILE* old_fp;
    old_fp = fopen(old_fullname, "r");
    if(old_fp==NULL) {
        fprintf(stderr, "No such file!");
        return 3;
    }

    char* new_fullname = concat(concat(cwd, "/"), argv[2]);

    int new_fd = open(new_fullname, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    char buffer[555];

    int flag;

    prevChar = -1;
    eof = 0;

//    flag = convertASCII(old_fp, buffer, 512);
    while(1) {
        flag = convertASCII(old_fp, buffer, 512);
        if (flag < 0) {
            exit(15);
        } else {
            write(new_fd, buffer, flag);
            if(eof == 1) {
                // Stop
                break;
            }
        }
        memset(&buffer, 0, sizeof(buffer));
    }

    close(new_fd);
    fclose(old_fp);
    return 0;

}

