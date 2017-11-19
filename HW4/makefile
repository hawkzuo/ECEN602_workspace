all: basicProxy proxy client HTTP.o

proxy: ProxyBonus.c HTTP.o
	gcc -o proxy ProxyBonus.c HTTP.o -std=gnu99

client: Client.c HTTP.o
	gcc -o client Client.c HTTP.o -std=gnu99

basicProxy: ProxyBasic.c HTTP.o
	gcc -o basicProxy ProxyBasic.c HTTP.o -std=gnu99

HTTP.o: HTTP.c
	gcc -c HTTP.c -std=gnu99

clean:
	rm *.o
	rm proxy
	rm client
	rm basicProxy

