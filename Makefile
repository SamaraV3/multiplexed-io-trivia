#Name: Samara Vassell
#Pledge: I pledge my honor that I have abided by the Stevens Honor System - Samara Vassell

CC = gcc
CFLAGS = -Wall -Wextra -g

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client