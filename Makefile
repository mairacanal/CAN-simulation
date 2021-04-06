CC = gcc

CFLAGS = -g -Wall

ACS: ACS.o can.o
		$(CC) $(CFLAGS) -pthread -o ACS ACS.o can.o

ACS.o: ACS.c can.h
		$(CC) $(CFLAGS) -c ACS.c

can.o: can.c can.h
		$(CC) $(CFLAGS) -c can.c
