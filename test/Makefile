-include config.inc
LIBPATH=../src

CC=cc
LD=cc
RM=rm -f
CFLAGS=-O2 -DW65C02SCE_LINK=1 -DW65C02SCE_HOOK_REGS=1 -DW65C02SCE_HOOK_EOI=1
LFLAGS=

ifdef SEPARATE
LIBFILES=$(LIBPATH)/decode.o $(LIBPATH)/execute.o $(LIBPATH)/mode.o			   \
		 $(LIBPATH)/oper.o $(LIBPATH)/w65c02s.o
CFLAGS:=$(CFLAGS) -DW65C02SCE_SEPARATE=1
else
LIBFILES=$(LIBPATH)/w65c02s.o
endif

PROGS=busdump monitor

.PHONY: all clean

%.o: %.c
	$(CC) $(CFLAGS) -I$(LIBPATH) -c -o $@ $<

busdump: $(LIBFILES) busdump.o
	$(LD) -o $@ $^ $(LFLAGS)

monitor: $(LIBFILES) monitor.o
	$(LD) -o $@ $^ $(LFLAGS)

all: $(PROGS) 

clean:
	$(RM) ../src/*.o *.o $(PROGS)
