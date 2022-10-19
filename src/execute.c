/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-19

            execute.c - instruction execution unit
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "execute.h"
#include "mode.h"
#include "modejump.h"
#include "oper.h"

INLINE void handle_reset(struct w65c02s_cpu *cpu) {
    /* do RESET */
    cpu->in_rst = 1;
    cpu->in_nmi = cpu->in_irq = 0;
    CPU_STATE_INSERT(cpu, CPU_STATE_RUN);
    CPU_STATE_CLEAR_NMI(cpu);
    CPU_STATE_CLEAR_IRQ(cpu);
    SET_P(P_A1, 1);
    SET_P(P_B, 1);
}

INLINE void handle_nmi(struct w65c02s_cpu *cpu) {
    cpu->in_nmi = 1;
    cpu->int_trig &= ~CPU_STATE_NMI;
    CPU_STATE_CLEAR_NMI(cpu);
}

INLINE void handle_irq(struct w65c02s_cpu *cpu) {
    cpu->in_irq = 1;
    CPU_STATE_CLEAR_IRQ(cpu);
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
    w65c02si_stall(cpu);
    w65c02si_decode(cpu, 0); /* force BRK to handle interrupt */
    return 1;
}

INLINE void handle_end_of_instruction(struct w65c02s_cpu *cpu) {
    /* increment instruction tally */
    ++cpu->total_instructions;
#if W65C02SCE_HOOK_EOI
    if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
}

#if !W65C02SCE_COARSE_CYCLE_COUNTER
#define SPENT_CYCLE             ++cpu->total_cycles
#else
#define SPENT_CYCLE
#endif

#define STARTING_INSTRUCTION 0
#define CONTINUE_INSTRUCTION 1

static int handle_stp_wai_i(struct w65c02s_cpu *cpu) {
    switch (CPU_STATE_EXTRACT(cpu)) {
        case CPU_STATE_WAIT:
            /* if there is an IRQ or NMI, latch it immediately and continue */
            if (cpu->int_trig) {
                w65c02si_irq_latch(cpu);
                CPU_STATE_INSERT(cpu, CPU_STATE_RUN);
                return 0;
            }
        case CPU_STATE_STOP:
            /* spurious read to waste a cycle */
            w65c02si_stall(cpu);
            SPENT_CYCLE;
            return 1;
    }
    return 0;
}

#if !W65C02SCE_COARSE

static int handle_stp_wai_c(struct w65c02s_cpu *cpu) {
    switch (CPU_STATE_EXTRACT(cpu)) {
        case CPU_STATE_WAIT:
            for (;;) {
                if (CPU_STATE_EXTRACT(cpu) == CPU_STATE_RESET) {
                    return 0;
                } else if (cpu->int_trig) {
                    w65c02si_irq_latch(cpu);
                    CPU_STATE_INSERT(cpu, CPU_STATE_RUN);
                    return 0;
                }
                w65c02si_stall(cpu);
                if (CYCLE_CONDITION) return 1;
            }
        case CPU_STATE_STOP:
            for (;;) {
                if (CPU_STATE_EXTRACT(cpu) == CPU_STATE_RESET)
                    return 0;
                w65c02si_stall(cpu);
                if (CYCLE_CONDITION) return 1;
            }
    }
    return 0;
}

INTERNAL unsigned long w65c02si_execute_c(struct w65c02s_cpu *cpu,
                                          unsigned long maximum_cycles) {
    if (UNLIKELY(!maximum_cycles)) return 0;

#if W65C02SCE_COARSE_CYCLE_COUNTER
    cpu->left_cycles = maximum_cycles;
#else
    cpu->target_cycles = cpu->total_cycles + maximum_cycles;
#endif
    if (cpu->cycl) {
        /* continue running instruction */
        if (w65c02si_run_mode(cpu, CONTINUE_INSTRUCTION)) {
            if (cpu->cycl) cpu->cycl += maximum_cycles;
            return maximum_cycles;
        }
        goto end_of_instruction;
    }

    /* new instruction, handle special states now */
    if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN))
        goto check_special_state;

    for (;;) {
        unsigned long cyclecount;

        w65c02si_decode(cpu, READ(cpu->pc++));
        w65c02si_prerun_mode(cpu);
        cpu->cycl = 1; /* interrupts don't need this -- no cycle skip in BRK */

decoded:
#if W65C02SCE_COARSE_CYCLE_COUNTER
        cyclecount = cpu->left_cycles;
#else
        cyclecount = cpu->total_cycles;
#endif
        if (UNLIKELY(CYCLE_CONDITION))
            return maximum_cycles;

        if (UNLIKELY(w65c02si_run_mode(cpu, STARTING_INSTRUCTION))) {
            if (cpu->cycl) {
#if W65C02SCE_COARSE_CYCLE_COUNTER
                cpu->cycl += cyclecount - cpu->left_cycles;
#else
                cpu->cycl += cpu->total_cycles - cyclecount;
#endif
            } else {
                handle_end_of_instruction(cpu);
            }
            return maximum_cycles;
        }

#if W65C02SCE_COARSE_CYCLE_COUNTER
#define CYCLES_NOW (maximum_cycles - cpu->left_cycles)
#else
#define CYCLES_NOW (maximum_cycles - (cpu->target_cycles - cpu->total_cycles))
#endif
end_of_instruction:
        handle_end_of_instruction(cpu);
        if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
check_special_state:
            if (handle_stp_wai_c(cpu)) return CYCLES_NOW;
            if (handle_interrupt(cpu)) goto decoded;
        }
    }
}

INLINE unsigned run_mode_and_return_cycles(struct w65c02s_cpu *cpu,
                                           unsigned cont) {
#if W65C02SCE_COARSE_CYCLE_COUNTER
    cpu->left_cycles = cont ? 0 : -1;
#else
    cpu->target_cycles = cpu->total_cycles - (cont ? 0 : 1);
#endif
    w65c02si_run_mode(cpu, cont);
#if W65C02SCE_COARSE_CYCLE_COUNTER
    return -cpu->left_cycles;
#else
    return cpu->total_cycles - cpu->target_cycles;
#endif
}

INTERNAL unsigned long w65c02si_execute_ic(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    cycles = run_mode_and_return_cycles(cpu, CONTINUE_INSTRUCTION);
    handle_end_of_instruction(cpu);
    return cycles;
}

#endif /* W65C02SCE_COARSE */

INTERNAL unsigned long w65c02si_execute_i(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    if (UNLIKELY(cpu->cpu_state != CPU_STATE_RUN)) {
        if (handle_stp_wai_i(cpu)) return 1;
        if (handle_interrupt(cpu)) goto decoded;
    }

    w65c02si_decode(cpu, READ(cpu->pc++));    
decoded:
    w65c02si_prerun_mode(cpu);
    SPENT_CYCLE;

#if !W65C02SCE_COARSE
    cpu->cycl = 1; /* make sure we start at the first cycle */
    cycles = run_mode_and_return_cycles(cpu, STARTING_INSTRUCTION);
#else
    cycles = w65c02si_run_mode(cpu);
#endif
    handle_end_of_instruction(cpu);
    return cycles;
}

#endif /* W65C02SCE_SEPARATE */
