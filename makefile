CC=gcc
CFLAGS = -Wall -Werror -Wextra -pthread

all: server

server: server.o lista_server.o
	$(CC) $(CFLAGS) server.o lista_server.o -o server

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

lista_server.o: lista_server.c
	$(CC) $(CFLAGS) -c lista_server.c

clean:
	rm -f server *.o
