-include config.inc
LIBPATH=../src

CC=cc
LD=cc
RM=rm -f

ifdef DEBUG
CFLAGS=-Og -g -Wall -Werror -Wpedantic -ansi -pedantic
LFLAGS=
else
CFLAGS=-O3 -flto
LFLAGS=-flto
endif

CEFLAGS=-DW65C02SCE_LINK=1

ifdef SEPARATE
LIBFILES=$(LIBPATH)/decode.o $(LIBPATH)/execute.o $(LIBPATH)/mode.o			   \
		 $(LIBPATH)/oper.o $(LIBPATH)/w65c02s.o
CFLAGS:=$(CFLAGS) -DW65C02SCE_SEPARATE=1
else
LIBFILES=$(LIBPATH)/w65c02s.o
endif

PROGS=monitor busdump benchmark

.PHONY: all clean

%.o: %.c
	$(CC) $(CFLAGS) $(CEFLAGS) -I$(LIBPATH) -c -o $@ $<

monitor: $(LIBFILES) monitor.o
	$(LD) -o $@ $^ $(LFLAGS)

busdump: $(LIBFILES) busdump.o
	$(LD) -o $@ $^ $(LFLAGS)

benchmark: $(LIBFILES) benchmark.o
	$(LD) -o $@ $^ $(LFLAGS)

all: $(PROGS) 

clean:
	$(RM) ../src/*.o *.o $(PROGS)
