all: Server Parser TFTP.o

Server: Server.c TFTP.o
	gcc -o Server Server.c TFTP.o -std=gnu99

Parser: Parser.c TFTP.o
	gcc -o Parser Parser.c TFTP.o -std=gnu99

TFTP.o: TFTP.c
	gcc -c TFTP.c -std=gnu99

clean:
	rm *.o
	rm Server
	rm Parser

