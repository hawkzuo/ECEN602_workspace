all: Server Client

Server: Server.c TFTP.o
	gcc -o Server Server.c TFTP.o

Client: Client.c TFTP.o
	gcc -o Client Client.c TFTP.o

TFTP.o: TFTP.c
	gcc -c TFTP.c

clean:
	rm *.o


