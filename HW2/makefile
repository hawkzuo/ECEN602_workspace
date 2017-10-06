all: Server Client readFast.o

Server: Server.c readFast.o SBCP.o
	gcc -o server Server.c readFast.o SBCP.o -std=gnu99

Client: Client.c readFast.o SBCP.o
	gcc -o client Client.c readFast.o SBCP.o -std=gnu99
	
SBCP.o: SBCP.c	
	gcc -c SBCP.c -std=gnu99

readFast.o: readFast.c 
	gcc -c readFast.c 

clean:
	rm *.o
	rm server
	rm client


