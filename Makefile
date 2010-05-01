.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .c .o

CFLAGS=-Wall -Wextra -Wshadow -O2

all: ppp_echo_mon
clean:
	rm -f ppp_echo_mon *.o

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

ppp_echo_mon: ppp_echo_mon.c ppp_echo_mon.h
	$(CC) $(CFLAGS) -o $@ $<
