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

bool irq_latch_on_cycle_0[] = {
    1, /* MODE_IMPLIED             */
    1, /* MODE_IMMEDIATE           */
    1, /* MODE_RELATIVE            */
    0, /* MODE_RELATIVE_BIT        */
    0, /* MODE_ZEROPAGE            */
    0, /* MODE_ZEROPAGE_X          */
    0, /* MODE_ZEROPAGE_Y          */
    0, /* MODE_ZEROPAGE_BIT        */
    0, /* MODE_ABSOLUTE            */
    0, /* MODE_ABSOLUTE_X          */
    0, /* MODE_ABSOLUTE_Y          */
    0, /* MODE_ZEROPAGE_INDIRECT   */
    0, /* MODE_ZEROPAGE_INDIRECT_X */
    0, /* MODE_ZEROPAGE_INDIRECT_Y */
    0, /* MODE_ABSOLUTE_INDIRECT   */
    0, /* MODE_ABSOLUTE_INDIRECT_X */
    0, /* MODE_ABSOLUTE_JUMP       */
    0, /* MODE_IMPLIED_1C          */
    1, /* MODE_IMPLIED_X           */
    1, /* MODE_IMPLIED_Y           */
    0, /* MODE_RMW_ZEROPAGE        */
    0, /* MODE_RMW_ZEROPAGE_X      */
    0, /* MODE_SUBROUTINE          */
    0, /* MODE_RETURN_SUB          */
    0, /* MODE_RMW_ABSOLUTE        */
    0, /* MODE_RMW_ABSOLUTE_X      */
    0, /* MODE_NOP_5C              */
    0, /* MODE_INT_WAIT_STOP       */
    0, /* MODE_STACK_PUSH          */
    0, /* MODE_STACK_PULL          */
    0, /* MODE_STACK_BRK           */
    0, /* MODE_STACK_RTI           */
};

INLINE void w65c02si_execute_i(struct w65c02s_cpu *cpu) {
    /* if there is a pending reset, do it now */
    if (cpu->do_reset) {
        cpu->do_reset = 0;
        cpu->reset = 1;
        cpu->stp = cpu->wai = 0;
        cpu->do_nmi = cpu->do_irq = 0;
        cpu->in_nmi = cpu->in_irq = 0;
        cpu->cycl = 0;

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

            /* increment instruction tally */
            ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
            if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
            if (cpu->step) {
                /* we finished one instruction, stop running */
                cpu->cycl = 0;
                return;
            }
        }

        if (cpu->do_nmi) {
            /* handle NMI: treat current instruction as BRK */
            cpu->in_nmi = 1;
            cpu->nmi = cpu->do_nmi = 0;
            READ(cpu->pc);
            w65c02si_decode(cpu, 0);
        } else if (cpu->do_irq) {
            /* handle IRQ: treat current instruction as BRK */
            cpu->in_irq = 1;
            cpu->do_irq = 0;
            READ(cpu->pc);
            w65c02si_decode(cpu, 0);
        } else {
            w65c02si_decode(cpu, READ(cpu->pc++));
            if (irq_latch_on_cycle_0[cpu->oper])
                w65c02si_irq_latch(cpu);
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
