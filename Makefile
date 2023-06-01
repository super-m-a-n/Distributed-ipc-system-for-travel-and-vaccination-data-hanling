SRC = ./src
OBJS = ./obj
STRUCTS = $(SRC)/structs
MON = $(SRC)/monitor
TMON = $(SRC)/travel_monitor
UTILS = $(SRC)/utils

CC = gcc
CFLAGS = -g -Wall -I. -I$(STRUCTS) -I$(UTILS) -I$(MON) -I$(TMON)
target: travelMonitor Monitor

OBJS1 = travelMonitor.o 
OBJS1 += input_check.o
OBJS1 += tm_helper.o tm_signals.o

OBJS2 = Monitor.o
OBJS2 += m_helper.o m_signals.o

COMMON = date.o messages.o bloom.o skip_list.o list.o hash.o m_items.o tm_items.o

bloom.o: $(STRUCTS)/bloom.c
	$(CC) $(CFLAGS) -c $(STRUCTS)/bloom.c
skip_list.o: $(STRUCTS)/skip_list.c
	$(CC) $(CFLAGS) -c $(STRUCTS)/skip_list.c
m_items.o: $(MON)/m_items.c
	$(CC) $(CFLAGS) -c $(MON)/m_items.c
tm_items.o: $(TMON)/tm_items.c
	$(CC) $(CFLAGS) -c $(TMON)/tm_items.c	
list.o: $(STRUCTS)/list.c
	$(CC) $(CFLAGS) -c $(STRUCTS)/list.c
hash.o: $(STRUCTS)/hash.c
	$(CC) $(CFLAGS) -c $(STRUCTS)/hash.c
input_check.o: $(UTILS)/input_check.c
	$(CC) $(CFLAGS) -c $(UTILS)/input_check.c
date.o: $(UTILS)/date.c
	$(CC) $(CFLAGS) -c $(UTILS)/date.c
messages.o: $(UTILS)/messages.c
	$(CC) $(CFLAGS) -c $(UTILS)/messages.c
m_helper.o: $(MON)/m_helper.c
	$(CC) $(CFLAGS) -c $(MON)/m_helper.c
tm_helper.o: $(TMON)/tm_helper.c
	$(CC) $(CFLAGS) -c $(TMON)/tm_helper.c
m_signals.o: $(MON)/m_signals.c
	$(CC) $(CFLAGS) -c $(MON)/m_signals.c
tm_signals.o: $(TMON)/tm_signals.c
	$(CC) $(CFLAGS) -c $(TMON)/tm_signals.c
travelMonitor.o: $(SRC)/travelMonitor.c
	$(CC) $(CFLAGS) -c $(SRC)/travelMonitor.c
Monitor.o: $(SRC)/Monitor.c
	$(CC) $(CFLAGS) -c $(SRC)/Monitor.c

travelMonitor: $(OBJS1) $(COMMON)
	$(CC) $(CFLAGS) $(OBJS1) $(COMMON) -o travelMonitor
	mkdir -p $(OBJS)
	mv -f $(OBJS1) $(OBJS)
	mv -f $(COMMON) $(OBJS)

Monitor: $(OBJS2) $(COMMON)
	$(CC) $(CFLAGS) $(OBJS2) $(COMMON) -o Monitor
	mv -f $(OBJS2) $(OBJS)
	mv -f $(COMMON) $(OBJS)

.PHONY: clean

clean:
	rm -f travelMonitor
	rm -f Monitor
	rm -rf $(OBJS)
