#*******************************************************************************
 # Name        : MakeFile
 # Author      : Jake Paccione
 # Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
#*******************************************************************************

CC = gcc
CFLAGS = -g -Wall

all: server client

server: server.c
	@echo "* Compiling server.c..."
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	@echo "* Compiling client.c..."
	$(CC) $(CFLAGS) client.c -o client

clean:
	@echo "Cleaning up..."
	rm -f server client