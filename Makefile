CC = gcc
CFLAGS  = -g -Wall


all: myport port-master monitor vessel

myport:  myport.o utils.o
	$(CC) $(CFLAGS) -o executables/myport myport.o utils.o

port-master: port-master.o utils.o
	$(CC) $(CFLAGS) -o executables/port-master port-master.o utils.o

monitor: monitor.o utils.o
	$(CC) $(CFLAGS) -o executables/monitor monitor.o utils.o

vessel: vessel.o utils.o
	$(CC) $(CFLAGS) -o executables/vessel vessel.o utils.o

myport.o:  myport/myport.c
	$(CC) $(CFLAGS) -c myport/myport.c

port-master.o: port-master/port-master.c
	$(CC) $(CFLAGS) -c port-master/port-master.c

monitor.o: monitor/monitor.c
	$(CC) $(CFLAGS) -c monitor/monitor.c

vessel.o: vessel/vessel.c
	$(CC) $(CFLAGS) -c vessel/vessel.c

utils.o:  utils/utils.c
	$(CC) $(CFLAGS) -c utils/utils.c


clean: 
	rm -rf executables/* *.o