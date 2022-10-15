/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-15

            busdump.c - bus dump program
*******************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "w65c02s.h"

uint8_t ram[65536];
FILE *dumpfile;
struct w65c02s_cpu cpu;
unsigned long cycles;
unsigned long total_cycles;
unsigned instruction_cycles;

void busdump(unsigned write, uint16_t addr, uint8_t data) {
    unsigned char buf[8];
    buf[0] = write | (cpu.in_rst ? 16 : 0)
                   | (cpu.do_nmi ? 8 : 0)
                   | (cpu.do_irq ? 4 : 0)
                   | (cpu.nmi ? 2 : 0)
                   | (cpu.irq ? 1 : 0);
    buf[1] = instruction_cycles;
    buf[2] = cpu.pc & 0xFF;
    buf[3] = (cpu.pc >> 8) & 0xFF;
    buf[4] = addr & 0xFF;
    buf[5] = (addr >> 8) & 0xFF;
    buf[6] = 0;
    buf[7] = data;
    fwrite(buf, 1, sizeof(buf), dumpfile);
}

uint8_t w65c02s_read(uint16_t a) {
    busdump(0x00, a, ram[a]);
    ++instruction_cycles;
    ++total_cycles;
    return ram[a];
}

void w65c02s_write(uint16_t a, uint8_t v) {
    busdump(0x80, a, v);
    ++instruction_cycles;
    ++total_cycles;
    ram[a] = v;
}

uint16_t vector = 0;

static size_t loadmemfromfile(const char *filename) {
    FILE *file = fopen(filename, "rb");
    size_t size = 0;

    if (!file) {
        perror("fopen");
        return 0;
    }

    size = fread(ram, 1, 0x10000UL, file);
    fclose(file);
    return size;
}

int main(int argc, char *argv[]) {
    if (argc <= 4) {
        printf("%s <file_in> <vector> <cyclecount> <file_out>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!loadmemfromfile(argv[1])) {
        return EXIT_FAILURE;
    }

    dumpfile = fopen(argv[4], "wb");
    if (!dumpfile) {
        perror("file_out fopen");
        return EXIT_FAILURE;
    }

    vector = strtoul(argv[2], NULL, 16);
    cycles = strtoul(argv[3], NULL, 0);

    w65c02s_init(&cpu, NULL, NULL, NULL);
    /* RESET cycles */
    w65c02s_run_cycles(&cpu, 7);
    cpu.pc = vector;
    total_cycles = 0;
    while (total_cycles < cycles) {
        instruction_cycles = 0;
        w65c02s_run_instructions(&cpu, 1, 0);
    }

    fclose(dumpfile);
    return EXIT_SUCCESS;
}
