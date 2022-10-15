/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-15

            execute.c - instruction execution unit
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "execute.h"
#include "mode.h"
#include "oper.h"

static int handle_special_cpu_state(struct w65c02s_cpu *cpu) {
    switch (CPU_STATE_EXTRACT(cpu)) {
        case CPU_STATE_WAIT:
            if (cpu->irq || cpu->nmi) {
                w65c02si_irq_latch(cpu);
                CPU_STATE_INSERT(cpu, CPU_STATE_RUN);
                return 1;
            } else {
                return 0;
            }
        case CPU_STATE_STOP:
            return 0;
    }
    return 1;
}

INLINE void handle_reset(struct w65c02s_cpu *cpu) {
    /* do RESET */
    cpu->handled_interrupt = cpu->in_rst = 1;
    cpu->in_nmi = cpu->in_irq = 0;
    cpu->cpu_state = CPU_STATE_RUN;

    SET_P(P_A1, 1);
    SET_P(P_B, 1);
    READ(cpu->pc); /* spurious */
    w65c02si_decode(cpu, 0);
}

INLINE void handle_nmi(struct w65c02s_cpu *cpu) {
    /* handle NMI: treat current instruction as BRK */
    cpu->handled_interrupt = cpu->in_nmi = 1;
    cpu->nmi = 0;
    CPU_STATE_CLEAR_NMI(cpu);

    READ(cpu->pc);
    w65c02si_decode(cpu, 0);
}

INLINE void handle_irq(struct w65c02s_cpu *cpu) {
    /* handle IRQ: treat current instruction as BRK */
    cpu->handled_interrupt = cpu->in_irq = 1;
    CPU_STATE_CLEAR_IRQ(cpu);

    READ(cpu->pc);
    w65c02si_decode(cpu, 0);
}

#if W65C02SCE_ACCURATE

INTERNAL unsigned long w65c02si_execute_c(struct w65c02s_cpu *cpu,
                                          unsigned long maximum_cycles) {
    if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
        if (!handle_special_cpu_state(cpu)) return maximum_cycles;
    }

    cpu->left_cycles = maximum_cycles;
    while (cpu->left_cycles) {
        if (LIKELY(cpu->cycl)) {
            unsigned long cyclecount;
            /* continue running instruction */
instruction:
            cyclecount = cpu->left_cycles;
            if (w65c02si_run_mode(cpu)) {
                if (cpu->cycl) cpu->cycl += cyclecount - cpu->left_cycles;
                return maximum_cycles;
            }

            /* increment instruction tally */
            if (UNLIKELY(cpu->handled_interrupt))
                cpu->handled_interrupt = 0;
            else
                ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
            if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
            if (UNLIKELY(cpu->step)) {
                /* we finished one instruction, stop running */
                cpu->cycl = 0;
                return maximum_cycles - cpu->left_cycles;
            }
        }

        if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
            if (CPU_STATE_EXTRACT(cpu) == CPU_STATE_RESET) {
                handle_reset(cpu);
                goto decoded;
            } else if (!handle_special_cpu_state(cpu)) return 1;

            if (CPU_STATE_HAS_NMI(cpu)) {
                handle_nmi(cpu);
                goto decoded;
            } else if (CPU_STATE_HAS_IRQ(cpu)) {
                handle_irq(cpu);
                goto decoded;
            }
        }
        w65c02si_decode(cpu, READ(cpu->pc++));
        w65c02si_prerun_mode(cpu);

decoded:
        if (--cpu->left_cycles)
            goto instruction;
    }
    return maximum_cycles;
}

#else /* W65C02SCE_ACCURATE */

INTERNAL unsigned long w65c02si_execute_i(struct w65c02s_cpu *cpu) {
    unsigned cycles;

    if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
        if (CPU_STATE_EXTRACT(cpu) == CPU_STATE_RESET) {
            handle_reset(cpu);
            goto decoded;
        } else if (!handle_special_cpu_state(cpu)) return 1;

        if (CPU_STATE_HAS_NMI(cpu)) {
            handle_nmi(cpu);
            goto decoded;
        } else if (CPU_STATE_HAS_IRQ(cpu)) {
            handle_irq(cpu);
            goto decoded;
        }
    }

    w65c02si_decode(cpu, READ(cpu->pc++));
    w65c02si_prerun_mode(cpu);
decoded:
    cycles = w65c02si_run_mode(cpu);

    if (UNLIKELY(cpu->handled_interrupt))
        cpu->handled_interrupt = 0;
    else
        ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
    if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
    return cycles + 1;
}

#endif /* W65C02SCE_ACCURATE */

#endif /* W65C02SCE_SEPARATE */
