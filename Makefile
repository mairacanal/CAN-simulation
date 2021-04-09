CC = gcc

CFLAGS = -g -Wall

all: ACS ADS CDH CTH EPS

ACS: ACS.o can.o
		$(CC) $(CFLAGS) -pthread -o ACS ACS.o can.o

ACS.o: ACS.c can.h CAN_ID_map.h 
		$(CC) $(CFLAGS) -c ACS.c

ADS: ADS.o can.o
		$(CC) $(CFLAGS) -pthread -o ADS ADS.o can.o

ADS.o: ADS.c can.h CAN_ID_map.h
		$(CC) $(CFLAGS) -c ADS.c

CDH: CDH.o can.o
		$(CC) $(CFLAGS) -pthread -o CDH CDH.o can.o

CDH.o: CDH.c can.h CAN_ID_map.h
		$(CC) $(CFLAGS) -c CDH.c

CTH: CTH.o can.o
		$(CC) $(CFLAGS) -pthread -o CTH CTH.o can.o

CTH.o: CTH.c can.h CAN_ID_map.h
		$(CC) $(CFLAGS) -c CTH.c

EPS: EPS.o can.o
		$(CC) $(CFLAGS) -pthread -o EPS EPS.o can.o

EPS.o: EPS.c can.h CAN_ID_map.h
		$(CC) $(CFLAGS) -c EPS.c

can.o: can.c can.h
		$(CC) $(CFLAGS) -c can.c

clean:
	rm -f *.o

.PHONY: all