CC = gcc
CFLAGS = -Wall -g

CLIENT_OBJ = main.o communication.o

all: client
	 @echo "All files compiled!"

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(CLIENT_OBJ) -o $@ -pthread -Iinclude

clean: 
	rm -f *.o client