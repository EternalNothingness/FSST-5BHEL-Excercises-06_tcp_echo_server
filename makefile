tcp-echo-server : tcp-echo-server.o
	gcc tcp-echo-server.o -o tcp-echo-server

tcp-echo-server.o : tcp-echo-server.c
	gcc -c tcp-echo-server.c -o tcp-echo-server.o

clean :
	rm -f tcp-echo-server.o
