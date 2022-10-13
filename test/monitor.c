/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            monitor.c - test monitor
*******************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "w65c02s.h"

uint8_t ram[65536];
uint8_t breakpoints[65536];

uint8_t w65c02s_read(uint16_t a) {
    return ram[a];
}

void w65c02s_write(uint16_t a, uint8_t v) {
    ram[a] = v;
}

struct w65c02s_cpu cpu;

static uint16_t dumpmem(uint16_t start) {
    int i, j;
    start = start & ~0xF;
    puts("          +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F");
    putchar('\n');
    for (i = 0; i < 8; ++i) {
        printf("$%04X     ", start);
        for (j = 0; j < 16; ++j) printf("%02X ", ram[start++]);
        putchar('\n');
    }
    return start;
}

static void loadmemfromfile(const char *filename, uint16_t offset) {
    FILE *file = fopen(filename, "rb");
    size_t size = 0;

    if (!file) {
        perror("fopen");
        return;
    }

    size = fread(&ram[offset], 1, 0x10000UL - offset, file);
    fclose(file);

    if (!size)
        puts("0");
    else
        printf("$%04X:$%04X\n", offset, (unsigned)(offset + size - 1));
}

static void dumpmemtofile(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        return;
    }

    if (!fwrite(ram, sizeof(ram), 1, file)) {
        perror("fwrite");
    }
    fclose(file);
}

char linebuf[256];
char linebuf_previous[256];
int run = 1;

uint16_t address_asm = 0;
uint16_t address_break = 0;
uint16_t address_disasm = 0;
uint16_t address_go = 0xFFFC;
uint16_t address_jump = 0;
uint16_t address_load = 0;
uint16_t address_view = 0;
uint16_t address_write = 0;
uint16_t address_set = 0;

unsigned long cycles = 1;

static const char *prune(const char *s) {
    while (*s && isspace(*s)) ++s;
    return s;
}

static int octtodec(char c) {
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    }
    return -1;
}

static int hextodec(char c) {
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': case 'a': return 10;
    case 'B': case 'b': return 11;
    case 'C': case 'c': return 12;
    case 'D': case 'd': return 13;
    case 'E': case 'e': return 14;
    case 'F': case 'f': return 15;
    }
    return -1;
}

static int readbyte(uint8_t *value, const char **sp) {
    const char *s = *sp;
    int dh, dl;

    dh = hextodec(*s++);
    if (dh < 0) return 0;

    dl = hextodec(*s++);
    if (dl < 0) return 0;

    *sp = s;
    *value = (dh << 4) | dl;
    return 1;
}

static int readaddress(uint16_t *address, const char **sp) {
    const char *s = *sp;
    unsigned long result;

    s = prune(s);
    if (*s == '$') ++s;
    result = strtoul(s, (char **)sp, 16);

    if (result || *sp > s) {
        *address = result & 0xFFFFU;
        return 1;
    }
    return 0;
}

static int readcount(unsigned long *address, const char **sp) {
    const char *s = *sp;
    unsigned long result;

    s = prune(s);
    result = strtoul(s, (char **)sp, 0);

    if (result || *sp > s) {
        *address = result;
        return 1;
    }
    return 0;
}

static int readline(const char *prompt) {
    printf("%s", prompt);
    fflush(stdout);
    return !!fgets(linebuf, sizeof(linebuf), stdin);
}

static int readlinefor(uint16_t address) {
    printf("$%04X", address);
    return readline(": ");
}

static void doinput(void) {
    while (readlinefor(address_write)) {
        const char *s = linebuf;
        unsigned i;

        s = prune(s);
        if (!*s) return;

        do {
            i = readbyte(&ram[address_write], &s);
            if (!i) goto line_error;
            ++address_write;
            s = prune(s);
        } while (*s);

        continue;
line_error:
        puts("     ^ Error");
    }
}

typedef int (*asm_mode)(const char *s, uint8_t mask);

uint8_t asm_instr[8];
unsigned int asm_instr_n;

static void asmpush(uint8_t b) {
    if (asm_instr_n < sizeof(asm_instr))
        asm_instr[asm_instr_n++] = b;
}

static void asmpush16(uint16_t a) {
    asmpush(a & 0xFF);
    asmpush((a >> 8) & 0xFF);
}

static int asm_one(const char *s, uint8_t mask) {
    asmpush(mask);
    return 0;
}

static int asm_alu(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0, y = 0;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(mask | 0x09);
        asmpush(a);
        return s - so;
    }
    if (*s == '(') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        s = prune(s);
        if (*s == ',') {
            ++s;
            if (toupper(*s) == 'X') {
                x = 1;
                ++s;
            } else if (toupper(*s) == 'Y') {
                y = 1;
                ++s;
            }
        }
        if (a >= 0x100) return -1;
        if (y) {
            asmpush(mask | 0x11);
        } else if (x) {
            asmpush(mask | 0x01);
        } else {
            asmpush(mask | 0x12);
        }
        asmpush(a);
        s = prune(s);
        if (*s++ != ')') return -1;
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        ++s;
        if (toupper(*s) == 'X') {
            x = 1;
            ++s;
        } else if (toupper(*s) == 'Y') {
            y = 1;
            ++s;
        }
    }
    if (y) {
        asmpush(mask | 0x19);
        asmpush16(a);
    } else {
        if (a >= 0x100) {
            asmpush(mask | 0x0D | (x ? 0x10 : 0x00));
            asmpush16(a);
        } else {
            asmpush(mask | 0x05 | (x ? 0x10 : 0x00));
            asmpush(a);
        }
    }
    return s - so;
}

static int asm_rmw(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0;
    s = prune(s);
    if (!*s || toupper(*s) == 'A') {
        asmpush(mask | 0x0A);
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'X') {
            x = 1;
            ++s;
        }
    }
    if (a >= 0x100) {
        asmpush(mask | 0x0E | (x ? 0x10 : 0x00));
        asmpush16(a);
    } else {
        asmpush(mask | 0x06 | (x ? 0x10 : 0x00));
        asmpush(a);
    }
    return s - so;
}

static int asm_rmw_inc(const char *s, uint8_t mask) {
    const char *so = s;
    s = prune(s);
    if (!*s || toupper(*s) == 'A') {
        asmpush(0x1A);
        return s - so;
    }
    return asm_rmw(s, mask);
}

static int asm_rmw_dec(const char *s, uint8_t mask) {
    const char *so = s;
    s = prune(s);
    if (!*s || toupper(*s) == 'A') {
        asmpush(0x3A);
        return s - so;
    }
    return asm_rmw(s, mask);
}

static int asm_rmb(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    char c = *s++;
    int d = octtodec(c);
    if (d < 0) return -1;
    s = prune(s);
    if (!readaddress(&a, &s)) return -1;
    asmpush(mask | (d << 3));
    asmpush(a);
    return s - so;
}

static int asm_bbr(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a, b;
    char c = *s++;
    int d = octtodec(c);
    if (d < 0) return -1;
    s = prune(s);
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s != ',') return -1;
    ++s;
    s = prune(s);
    if (!readaddress(&b, &s)) return -1;
    asmpush(mask | (d << 3));
    asmpush(a);
    asmpush(b);
    return s - so;
}

static int asm_bra(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (!readaddress(&a, &s)) return -1;
    if (a >= 0x100) return -1;
    asmpush(mask);
    asmpush(a);
    return s - so;
}

static int asm_bit(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(0x89);
        asmpush(a);
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'X') {
            x = 1;
            ++s;
        }
    }
    if (a >= 0x100) {
        asmpush(0x2C | (x ? 0x10 : 0x00));
        asmpush16(a);
    } else {
        asmpush(0x24 | (x ? 0x10 : 0x00));
        asmpush(a);
    }
    return s - so;
}

static int asm_cpx(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(mask | 0x0C);
        asmpush(a);
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (a >= 0x100) {
        asmpush(0xEC);
        asmpush16(a);
    } else {
        asmpush(0xE4);
        asmpush(a);
    }
    return s - so;
}

static int asm_ldx(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int y = 0;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(0xA2);
        asmpush(a);
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'Y') {
            y = 1;
            ++s;
        }
    }
    if (a >= 0x100) {
        asmpush(0xAE | (y ? 0x10 : 0x00));
        asmpush16(a);
    } else {
        asmpush(0xA6 | (y ? 0x10 : 0x00));
        asmpush(a);
    }
    return s - so;
}

static int asm_ldy(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(0xA0);
        asmpush(a);
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'X') {
            x = 1;
            ++s;
        }
    }
    if (a >= 0x100) {
        asmpush(0xAC | (x ? 0x10 : 0x00));
        asmpush16(a);
    } else {
        asmpush(0xA4 | (x ? 0x10 : 0x00));
        asmpush(a);
    }
    return s - so;
}

static int asm_sta(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0, y = 0;
    if (*s == '(') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        s = prune(s);
        if (*s == ',') {
            ++s;
            if (toupper(*s) == 'X') {
                x = 1;
                ++s;
            } else if (toupper(*s) == 'Y') {
                y = 1;
                ++s;
            }
        }
        if (a >= 0x100) return -1;
        if (y) {
            asmpush(0x91);
        } else if (x) {
            asmpush(0x81);
        } else {
            asmpush(0x92);
        }
        asmpush(a);
        s = prune(s);
        if (*s++ != ')') return -1;
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        ++s;
        if (toupper(*s) == 'X') {
            x = 1;
            ++s;
        } else if (toupper(*s) == 'Y') {
            y = 1;
            ++s;
        }
    }
    if (y) {
        asmpush(0x99);
        asmpush16(a);
    } else {
        if (a >= 0x100) {
            asmpush(0x8D | (x ? 0x10 : 0x00));
            asmpush16(a);
        } else {
            asmpush(0x85 | (x ? 0x10 : 0x00));
            asmpush(a);
        }
    }
    return s - so;
}

static int asm_stx(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int y = 0;
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'Y') {
            y = 1;
            ++s;
        }
    }
    if (y) {
        if (a >= 0x100) return -1;
        asmpush(0x96);
        asmpush(a);
    } else {
        if (a >= 0x100) {
            asmpush(0x8E);
            asmpush16(a);
        } else {
            asmpush(0x86);
            asmpush(a);
        }
    }
    return s - so;
}

static int asm_sty(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0;
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'X') {
            x = 1;
            ++s;
        }
    }
    if (x) {
        if (a >= 0x100) return -1;
        asmpush(0x94);
        asmpush(a);
    } else {
        if (a >= 0x100) {
            asmpush(0x8C);
            asmpush16(a);
        } else {
            asmpush(0x84);
            asmpush(a);
        }
    }
    return s - so;
}

static int asm_stz(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    int x = 0;
    if (!readaddress(&a, &s)) return -1;
    s = prune(s);
    if (*s == ',') {
        if (toupper(*++s) == 'X') {
            x = 1;
            ++s;
        }
    }
    if (a >= 0x100) {
        asmpush(0x9C | (x ? 0x02 : 0x00));
        asmpush16(a);
    } else {
        asmpush(0x64 | (x ? 0x10 : 0x00));
        asmpush(a);
    }
    return s - so;
}

static int asm_tsb(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (!readaddress(&a, &s)) return -1;
    if (a >= 0x100) {
        asmpush(mask | 0x0C);
        asmpush16(a);
    } else {
        asmpush(mask | 0x04);
        asmpush(a);
    }
    return s - so;
}

static int asm_jmp(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (*s == '(') {
        int x = 0;
        ++s;
        if (!readaddress(&a, &s)) return -1;
        s = prune(s);
        if (*s == ',') {
            ++s;
            if (toupper(*s) == 'X') {
                x = 1;
                ++s;
            }
        }
        asmpush(x ? 0x7C : 0x6C);
        asmpush16(a);
        if (*s++ != ')') return -1;
        return s - so;
    }
    if (!readaddress(&a, &s)) return -1;
    asmpush(0x4C);
    asmpush16(a);
    return s - so;
}

static int asm_jsr(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (!readaddress(&a, &s)) return -1;
    asmpush(0x20);
    asmpush16(a);
    return s - so;
}

static int asm_brk(const char *s, uint8_t mask) {
    const char *so = s;
    uint16_t a;
    if (*s == '#') {
        ++s;
        if (!readaddress(&a, &s)) return -1;
        if (a >= 0x100) return -1;
        asmpush(0x00);
        asmpush(a);
        return s - so;
    }
    asmpush(0x00);
    asmpush(0x00);
    return s - so;
}

struct asm_opcode {
    char id[4];
    uint8_t mask;
    asm_mode assemble;
};

struct asm_opcode asm_opcode_table[] = {
    { "ADC", 0x60, &asm_alu },
    { "AND", 0x20, &asm_alu },
    { "ASL", 0x00, &asm_rmw },
    { "BBR", 0x0F, &asm_bbr },
    { "BBS", 0x8F, &asm_bbr },
    { "BCC", 0x90, &asm_bra },
    { "BCS", 0xB0, &asm_bra },
    { "BEQ", 0xF0, &asm_bra },
    { "BIT", 0x00, &asm_bit },
    { "BMI", 0x30, &asm_bra },
    { "BNE", 0xD0, &asm_bra },
    { "BPL", 0x10, &asm_bra },
    { "BRA", 0x80, &asm_bra },
    { "BRK", 0x00, &asm_brk },
    { "BVC", 0x50, &asm_bra },
    { "BVS", 0x70, &asm_bra },
    { "CLC", 0x18, &asm_one },
    { "CLD", 0xD8, &asm_one },
    { "CLI", 0x58, &asm_one },
    { "CLV", 0xB8, &asm_one },
    { "CMP", 0xC0, &asm_alu },
    { "CPX", 0xC0, &asm_cpx },
    { "CPY", 0xE0, &asm_cpx },
    { "DEC", 0xC0, &asm_rmw_dec },
    { "DEX", 0xCA, &asm_one },
    { "DEY", 0x88, &asm_one },
    { "EOR", 0x40, &asm_alu },
    { "INC", 0xE0, &asm_rmw_inc },
    { "INX", 0xE8, &asm_one },
    { "INY", 0xC8, &asm_one },
    { "JMP", 0x00, &asm_jmp },
    { "JSR", 0x00, &asm_jsr },
    { "LDA", 0xA0, &asm_alu },
    { "LDX", 0x00, &asm_ldx },
    { "LDY", 0x00, &asm_ldy },
    { "LSR", 0x40, &asm_rmw },
    { "NOP", 0xEA, &asm_one },
    { "ORA", 0x00, &asm_alu },
    { "PHA", 0x48, &asm_one },
    { "PHP", 0x08, &asm_one },
    { "PHX", 0xDA, &asm_one },
    { "PHY", 0x5A, &asm_one },
    { "PLA", 0x68, &asm_one },
    { "PLP", 0x28, &asm_one },
    { "PLX", 0xFA, &asm_one },
    { "PLY", 0x7A, &asm_one },
    { "RMB", 0x07, &asm_rmb },
    { "ROL", 0x20, &asm_rmw },
    { "ROR", 0x60, &asm_rmw },
    { "RTI", 0x40, &asm_one },
    { "RTS", 0x60, &asm_one },
    { "SBC", 0xE0, &asm_alu },
    { "SEC", 0x38, &asm_one },
    { "SED", 0xF8, &asm_one },
    { "SEI", 0x78, &asm_one },
    { "SMB", 0x87, &asm_rmb },
    { "STA", 0x00, &asm_sta },
    { "STP", 0xDB, &asm_one },
    { "STX", 0x00, &asm_stx },
    { "STY", 0x00, &asm_sty },
    { "STZ", 0x00, &asm_stz },
    { "TAX", 0xAA, &asm_one },
    { "TAY", 0xA8, &asm_one },
    { "TRB", 0x10, &asm_tsb },
    { "TSB", 0x00, &asm_tsb },
    { "TSX", 0xBA, &asm_one },
    { "TXA", 0x8A, &asm_one },
    { "TXS", 0x9A, &asm_one },
    { "TYA", 0x98, &asm_one },
    { "WAI", 0xCB, &asm_one },
};

static int find_by_mnemonic(const void *key, const void *el) {
    const char *key_s = key;
    const struct asm_opcode *opc = el;
    return strcmp(key_s, opc->id);
}

static void doassemble(void) {
    while (readlinefor(address_asm)) {
        const char *s = linebuf;
        char instrcode[4];
        unsigned i = 0;
        struct asm_opcode *opcode;

        s = prune(s);
        if (!*s) return;

        asm_instr_n = 0;

        while (i < 3 && *s) instrcode[i++] = toupper(*s++);
        if (i != 3) goto line_error;
        instrcode[i] = 0;
        s = prune(s);

        opcode = bsearch(instrcode, asm_opcode_table,
                         sizeof(asm_opcode_table) / sizeof(asm_opcode_table[0]),
                         sizeof(asm_opcode_table[0]),
                         find_by_mnemonic);
        if (!opcode) goto line_error;

        i = (*opcode->assemble)(s, opcode->mask);
        if (i < 0) goto line_error;

        s = prune(s + i);
        if (*s) goto line_error;

        for (i = 0; i < asm_instr_n; ++i)
            ram[address_asm++] = asm_instr[i];

        continue;
line_error:
        puts("     ^ Error");
    }
}

char disasm_buf[64];
char *disasm_ptr;

static void disasmoutchr(char c) {
    char *disasm_end = disasm_buf + sizeof(disasm_buf);
    if (disasm_ptr < disasm_end) *disasm_ptr++ = c;
}

static void disasmoutstr(const char *s) {
    char *disasm_end = disasm_buf + sizeof(disasm_buf);
    if (disasm_ptr < disasm_end) {
        int n = sprintf(disasm_ptr, "%s", s);
        disasm_ptr += n;
    }
}

static void disasmoutzp(uint16_t pzp) {
    char *disasm_end = disasm_buf + sizeof(disasm_buf);
    if (disasm_ptr < disasm_end) {
        uint8_t zp = ram[pzp];
        int n = sprintf(disasm_ptr, "$%02X", zp);
        disasm_ptr += n;
    }
}

static void disasmoutabs(uint16_t pabs) {
    char *disasm_end = disasm_buf + sizeof(disasm_buf);
    if (disasm_ptr < disasm_end) {
        uint16_t abs = ram[pabs++];
        int n;
        abs |= ram[pabs] << 8;
        n = sprintf(disasm_ptr, "$%04X", abs);
        disasm_ptr += n;
    }
}

static unsigned disasm_0(uint16_t ptr) {
    return 0;
}

static unsigned disasm_imm(uint16_t ptr) {
    disasmoutchr('#');
    disasmoutzp(ptr);
    return 1;
}

static unsigned disasm_zp(uint16_t ptr) {
    disasmoutzp(ptr);
    return 1;
}

static unsigned disasm_zpx(uint16_t ptr) {
    disasmoutzp(ptr);
    disasmoutstr(",X");
    return 1;
}

static unsigned disasm_zpy(uint16_t ptr) {
    disasmoutzp(ptr);
    disasmoutstr(",Y");
    return 1;
}

static unsigned disasm_zpr(uint16_t ptr) {
    disasmoutzp(ptr++);
    disasmoutchr(',');
    disasmoutzp(ptr++);
    return 2;
}

static unsigned disasm_izp(uint16_t ptr) {
    disasmoutchr('(');
    disasmoutzp(ptr);
    disasmoutchr(')');
    return 1;
}

static unsigned disasm_izx(uint16_t ptr) {
    disasmoutchr('(');
    disasmoutzp(ptr);
    disasmoutstr(",X");
    disasmoutchr(')');
    return 1;
}

static unsigned disasm_izy(uint16_t ptr) {
    disasmoutchr('(');
    disasmoutzp(ptr);
    disasmoutstr("),Y");
    return 1;
}

static unsigned disasm_abs(uint16_t ptr) {
    disasmoutabs(ptr);
    return 2;
}

static unsigned disasm_abx(uint16_t ptr) {
    disasmoutabs(ptr);
    disasmoutstr(",X");
    return 2;
}

static unsigned disasm_aby(uint16_t ptr) {
    disasmoutabs(ptr);
    disasmoutstr(",Y");
    return 2;
}

static unsigned disasm_ind(uint16_t ptr) {
    disasmoutchr('(');
    disasmoutabs(ptr);
    disasmoutchr(')');
    return 2;
}

static unsigned disasm_iax(uint16_t ptr) {
    disasmoutchr('(');
    disasmoutabs(ptr);
    disasmoutstr(",X)");
    return 2;
}

typedef unsigned (*disasm_mode)(uint16_t);

struct disasm_opcode {
    const char *mnemonic;
    disasm_mode disassemble;
};

struct disasm_opcode disasm_opcode_table[] = {
    { "BRK", &disasm_imm },
    { "ORA", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "TSB", &disasm_zp },
    { "ORA", &disasm_zp },
    { "ASL", &disasm_zp },
    { "RMB0", &disasm_zp },
    { "PHP", &disasm_0 },
    { "ORA", &disasm_imm },
    { "ASL", &disasm_0 },
    { "NOP", &disasm_0 },
    { "TSB", &disasm_abs },
    { "ORA", &disasm_abs },
    { "ASL", &disasm_abs },
    { "BBR0", &disasm_zpr },
    { "BPL", &disasm_zp },
    { "ORA", &disasm_izy },
    { "ORA", &disasm_izp },
    { "NOP", &disasm_0 },
    { "TRB", &disasm_zp },
    { "ORA", &disasm_zpx },
    { "ASL", &disasm_zpx },
    { "RMB1", &disasm_zp },
    { "CLC", &disasm_0 },
    { "ORA", &disasm_aby },
    { "INC", &disasm_0 },
    { "NOP", &disasm_0 },
    { "TRB", &disasm_abs },
    { "ORA", &disasm_abx },
    { "ASL", &disasm_abx },
    { "BBR1", &disasm_zpr },
    { "JSR", &disasm_abs },
    { "AND", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "BIT", &disasm_zp },
    { "AND", &disasm_zp },
    { "ROL", &disasm_zp },
    { "RMB2", &disasm_zp },
    { "PLP", &disasm_0 },
    { "AND", &disasm_imm },
    { "ROL", &disasm_0 },
    { "NOP", &disasm_0 },
    { "BIT", &disasm_abs },
    { "AND", &disasm_abs },
    { "ROL", &disasm_abs },
    { "BBR2", &disasm_zpr },
    { "BMI", &disasm_zp },
    { "AND", &disasm_izy },
    { "AND", &disasm_izp },
    { "NOP", &disasm_0 },
    { "BIT", &disasm_zpx },
    { "AND", &disasm_zpx },
    { "ROL", &disasm_zpx },
    { "RMB3", &disasm_zp },
    { "SEC", &disasm_0 },
    { "AND", &disasm_aby },
    { "DEC", &disasm_0 },
    { "NOP", &disasm_0 },
    { "BIT", &disasm_abx },
    { "AND", &disasm_abx },
    { "ROL", &disasm_abx },
    { "BBR3", &disasm_zpr },
    { "RTI", &disasm_0 },
    { "EOR", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_zp },
    { "EOR", &disasm_zp },
    { "LSR", &disasm_zp },
    { "RMB4", &disasm_zp },
    { "PHA", &disasm_0 },
    { "EOR", &disasm_imm },
    { "LSR", &disasm_0 },
    { "NOP", &disasm_0 },
    { "JMP", &disasm_abs },
    { "EOR", &disasm_abs },
    { "LSR", &disasm_abs },
    { "BBR4", &disasm_zpr },
    { "BVC", &disasm_zp },
    { "EOR", &disasm_izy },
    { "EOR", &disasm_izp },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_zpx },
    { "EOR", &disasm_zpx },
    { "LSR", &disasm_zpx },
    { "RMB5", &disasm_zp },
    { "CLI", &disasm_0 },
    { "EOR", &disasm_aby },
    { "PHY", &disasm_0 },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_abs },
    { "EOR", &disasm_abx },
    { "LSR", &disasm_abx },
    { "BBR5", &disasm_zpr },
    { "RTS", &disasm_0 },
    { "ADC", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "STZ", &disasm_zp },
    { "ADC", &disasm_zp },
    { "ROR", &disasm_zp },
    { "RMB6", &disasm_zp },
    { "PLA", &disasm_0 },
    { "ADC", &disasm_imm },
    { "ROR", &disasm_0 },
    { "NOP", &disasm_0 },
    { "JMP", &disasm_ind },
    { "ADC", &disasm_abs },
    { "ROR", &disasm_abs },
    { "BBR6", &disasm_zpr },
    { "BVS", &disasm_zp },
    { "ADC", &disasm_izy },
    { "ADC", &disasm_izp },
    { "NOP", &disasm_0 },
    { "STZ", &disasm_zpx },
    { "ADC", &disasm_zpx },
    { "ROR", &disasm_zpx },
    { "RMB7", &disasm_zp },
    { "SEI", &disasm_0 },
    { "ADC", &disasm_aby },
    { "PLY", &disasm_0 },
    { "NOP", &disasm_0 },
    { "JMP", &disasm_iax },
    { "ADC", &disasm_abx },
    { "ROR", &disasm_abx },
    { "BBR7", &disasm_zpr },
    { "BRA", &disasm_zp },
    { "STA", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "STY", &disasm_zp },
    { "STA", &disasm_zp },
    { "STX", &disasm_zp },
    { "SMB0", &disasm_zp },
    { "DEY", &disasm_0 },
    { "BIT", &disasm_imm },
    { "TXA", &disasm_0 },
    { "NOP", &disasm_0 },
    { "STY", &disasm_abs },
    { "STA", &disasm_abs },
    { "STX", &disasm_abs },
    { "BBS0", &disasm_zpr },
    { "BCC", &disasm_zp },
    { "STA", &disasm_izy },
    { "STA", &disasm_izp },
    { "NOP", &disasm_0 },
    { "STY", &disasm_zpx },
    { "STA", &disasm_zpx },
    { "STX", &disasm_zpy },
    { "SMB1", &disasm_zp },
    { "TYA", &disasm_0 },
    { "STA", &disasm_aby },
    { "TXS", &disasm_0 },
    { "NOP", &disasm_0 },
    { "STZ", &disasm_abs },
    { "STA", &disasm_abx },
    { "STZ", &disasm_abx },
    { "BBS1", &disasm_zpr },
    { "LDY", &disasm_imm },
    { "LDA", &disasm_izx },
    { "LDX", &disasm_imm },
    { "NOP", &disasm_0 },
    { "LDY", &disasm_zp },
    { "LDA", &disasm_zp },
    { "LDX", &disasm_zp },
    { "SMB2", &disasm_zp },
    { "TAY", &disasm_0 },
    { "LDA", &disasm_imm },
    { "TAX", &disasm_0 },
    { "NOP", &disasm_0 },
    { "LDY", &disasm_abs },
    { "LDA", &disasm_abs },
    { "LDX", &disasm_abs },
    { "BBS2", &disasm_zpr },
    { "BCS", &disasm_zp },
    { "LDA", &disasm_izy },
    { "LDA", &disasm_izp },
    { "NOP", &disasm_0 },
    { "LDY", &disasm_zpx },
    { "LDA", &disasm_zpx },
    { "LDX", &disasm_zpy },
    { "SMB3", &disasm_zp },
    { "CLV", &disasm_0 },
    { "LDA", &disasm_aby },
    { "TSX", &disasm_0 },
    { "NOP", &disasm_0 },
    { "LDY", &disasm_abx },
    { "LDA", &disasm_abx },
    { "LDX", &disasm_aby },
    { "BBS3", &disasm_zpr },
    { "CPY", &disasm_imm },
    { "CMP", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "CPY", &disasm_zp },
    { "CMP", &disasm_zp },
    { "DEC", &disasm_zp },
    { "SMB4", &disasm_zp },
    { "INY", &disasm_0 },
    { "CMP", &disasm_imm },
    { "DEX", &disasm_0 },
    { "WAI", &disasm_0 },
    { "CPY", &disasm_abs },
    { "CMP", &disasm_abs },
    { "DEC", &disasm_abs },
    { "BBS4", &disasm_zpr },
    { "BNE", &disasm_zp },
    { "CMP", &disasm_izy },
    { "CMP", &disasm_izp },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_zpx },
    { "CMP", &disasm_zpx },
    { "DEC", &disasm_zpx },
    { "SMB5", &disasm_zp },
    { "CLD", &disasm_0 },
    { "CMP", &disasm_aby },
    { "PHX", &disasm_0 },
    { "STP", &disasm_0 },
    { "NOP", &disasm_abs },
    { "CMP", &disasm_abx },
    { "DEC", &disasm_abx },
    { "BBS5", &disasm_zpr },
    { "CPX", &disasm_imm },
    { "SBC", &disasm_izx },
    { "NOP", &disasm_imm },
    { "NOP", &disasm_0 },
    { "CPX", &disasm_zp },
    { "SBC", &disasm_zp },
    { "INC", &disasm_zp },
    { "SMB6", &disasm_zp },
    { "INX", &disasm_0 },
    { "SBC", &disasm_imm },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_0 },
    { "CPX", &disasm_abs },
    { "SBC", &disasm_abs },
    { "INC", &disasm_abs },
    { "BBS6", &disasm_zpr },
    { "BEQ", &disasm_zp },
    { "SBC", &disasm_izy },
    { "SBC", &disasm_izp },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_zpx },
    { "SBC", &disasm_zpx },
    { "INC", &disasm_zpx },
    { "SMB7", &disasm_zp },
    { "SED", &disasm_0 },
    { "SBC", &disasm_aby },
    { "PLX", &disasm_0 },
    { "NOP", &disasm_0 },
    { "NOP", &disasm_abs },
    { "SBC", &disasm_abx },
    { "INC", &disasm_abx },
    { "BBS7", &disasm_zpr },
};

static void disassemble(void) {
    struct disasm_opcode *op = &disasm_opcode_table[ram[address_disasm]];
    unsigned i, len;

    disasm_ptr = disasm_buf;
    len = (*op->disassemble)(address_disasm + 1);
    
    printf("$%04X\t", address_disasm);
    for (i = 0; i <= len; ++i) {
        printf("%02X ", ram[address_disasm++]);
    }
    for (i = len; i <= 3; ++i) {
        printf("   ");
    }
    putchar('\t');
    printf("%s", op->mnemonic);
    putchar('\t');
    *disasm_ptr = 0;
    printf("%s\n", disasm_buf);
}

const char p_flags[8] = "NV--DIZC";

static void dumpregs(void) {
    uint8_t p = w65c02s_reg_get_p(&cpu);
    unsigned i;

    printf("CC=%010lu  CI=%010lu  IC=%d    ",
            w65c02s_get_cycle_count(&cpu),
            w65c02s_get_instruction_count(&cpu),
            cpu.cycl);
    if (cpu.reset)
        printf("RESET    ");
    if (cpu.nmi)
        printf("NMI      ");
    if (cpu.irq)
        printf("IRQ      ");
    if (w65c02s_is_cpu_stopped(&cpu))
        printf("STP");
    else if (w65c02s_is_cpu_waiting(&cpu))
        printf("WAI");
    putchar('\n');
    printf("PC=$%04X  A=$%02X  X=$%02X  Y=$%02X  S=$%02X  P=",
            w65c02s_reg_get_pc(&cpu),
            w65c02s_reg_get_a(&cpu),
            w65c02s_reg_get_x(&cpu),
            w65c02s_reg_get_y(&cpu),
            w65c02s_reg_get_s(&cpu));
    for (i = 0; i < 8; ++i)
        putchar((p & (1 << (7 - i))) ? p_flags[i] : '-');
    putchar('\n');
}

static void dumpstate(void) {
    uint16_t save = address_disasm;
    uint8_t p = w65c02s_reg_get_p(&cpu);
    unsigned i;

    printf("A=$%02X  X=$%02X  Y=$%02X  S=$%02X  P=",
            w65c02s_reg_get_a(&cpu),
            w65c02s_reg_get_x(&cpu),
            w65c02s_reg_get_y(&cpu),
            w65c02s_reg_get_s(&cpu));
    for (i = 0; i < 8; ++i)
        putchar((p & (1 << (7 - i))) ? p_flags[i] : '-');
    printf("   ");

    address_disasm = w65c02s_reg_get_pc(&cpu);
    disassemble();
    address_disasm = save;
}

static void runcpu(void) {
    uint16_t prev_pc = w65c02s_reg_get_pc(&cpu) - 1;
    for (;;) {
        uint16_t pc = w65c02s_reg_get_pc(&cpu);
        if (pc == address_go) {
            puts("Reached specified address");
            break;
        } else if (breakpoints[pc]) {
            printf("Reached breakpoint at $%04X\n", pc);
            break;
        }
        w65c02s_run_instructions(&cpu, 1, 0);
        if (w65c02s_is_cpu_stopped(&cpu)) {
            puts("CPU hit STP");
            break;
        } else if (w65c02s_is_cpu_waiting(&cpu)) {
            puts("CPU hit WAI");
            break;
        } else if (pc == prev_pc) {
            puts("Infinite loop detected");
            break;
        }
        prev_pc = pc;
    }
}

static void runinstrs(unsigned long instrs) {
    uint16_t prev_pc = w65c02s_reg_get_pc(&cpu) - 1;
    unsigned long u;
    for (u = 0; u < instrs; ++u) {
        uint16_t pc = w65c02s_reg_get_pc(&cpu);
        if (breakpoints[pc]) {
            printf("Reached breakpoint at $%04X\n", pc);
            break;
        }
        w65c02s_run_instructions(&cpu, 1, 0);
        dumpstate();
        if (w65c02s_is_cpu_stopped(&cpu)) {
            puts("CPU hit STP");
            break;
        } else if (w65c02s_is_cpu_waiting(&cpu)) {
            puts("CPU hit WAI");
            break;
        } else if (pc == prev_pc) {
            puts("Infinite loop detected");
            break;
        }
        prev_pc = pc;
    }
}

static void processline(void) {
    const char *s = linebuf;
    int c, i;
    char *tmp = strchr(s, '\n');
    if (tmp) *tmp = 0;

    if (!*s) {
        strcpy(linebuf, linebuf_previous);
    } else {
        strcpy(linebuf_previous, linebuf);
    }
    c = *s++;

    switch (toupper(c)) {
        case '?':
            puts("? - help");
            puts("a - assemble (a0000)");
            puts("b - add breakpoint for g (b0000) or delete all (bd)");
            puts("c - run one cycle (c2 = two cycles, etc.)");
            puts("d - disassemble (d0000)");
            puts("g - run until STP, WAI or specified address (g0000)");
            puts("iN,iI - trigger NMI/IRQ");
            puts("j - set PC (j0000)");
            puts("l - load file into RAM (l0000 abc.bin)");
            puts("m - view memory (m0000 starting from $0000)");
            puts("n - run next instruction (N2 = two instructions, etc.)");
            puts("q - quit");
            puts("r - dump registers incl. cycle counters");
            puts("w - overwrite memory by byte (w0000 starting from $0000)");
            puts("xA,xX,xY... - set 8-bit register (xAFF)");
            puts("y - dump memory to mem.img");
            puts("z - reset");
            break;
        case 'A':
            readaddress(&address_asm, &s);
            doassemble();
            break;
        case 'B':
            if (*s == '!') {
                memset(breakpoints, 0, sizeof(breakpoints));
                puts("All breakpoints deleted");
            } else {
                readaddress(&address_break, &s);
                breakpoints[address_break] = !breakpoints[address_break];
                if (breakpoints[address_break]) {
                    printf("Added breakpoint for $%04X\n", address_break);
                } else {
                    printf("Removed breakpoint for $%04X\n", address_break);
                }
            }
            break;
        case 'C':
            cycles = 1;
            readcount(&cycles, &s);
            w65c02s_run_cycles(&cpu, cycles);
            dumpstate();
            break;
        case 'D':
            readaddress(&address_disasm, &s);
            for (i = 0; i < 10; ++i) disassemble();
            break;
        case 'G':
            readaddress(&address_go, &s);
            runcpu();
            break;
        case 'I':
            switch (toupper(*s)) {
                case 'I':
                    w65c02s_irq(&cpu);
                    puts("IRQ");
                    break;
                case 'N':
                    w65c02s_nmi(&cpu);
                    puts("NMI");
                    break;
            }
            break;
        case 'J':
            readaddress(&address_jump, &s);
            w65c02s_reg_set_pc(&cpu, address_jump);
            printf("PC=$%04X\n", w65c02s_reg_get_pc(&cpu));
            if (cpu.reset)
                puts("CPU is resetting and will overwrite this value");
            break;
        case 'L':
            address_load = 0;
            readaddress(&address_load, &s);
            s = prune(s);
            loadmemfromfile(s, address_load);
            break;
        case 'M':
            readaddress(&address_view, &s);
            address_view = dumpmem(address_view);
            break;
        case 'N':
            cycles = 1;
            readcount(&cycles, &s);
            runinstrs(cycles);
            break;
        case 'Q':
            run = 0;
            break;
        case 'R':
            dumpregs();
            break;
        case 'W':
            readaddress(&address_write, &s);
            doinput();
            break;
        case 'X':
            c = *s++;
            readaddress(&address_set, &s);
            switch (toupper(c)) {
                case 'A':
                    w65c02s_reg_set_a(&cpu, address_set);
                    printf("A=$%02X\n", w65c02s_reg_get_a(&cpu));
                    break;
                case 'P':
                    w65c02s_reg_set_p(&cpu, address_set);
                    printf("P=$%02X\n", w65c02s_reg_get_p(&cpu));
                    break;
                case 'S':
                    w65c02s_reg_set_s(&cpu, address_set);
                    printf("S=$%02X\n", w65c02s_reg_get_s(&cpu));
                    break;
                case 'X':
                    w65c02s_reg_set_x(&cpu, address_set);
                    printf("X=$%02X\n", w65c02s_reg_get_x(&cpu));
                    break;
                case 'Y':
                    w65c02s_reg_set_y(&cpu, address_set);
                    printf("Y=$%02X\n", w65c02s_reg_get_y(&cpu));
                    break;
            }
            break;
        case 'Y':
            dumpmemtofile("mem.img");
            puts("Dumped to mem.img");
            break;
        case 'Z':
            w65c02s_reset(&cpu);
            puts("RESET");
            w65c02s_reset_cycle_count(&cpu);
            w65c02s_reset_instruction_count(&cpu);
            break;
    }
}

int main(int argc, char *argv[]) {
    w65c02s_init(&cpu);
    while (run && readline(">>> ")) processline();
    return 0;
}
