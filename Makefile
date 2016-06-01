.PHONY: all clean install
.SUFFIXES:
.SUFFIXES: .c .o

CFLAGS=-Wall -Wextra -Wshadow -O2
INSTALL=install

all: ppp_echo_mon
clean:
	rm -f ppp_echo_mon *.o

prefix=/usr
exec_prefix=$(prefix)
libdir=$(exec_prefix)/lib

install: all
	$(INSTALL) -m 755 -D ppp_echo_mon $(DESTDIR)$(libdir)/ppp_echo_mon

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

ppp_echo_mon: ppp_echo_mon.c ppp_echo_mon.h
	$(CC) $(CFLAGS) -o $@ $<
