-include config.inc
LIBPATH=../src

CC=cc
LD=cc
RM=rm -f
CFLAGS=-Og -g -ansi -pedantic -Wall -Werror -Wno-switch-unreachable            \
		-DW65C02SCE_LINK=1 -DW65C02SCE_HOOK_REGS=1

LFLAGS=

ifdef SINGLEFILE
LIBFILES=$(LIBPATH)/w65c02s.o
CFLAGS:=$(CFLAGS) -DW65C02SCE_SINGLEFILE=1
else
LIBFILES=$(LIBPATH)/decode.o $(LIBPATH)/execute.o $(LIBPATH)/mode.o			   \
		 $(LIBPATH)/oper.o $(LIBPATH)/w65c02s.o
endif

PROGS=monitor

.PHONY: all clean

%.o: %.c
	$(CC) $(CFLAGS) -I$(LIBPATH) -c -o $@ $<

monitor: $(LIBFILES) monitor.o
	$(LD) -o $@ $^ $(LFLAGS)

all: $(PROGS) 

clean:
	$(RM) ../src/*.o *.o $(PROGS)
