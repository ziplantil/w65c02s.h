/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            execute.c - instruction execution unit
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "execute.h"
#include "mode.h"
#include "oper.h"

INLINE void w65c02si_execute_i(struct w65c02s_cpu *cpu) {
    uint8_t ir;

    /* if there is a pending reset, do it now */
    if (cpu->do_reset) {
        cpu->do_reset = 0;
        cpu->reset = 1;
        cpu->stp = cpu->wai = 0;
        cpu->do_nmi = cpu->do_irq = 0;
        cpu->in_nmi = cpu->in_irq = 0;
        cpu->had_instruction = 0;

        SET_P(P_A1, 1);
        SET_P(P_B, 1);
        READ(cpu->pc); /* spurious */
        w65c02si_decode(cpu, 0);
        if (!--cpu->left_cycles) return;
    }

    /* if STP, skip everything, */
    if (cpu->stp) {
        /* no more cycles, return immediately */
        cpu->left_cycles = 0;
        return;
    }

    /* if WAI, check for interrupt, or skip everything, */
    if (cpu->wai) {
        if (cpu->irq || cpu->nmi) {
            cpu->do_nmi |= cpu->nmi;
            cpu->do_irq |= cpu->irq;
            cpu->wai = 0;
        } else {
            /* no more cycles, return immediately */
            cpu->left_cycles = 0;
            return;
        }
    }

    while (cpu->left_cycles) {
        if (cpu->cycl) {
            unsigned long cyclecount;
            /* continue running instruction */
instruction:
            cyclecount = cpu->left_cycles;
            if (w65c02si_run_mode(cpu)) {
                cpu->cycl += cyclecount - cpu->left_cycles;
                return;
            }
        }

        if (cpu->had_instruction) {
            /* increment instruction tally */
            ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
            if (cpu->hook_eoi) *(cpu->hook_eoi)();
#endif
            if (cpu->step) {
                /* we finished one instruction, stop running */
                cpu->had_instruction = cpu->cycl = 0;
                return;
            }
        }

        ir = READ(cpu->pc);
        if (cpu->do_nmi) {
            /* handle NMI: treat current instruction as BRK */
            cpu->in_nmi = 1;
            cpu->do_nmi = 0;
            cpu->had_instruction = 0;
            w65c02si_decode(cpu, 0);
        } else if (cpu->do_irq) {
            /* handle IRQ: treat current instruction as BRK */
            cpu->in_irq = 1;
            cpu->do_irq = 0;
            cpu->had_instruction = 0;
            w65c02si_decode(cpu, 0);
        } else {
            ++cpu->pc;
            cpu->had_instruction = 1;
            w65c02si_decode(cpu, ir);
        }

        if (--cpu->left_cycles)
            goto instruction;
    }
}

INTERNAL void w65c02si_execute(struct w65c02s_cpu *cpu) {
    unsigned long cyclecount = cpu->left_cycles;
    w65c02si_execute_i(cpu);
    cpu->total_cycles += cyclecount - cpu->left_cycles;
}

#endif /* W65C02SCE_SEPARATE */
