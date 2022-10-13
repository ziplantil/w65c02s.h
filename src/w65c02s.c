/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            w65c02s.c - main emulator methods
*******************************************************************************/

#include "w65c02s.h"
#include "execute.h"
#include "oper.h"

void w65c02s_init(struct w65c02s_cpu *cpu
#if !W65C02SCE_LINK
                  , uint8_t (*mem_read)(uint16_t)
                  , void (*mem_write)(uint16_t, uint8_t)
#endif
                    ) {
    cpu->total_cycles = cpu->total_instructions = 0;
    cpu->left_cycles = 0;
    cpu->reset = 1;
    cpu->cycl = 0;

#if !W65C02SCE_LINK
    cpu->mem_read = mem_read;
    cpu->mem_write = mem_write;
#endif
#if W65C02SCE_HOOK_BRK
    cpu->hook_brk = NULL;
#endif
#if W65C02SCE_HOOK_STP
    cpu->hook_stp = NULL;
#endif
#if W65C02SCE_HOOK_EOI
    cpu->hook_eoi = NULL;
#endif

    w65c02s_reset(cpu);
}

void w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles) {
    cpu->left_cycles = cycles;
    cpu->step = 0;
    if (cpu->left_cycles) w65c02si_execute(cpu);
}

void w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                              unsigned long instructions,
                              int finish_existing) {
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
}

void w65c02s_nmi(struct w65c02s_cpu *cpu) {
    cpu->nmi = 1;
}

void w65c02s_reset(struct w65c02s_cpu *cpu) {
    cpu->do_reset = 1;
}

void w65c02s_irq(struct w65c02s_cpu *cpu) {
    cpu->irq = 1;
}

void w65c02s_irq_cancel(struct w65c02s_cpu *cpu) {
    cpu->irq = 0;
}

#if W65C02SCE_HOOK_BRK
/* brk_hook: 0 = treat BRK as normal, <>0 = treat it as NOP */
void w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t)) {
    cpu->hook_brk = brk_hook;
}
#endif

#if W65C02SCE_HOOK_STP
/* stp_hook: 0 = treat STP as normal, <>0 = treat it as NOP */
void w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void)) {
    cpu->hook_stp = stp_hook;
}
#endif

#if W65C02SCE_HOOK_EOI
void w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu, void (*instruction_hook)(void)) {
    cpu->hook_eoi = instruction_hook;
}
#endif

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

int w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu) {
    return !!cpu->wai;
}

int w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu) {
    return !!cpu->stp;
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

#if W65C02SCE_SINGLEFILE
#undef W65C02SCE_SINGLEFILE
#include "execute.c"
#include "mode.c"
#include "oper.c"
#include "decode.c"
#endif
