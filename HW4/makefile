all: Proxy Client HTTP.o

Proxy: Proxy.c HTTP.o
	gcc -o Proxy Proxy.c HTTP.o -std=gnu99

Client: Client.c HTTP.o
	gcc -o Client Client.c HTTP.o -std=gnu99

HTTP.o: HTTP.c
	gcc -c HTTP.c -std=gnu99

clean:
	rm *.o
	rm Proxy
	rm Client

