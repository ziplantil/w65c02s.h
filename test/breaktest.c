/*******************************************************************************
            w65c02s.h -- cycle-accurate C emulator of the WDC 65C02S
                         as a single-header library
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-29

            breaktest.c - w65c02s_break test program
*******************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W65C02S_IMPL 1
#define W65C02S_LINK 1
#include "w65c02s.h"

#if __STDC_VERSION__ >= 201112L
_Alignas(128)
#endif
uint8_t ram[65536];
struct w65c02s_cpu cpu;
unsigned long cycles;
unsigned long break_cycles;

uint8_t w65c02s_read(uint16_t a) {
    if (!--break_cycles) w65c02s_break(&cpu);
    return ram[a];
}

void w65c02s_write(uint16_t a, uint8_t v) {
    if (!--break_cycles) w65c02s_break(&cpu);
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
        printf("%s <file_in> <vector> <cyclecount> <breakcyclecount>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!loadmemfromfile(argv[1])) {
        return EXIT_FAILURE;
    }

    vector = strtoul(argv[2], NULL, 16);
    cycles = strtoul(argv[3], NULL, 0);
    break_cycles = -1;

    w65c02s_init(&cpu, NULL, NULL, NULL);
    /* RESET cycles */
    w65c02s_run_cycles(&cpu, 7);

    break_cycles = strtoul(argv[4], NULL, 0);

    printf("Running %lu cycles but breaking after %lu cycles\n", cycles, break_cycles);
    cpu.pc = vector;
    cycles = w65c02s_run_cycles(&cpu, cycles);
    printf("Ran %lu cycles\n", cycles);

    return EXIT_SUCCESS;
}
