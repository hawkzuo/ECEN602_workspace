#How to compile?
gcc -c readFast.c
gcc -c Client.c
gcc -c Server.c
gcc -o server Server.o
gcc -o client Client.o readFast.o


