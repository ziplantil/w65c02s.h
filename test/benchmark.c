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
struct w65c02s_cpu cpu;
unsigned long cycles;

uint8_t w65c02s_read(uint16_t a) {
    return ram[a];
}

void w65c02s_write(uint16_t a, uint8_t v) {
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
    if (argc <= 3) {
        printf("%s <file_in> <vector> <cyclecount>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!loadmemfromfile(argv[1])) {
        return EXIT_FAILURE;
    }

    vector = strtoul(argv[2], NULL, 16);
    cycles = strtoul(argv[3], NULL, 0);

    w65c02s_init(&cpu, NULL, NULL, NULL);
    /* RESET cycles */
    w65c02s_run_cycles(&cpu, 7);
    cpu.pc = vector;
    w65c02s_run_cycles(&cpu, cycles);

    return EXIT_SUCCESS;
}
