TARGET 	= sircd

CC		= gcc
LD		= gcc

CFLAGS	= -O2 -Wall
LFLAGS	= 
LIBS	= -pthread

COM     = ../common/

OBJECTS = sircd.o rtlib.o csapp.o util.o int_list.o user.o channel.o socket.o

all:	$(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LFLAGS) $(LIBS) -o $@ $^

sircd.o: sircd.c sircd.h
	$(CC) $(CFLAGS) -c -o $@ $<

rtlib.o: $(COM)rtlib.c $(COM)rtlib.h $(COM)csapp.h
	$(CC) $(CFLAGS) -c -o $@ $<

csapp.o: $(COM)csapp.c $(COM)csapp.h
	$(CC) $(CFLAGS) -c -o $@ $<

util.o: $(COM)util.c $(COM)util.h
	$(CC) $(CFLAGS) -c -o $@ $<	

int_list.o: $(COM)int_list.c $(COM)int_list.h
	$(CC) $(CFLAGS) -c -o $@ $<	

user.o: user.c user.h
	$(CC) $(CFLAGS) -c -o $@ $<	

channel.o: channel.c channel.h
	$(CC) $(CFLAGS) -c -o $@ $<	

socket.o: $(COM)socket.c $(COM)socket.h
	$(CC) $(CFLAGS) -c -o $@ $<	

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(TARGET)

.PHONY: clean
