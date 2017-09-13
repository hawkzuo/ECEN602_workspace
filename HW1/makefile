all: Server Client readFast.o

Server: Server.c readFast.o
	gcc -o Server Server.c readFast.o

Client: Client.c readFast.o
	gcc -o Client Client.c readFast.o

readFast.o: readFast.c
	gcc -c readFast.c 

clean:
	rm -rf *.o


