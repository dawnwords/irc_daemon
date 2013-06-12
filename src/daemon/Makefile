TARGET 	= srouted

CC		= gcc
LD		= gcc

CFLAGS	= -O2 -Wall
LFLAGS	= 
LIBS	= -pthread


COM     = ../common/


OBJECTS	= srouted.o rtlib.o csapp.o util.o socket.o udp.o rtgrading.o lsa_list.o wait_ack_list.o

all:	$(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LFLAGS) $(LIBS) -o $@ $^

srouted.o: srouted.c srouted.h
	$(CC) $(CFLAGS) -c -o $@ $<

rtlib.o: $(COM)rtlib.c $(COM)rtlib.h $(COM)csapp.h
	$(CC) $(CFLAGS) -c -o $@ $<

csapp.o: $(COM)csapp.c $(COM)csapp.h
	$(CC) $(CFLAGS) -c -o $@ $<

util.o: $(COM)util.c $(COM)util.h
	$(CC) $(CFLAGS) -c -o $@ $<	

socket.o: $(COM)socket.c $(COM)socket.h
	$(CC) $(CFLAGS) -c -o $@ $<	

rtgrading.o: rtgrading.c rtgrading.h
	$(CC) $(CFLAGS) -c -o $@ $<	

udp.o: udp.c udp.h
	$(CC) $(CFLAGS) -c -o $@ $<

lsa_list.o: lsa_list.c lsa_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

wait_ack_list.o: wait_ack_list.c wait_ack_list.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(TARGET)

.PHONY: clean
