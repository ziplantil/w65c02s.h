-include config.inc
LIBPATH=../include

CC=cc
LD=$(CC)
RM=rm -f

ifdef DEBUG
CFLAGS=-Og -g -Wall -Werror -Wpedantic -ansi -pedantic
LFLAGS=
else
CFLAGS=-O3 -fno-tree-tail-merge -flto
LFLAGS=-flto
endif

HEADERS=../include/w65c02s.h

CEFLAGS=

ifdef PROFILE
CFLAGS:=$(CFLAGS) -pg -g2
LFLAGS:=$(LFLAGS) -pg
endif

ifdef COARSE
CEFLAGS:=$(CEFLAGS) -DW65C02S_COARSE=1
endif

ifdef COARSE_CYCLE_COUNTER
CEFLAGS:=$(CEFLAGS) -DW65C02S_COARSE_CYCLE_COUNTER=1
endif

PROGS=monitor busdump benchmark

.PHONY: all clean

all: $(PROGS) 

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(CEFLAGS) -I$(LIBPATH) -c -o $@ $<

monitor: $(LIBFILES) monitor.o
	$(LD) -o $@ $^ $(LFLAGS)

busdump: $(LIBFILES) busdump.o
	$(LD) -o $@ $^ $(LFLAGS)

benchmark: $(LIBFILES) benchmark.o
	$(LD) -o $@ $^ $(LFLAGS)

clean:
	$(RM) ../src/*.o *.o $(PROGS)
