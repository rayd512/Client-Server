CC=g++
CFLAGS=-Wall -O -std=c++11
# OBJECTS=client.o

all:client server

client: client.o tands.o
	$(CC) -o client client.o tands.o

client.o: client.cpp
	$(CC) $(CFLAGS) -c client.cpp -o client.o 

server: server.o tands.o
	$(CC) -o server server.o tands.o

server.o: server.cpp
	$(CC) $(CFLAGS) -c server.cpp -o server.o 

tands.o: tands.c
	$(CC) $(CFLAGS) -c tands.c -o tands.o 

logs:
	$(RM) *.[0-9]*
clean:
	$(RM) client server *.[0-9]* *.o
