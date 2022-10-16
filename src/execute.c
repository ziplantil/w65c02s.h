/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-16

            execute.c - instruction execution unit
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "execute.h"
#include "mode.h"
#include "oper.h"

static int handle_stp_wai(struct w65c02s_cpu *cpu) {
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
    cpu->in_rst = 1;
    cpu->in_nmi = cpu->in_irq = 0;
    cpu->cpu_state = CPU_STATE_RUN;
    SET_P(P_A1, 1);
    SET_P(P_B, 1);
    READ(cpu->pc); /* spurious */
    w65c02si_decode(cpu, 0);
}

INLINE void handle_nmi(struct w65c02s_cpu *cpu) {
    /* handle NMI: treat current instruction as BRK */
    cpu->in_nmi = 1;
    cpu->nmi = 0;
    CPU_STATE_CLEAR_NMI(cpu);
    READ(cpu->pc); /* spurious */
    w65c02si_decode(cpu, 0);
}

INLINE void handle_irq(struct w65c02s_cpu *cpu) {
    /* handle IRQ: treat current instruction as BRK */
    cpu->in_irq = 1;
    CPU_STATE_CLEAR_IRQ(cpu);
    READ(cpu->pc); /* spurious */
    w65c02si_decode(cpu, 0);
}

INLINE int handle_interrupt(struct w65c02s_cpu *cpu) {
    if (CPU_STATE_EXTRACT(cpu) == CPU_STATE_RESET)
        handle_reset(cpu);
    else if (CPU_STATE_HAS_NMI(cpu))
        handle_nmi(cpu);
    else if (CPU_STATE_HAS_IRQ(cpu))
        handle_irq(cpu);
    else
        return 0;
    return 1;
}

INLINE void handle_end_of_instruction(struct w65c02s_cpu *cpu) {
    /* increment instruction tally */
    ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
    if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
}

#define STARTING_INSTRUCTION 0
#define CONTINUE_INSTRUCTION 1

#if !W65C02SCE_COARSE

INTERNAL unsigned long w65c02si_execute_c(struct w65c02s_cpu *cpu,
                                          unsigned long maximum_cycles) {
    uint8_t ir;
    if (CPU_STATE_EXTRACT(cpu) != CPU_STATE_RUN) {
        if (!handle_stp_wai(cpu)) return maximum_cycles;
    }

    cpu->left_cycles = maximum_cycles;
    if (cpu->cycl) {
        /* continue running instruction */
        if (w65c02si_run_mode(cpu, CONTINUE_INSTRUCTION)) {
            if (cpu->cycl) cpu->cycl += maximum_cycles;
            return maximum_cycles;
        }
        handle_end_of_instruction(cpu);
    } else if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
        /* we are starting a new instruction and state isn't RUN.
           we don't want STEP to trigget yet - we didn't run an instruction! */
        goto bypass_step;
    } else {
        /* we are starting a new instruction and state is RUN. */
        goto next_instruction;
    }

    while (cpu->left_cycles) {
        unsigned long cyclecount;
        if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
            if (cpu->cpu_state & CPU_STATE_STEP) {
                cpu->cycl = 0;
                return maximum_cycles - cpu->left_cycles;
            }
bypass_step:
            if (!handle_stp_wai(cpu)) return maximum_cycles;
            if (handle_interrupt(cpu)) {
                ir = 0;
                goto decoded;
            }
        }
next_instruction:
        w65c02si_decode(cpu, READ(cpu->pc++));
        w65c02si_prerun_mode(cpu);
decoded:
#if !W65C02SCE_COARSE_CYCLE_COUNTER
        ++cpu->total_cycles;
#endif
        if (!--cpu->left_cycles) break;
        cyclecount = cpu->left_cycles;
        cpu->cycl = 1;
        if (UNLIKELY(w65c02si_run_mode(cpu, STARTING_INSTRUCTION))) {
            if (cpu->cycl) {
                cpu->cycl += cyclecount - cpu->left_cycles;
            } else {
                handle_end_of_instruction(cpu);
            }
            return maximum_cycles;
        }
        handle_end_of_instruction(cpu);
    }
    return maximum_cycles;
}

#else /* W65C02SCE_COARSE */

INTERNAL unsigned long w65c02si_execute_i(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    uint8_t ir;
    if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
        if (!handle_stp_wai(cpu)) return 1;
        if (handle_interrupt(cpu)) {
            ir = 0;
            goto decoded;
        }
    }
    w65c02si_decode(cpu, READ(cpu->pc++));
decoded:
#if !W65C02SCE_COARSE_CYCLE_COUNTER
    ++cpu->total_cycles;
#endif
    w65c02si_prerun_mode(cpu);
    cycles = w65c02si_run_mode(cpu, STARTING_INSTRUCTION);
    handle_end_of_instruction(cpu);
    return cycles;
}

#endif /* W65C02SCE_COARSE */

#endif /* W65C02SCE_SEPARATE */
