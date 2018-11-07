CC = gcc
CFLAGS = -Wall -g #-DDEBUG
SRC = src
TESTS = test
FILES = $(SRC)/comm.c $(SRC)/sync.c $(SRC)/log.c $(SRC)/file.c
CMD = $(CC) $(CFLAGS) $(FILES) -lpthread -Iinclude -lm

all: client

client:
	$(CMD) $(SRC)/client.c -o client

clean:
	rm -f client
