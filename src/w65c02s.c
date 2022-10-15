/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-15

            w65c02s.c - main emulator methods
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"

#include <limits.h>
#include <stddef.h>

#include "execute.h"
#include "oper.h"

#if !W65C02SCE_LINK
static uint8_t w65c02s_openbus_read(struct w65c02s_cpu *cpu,
                                    uint16_t addr) {
    return 0xFF;
}

static void w65c02s_openbus_write(struct w65c02s_cpu *cpu,
                                  uint16_t addr, uint8_t value) { }
#endif

void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data) {
    cpu->total_cycles = cpu->total_instructions = 0;
    cpu->cycl = 0;

#if !W65C02SCE_LINK
    cpu->mem_read = mem_read ? mem_read : &w65c02s_openbus_read;
    cpu->mem_write = mem_write ? mem_write : &w65c02s_openbus_write;
#endif
    cpu->hook_brk = NULL;
    cpu->hook_stp = NULL;
    cpu->hook_eoi = NULL;
    cpu->cpu_data = cpu_data;

    w65c02s_reset(cpu);
}

unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu,
                                 unsigned long cycles) {
#if !W65C02SCE_ACCURATE
    /* we may overflow otherwise */
    if (cycles > ULONG_MAX - 8) cycles = ULONG_MAX - 8;
#endif
    cpu->left_cycles = cycles;
    cpu->step = 0;
    if (cpu->left_cycles) w65c02si_execute(cpu);
    return cycles;
}

unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                              unsigned long instructions,
                              int finish_existing) {
    unsigned long total_cycles = cpu->total_cycles;
    if (finish_existing && cpu->cycl) {
        cpu->left_cycles = -1;
        cpu->step = 1;
        w65c02si_execute(cpu);
    }
    while (instructions--) {
        cpu->left_cycles = -1;
        cpu->step = 1;
        w65c02si_execute(cpu);
    }
    return cpu->total_cycles - total_cycles;
}

void w65c02s_nmi(struct w65c02s_cpu *cpu) {
    cpu->nmi = 1;
}

void w65c02s_reset(struct w65c02s_cpu *cpu) {
    cpu->rst = 1;
}

void w65c02s_irq(struct w65c02s_cpu *cpu) {
    cpu->irq = 1;
}

void w65c02s_irq_cancel(struct w65c02s_cpu *cpu) {
    cpu->irq = 0;
}

/* brk_hook: 0 = treat BRK as normal, <>0 = treat it as NOP */
int w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t)) {
#if W65C02SCE_HOOK_BRK
    cpu->hook_brk = brk_hook;
    return 1;
#else
    return 0;
#endif
}

/* stp_hook: 0 = treat STP as normal, <>0 = treat it as NOP */
int w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void)) {
#if W65C02SCE_HOOK_STP
    cpu->hook_stp = stp_hook;
    return 1;
#else
    return 0;
#endif
}

int w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                    void (*instruction_hook)(void)) {
#if W65C02SCE_HOOK_EOI
    cpu->hook_eoi = instruction_hook;
    return 1;
#else
    return 0;
#endif
}

unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_cycles;
}

unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_instructions;
}

void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu) {
    cpu->total_cycles = 0;
}

void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu) {
    cpu->total_instructions = 0;
}

void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu) {
    return cpu->cpu_data;
}

bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu) {
    return cpu->wai;
}

bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu) {
    return cpu->stp;
}

void w65c02s_set_overflow(struct w65c02s_cpu *cpu) {
    cpu->p |= P_V;
}

#if W65C02SCE_HOOK_REGS
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu) {
    return cpu->a;
}

uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu) {
    return cpu->x;
}

uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu) {
    return cpu->y;
}

uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu) {
    return cpu->p | P_A1 | P_B;
}

uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu) {
    return cpu->s;
}

uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu) {
    return cpu->pc;
}

void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->a = v;
}

void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->x = v;
}

void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->y = v;
}

void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->p = v | P_A1 | P_B;
}

void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->s = v;
}

void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v) {
    cpu->pc = v;
}
#endif

#if !W65C02SCE_SEPARATE
#undef W65C02SCE_SEPARATE
#define W65C02SCE_SEPARATE 1
#include "execute.c"
#include "oper.c"
#include "mode.c"
#include "decode.c"
#endif
