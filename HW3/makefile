all: Server Client TFTP.o

Server: Server.c TFTP.o
	gcc -o server Server.c TFTP.o -std=gnu99

Client: Client.c TFTP.o
	gcc -o client Client.c TFTP.o -std=gnu99

TFTP.o: TFTP.c
	gcc -c TFTP.c -std=gnu99

clean:
	rm *.o
	rm client
	rm server


