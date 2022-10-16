/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-16

            mode.c - addressing modes
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "mode.h"
#include "modejump.h"
#include "oper.h"

INLINE void stack_push(struct w65c02s_cpu *cpu, uint8_t v) {
    uint16_t s = STACK_ADDR(cpu->s--);
    WRITE(s, v);
}

INLINE uint8_t stack_pull(struct w65c02s_cpu *cpu) {
    uint16_t s = STACK_ADDR(++cpu->s);
    return READ(s);
}

INLINE uint8_t update_flags(struct w65c02s_cpu *cpu, uint8_t q) {
    /* N: bit 7 of result. Z: whether result is 0 */
    SET_P(P_N, q & 0x80);
    SET_P(P_Z, q == 0);
    return q;
}

INLINE void irq_latch_slow(struct w65c02s_cpu *cpu) {
    /* cpu->nmi = 0 | CPU_STATE_NMI */
    cpu->cpu_state |= cpu->nmi;
    /* cpu->irq = 0 | CPU_STATE_IRQ */
    if (!GET_P(P_I)) cpu->cpu_state = (cpu->cpu_state & ~CPU_STATE_IRQ) | cpu->irq;
}

INLINE void irq_latch(struct w65c02s_cpu *cpu) {
    /* cpu->nmi = 0 | CPU_STATE_NMI */
    cpu->cpu_state |= cpu->nmi;
    /* cpu->irq = 0 | CPU_STATE_IRQ */
    /*           irq           0        IRQ  */
    /*  P.I=0 -> ~P &  IRQ    =0       =1    */
    /*  P.I=1 -> ~P & ~IRQ    =0       =0    */
    cpu->cpu_state |= cpu->irq & ~cpu->p;
}

STATIC void irq_reset(struct w65c02s_cpu *cpu) {
    if (cpu->in_rst) {
        cpu->in_rst = 0;
    } else if (cpu->in_nmi) {
        cpu->in_nmi = 0;
    } else if (cpu->in_irq) {
        cpu->in_irq = 0;
    }
}

/* 0 = no store penalty, 1 = store penalty */
INLINE int oper_is_store(unsigned oper) {
    switch (oper) {
        case OPER_STA:
        case OPER_STX:
        case OPER_STY:
        case OPER_STZ:
            return 1;
    }
    return 0;
}

INLINE unsigned decimal_penalty(struct w65c02s_cpu *cpu, unsigned oper) {
    if (!GET_P(P_D)) return 0;
    switch (oper) {
        case OPER_ADC:
        case OPER_SBC:
            return 1;
    }
    return 0;
}

INLINE unsigned fast_rmw_absx(unsigned oper) {
    switch (oper) {
        case OPER_INC:
        case OPER_DEC:
            return 0;
    }
    return 1;
}

STATIC void compute_branch(struct w65c02s_cpu *cpu) {
    /* offset in tr[0] */
    /* old PC in tr[0], tr[1] */
    /* new PC in tr[2], tr[3] */
    uint8_t offset = cpu->tr[0];
    uint16_t pc_old = cpu->pc;
    uint16_t pc_new = pc_old + offset - (offset & 0x80 ? 0x100 : 0);
    cpu->tr[0] = GET_LO(pc_old);
    cpu->tr[1] = GET_HI(pc_old);
    cpu->tr[2] = GET_LO(pc_new);
    cpu->tr[3] = GET_HI(pc_new);
}

/* false = no penalty, true = decimal mode penalty */
STATIC bool oper_imm(struct w65c02s_cpu *cpu, uint8_t v) {
    unsigned oper = cpu->oper;
    switch (oper) {
        case OPER_NOP:
            break;

        case OPER_AND:
        case OPER_EOR:
        case OPER_ORA:
        case OPER_ADC:
        case OPER_SBC:
            cpu->a = w65c02si_oper_alu(cpu, oper, cpu->a, v);
            return decimal_penalty(cpu, oper);

        case OPER_CMP: w65c02si_oper_cmp(cpu, cpu->a, v);     break;
        case OPER_CPX: w65c02si_oper_cmp(cpu, cpu->x, v);     break;
        case OPER_CPY: w65c02si_oper_cmp(cpu, cpu->y, v);     break;
        case OPER_BIT: w65c02si_oper_bit_imm(cpu, cpu->a, v); break;
        
        case OPER_LDA: cpu->a = update_flags(cpu, v);         break;
        case OPER_LDX: cpu->x = update_flags(cpu, v);         break;
        case OPER_LDY: cpu->y = update_flags(cpu, v);         break;
        default: unreachable();
    }
    return 0;
}

/* false = no penalty, true = decimal mode penalty */
STATIC bool oper_addr(struct w65c02s_cpu *cpu, uint16_t a) {
    unsigned oper = cpu->oper;
    switch (oper) {
        case OPER_NOP:
            READ(a);
            break;

        case OPER_AND:
        case OPER_EOR:
        case OPER_ORA:
        case OPER_ADC:
        case OPER_SBC:
            cpu->a = w65c02si_oper_alu(cpu, oper, cpu->a, READ(a));
            return decimal_penalty(cpu, oper);

        case OPER_CMP: w65c02si_oper_cmp(cpu, cpu->a, READ(a)); break;
        case OPER_CPX: w65c02si_oper_cmp(cpu, cpu->x, READ(a)); break;
        case OPER_CPY: w65c02si_oper_cmp(cpu, cpu->y, READ(a)); break;
        case OPER_BIT: w65c02si_oper_bit(cpu, cpu->a, READ(a)); break;

        case OPER_LDA: cpu->a = update_flags(cpu, READ(a));     break;
        case OPER_LDX: cpu->x = update_flags(cpu, READ(a));     break;
        case OPER_LDY: cpu->y = update_flags(cpu, READ(a));     break;

        case OPER_STA: WRITE(a, cpu->a);                        break;
        case OPER_STX: WRITE(a, cpu->x);                        break;
        case OPER_STY: WRITE(a, cpu->y);                        break;
        case OPER_STZ: WRITE(a, 0);                             break;
        default: unreachable();
    }
    return 0;
}

/* current CPU, current opcode, whether we are continuing an instruction */
#define MODE_PARAMS struct w65c02s_cpu *cpu,                                   \
                    unsigned cont

#define ADC_D_SPURIOUS(ea)                                                     \
    if (!cpu->take) SKIP_REST; /* no penalty cycle => skip last read */        \
    cpu->p = cpu->p_adj;                                                       \
    READ(ea);

static unsigned mode_implied(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case OPER_NOP:
                    break;

                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->a = w65c02si_oper_rmw(cpu, oper, cpu->a);
                    break;

#define TRANSFER(src, dst) cpu->dst = update_flags(cpu, cpu->src);
                case OPER_CLV: SET_P(P_V, 0);   break;
                case OPER_CLC: SET_P(P_C, 0);   break;
                case OPER_SEC: SET_P(P_C, 1);   break;
                case OPER_CLI: SET_P(P_I, 0);   break;
                case OPER_SEI: SET_P(P_I, 1);   break;
                case OPER_CLD: SET_P(P_D, 0);   break;
                case OPER_SED: SET_P(P_D, 1);   break;
                case OPER_TAX: TRANSFER(a, x);  break;
                case OPER_TXA: TRANSFER(x, a);  break;
                case OPER_TAY: TRANSFER(a, y);  break;
                case OPER_TYA: TRANSFER(y, a);  break;
                case OPER_TSX: TRANSFER(s, x);  break;
                case OPER_TXS: cpu->s = cpu->x; break;
                default: unreachable();
            }
        }
            READ(cpu->pc);
    END_INSTRUCTION
}

static unsigned mode_implied_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->x = w65c02si_oper_rmw(cpu, cpu->oper, cpu->x);
            READ(cpu->pc);
    END_INSTRUCTION
}

static unsigned mode_implied_y(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->y = w65c02si_oper_rmw(cpu, cpu->oper, cpu->y);
            READ(cpu->pc);
    END_INSTRUCTION
}

static unsigned mode_immediate(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->take = oper_imm(cpu, READ(cpu->pc++)); /* penalty cycle? */
        CYCLE(2)
            ADC_D_SPURIOUS(cpu->pc);
    END_INSTRUCTION
}

static unsigned mode_zeropage(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(2)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(3)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION
}

static unsigned mode_zeropage_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[0] += cpu->x;
            READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION
}

static unsigned mode_zeropage_y(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[0] += cpu->y;
            READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION
}

static unsigned mode_absolute(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_absolute_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->x); /* page wrap cycle? */
            cpu->tr[0] += cpu->x;
            cpu->tr[1] = READ(cpu->pc++);
            cpu->take = cpu->tr[4] || oper_is_store(cpu->oper);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(4);
            READ(!cpu->tr[4] ? GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(4)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_absolute_y(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->y); /* page wrap cycle? */
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = READ(cpu->pc++);
            cpu->take = cpu->tr[4] || oper_is_store(cpu->oper);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(4);
            READ(!cpu->tr[4] ? GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(4)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_zeropage_indirect(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[2] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[0] = READ(cpu->tr[2]++);
        CYCLE(3)
            cpu->tr[1] = READ(cpu->tr[2]);
            irq_latch(cpu);
        CYCLE(4)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_zeropage_indirect_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[2] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[2] += cpu->x;
            READ(cpu->pc++);
        CYCLE(3)
            cpu->tr[0] = READ(cpu->tr[2]++);
        CYCLE(4)
            cpu->tr[1] = READ(cpu->tr[2]++);
            irq_latch(cpu);
        CYCLE(5)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(6)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_zeropage_indirect_y(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[2] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[0] = READ(cpu->tr[2]++);
        CYCLE(3)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->y); /* page wrap cycle? */
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = READ(cpu->tr[2]);
            cpu->take = cpu->tr[4] || oper_is_store(cpu->oper);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(4)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(5);
            READ(cpu->tr[2]);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(5)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch_slow(cpu);
        CYCLE(6)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_jump_absolute(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc);
            cpu->pc = GET_T16(0);
    END_INSTRUCTION
}

static unsigned mode_jump_indirect(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc);
        CYCLE(3)
            READ(cpu->pc);
        CYCLE(4)
            cpu->tr[2] = READ(GET_T16(0));
            /* increment with overflow */
            if (!++cpu->tr[0]) ++cpu->tr[1];
            irq_latch(cpu);
        CYCLE(5)
            cpu->tr[3] = READ(GET_T16(0));
            cpu->pc = GET_T16(2);
    END_INSTRUCTION
}

static unsigned mode_jump_indirect_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc);
        CYCLE(3)
            /* add X to tr0,1 */
            cpu->tr[1] += OVERFLOW8(cpu->tr[0], cpu->x);
            cpu->tr[0] += cpu->x;
            READ(cpu->pc);
        CYCLE(4)
            cpu->tr[2] = READ(GET_T16(0));
            /* increment with overflow */
            if (!++cpu->tr[0]) ++cpu->tr[1];
            irq_latch(cpu);
        CYCLE(5)
            cpu->tr[3] = READ(GET_T16(0));
            cpu->pc = GET_T16(2);
    END_INSTRUCTION
}

static unsigned mode_zeropage_bit(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->tr[0]);
        CYCLE(3)
            cpu->tr[1] = w65c02si_oper_bitset(cpu->oper,
                                              cpu->tr[1]);
            READ(cpu->tr[0]);
            irq_latch(cpu);
        CYCLE(4)
            WRITE(cpu->tr[0], cpu->tr[1]);
    END_INSTRUCTION
}

static unsigned mode_relative(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            compute_branch(cpu);
            irq_latch(cpu);
        CYCLE(2)
            /* skip the rest of the instruction if branch is not taken */
            if (!w65c02si_oper_branch(cpu->oper, cpu->p))
                SKIP_REST;
            READ(GET_T16(0));
            cpu->pc = GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) irq_latch(cpu);
        CYCLE(3)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) SKIP_REST;
            READ(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_relative_bit(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[3] = READ(cpu->pc++);
        CYCLE(2)
            READ(cpu->tr[3]);
        CYCLE(3)
            cpu->tr[4] = READ(cpu->tr[3]);
        CYCLE(4)
            cpu->take = w65c02si_oper_bitbranch(cpu->oper,
                                                cpu->tr[4]);
            cpu->tr[0] = READ(cpu->pc++);
            compute_branch(cpu);
        CYCLE(5)
            /* skip the rest of the instruction if branch is not taken */
            if (!cpu->take) SKIP_REST;
            READ(GET_T16(0));
            cpu->pc = GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) irq_latch(cpu);
        CYCLE(6)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) SKIP_REST;
            READ(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_rmw_zeropage(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->tr[0]);
        CYCLE(3)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->tr[1] = w65c02si_oper_rmw(cpu, oper, cpu->tr[1]);
                    break;
                case OPER_TSB:
                    cpu->tr[1] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[1], 1);
                    break;
                case OPER_TRB:
                    cpu->tr[1] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[1], 0);
                    break;
                default: unreachable();
            }
        }
            READ(cpu->tr[0]);
            irq_latch(cpu);
        CYCLE(4)
            WRITE(cpu->tr[0], cpu->tr[1]);
    END_INSTRUCTION
}

static unsigned mode_rmw_zeropage_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[0] += cpu->x;
            READ(cpu->pc++);
        CYCLE(3)
            cpu->tr[1] = READ(cpu->tr[0]);
        CYCLE(4)
            READ(cpu->tr[0]);
            cpu->tr[1] = w65c02si_oper_rmw(cpu, cpu->oper, cpu->tr[1]);
            irq_latch(cpu);
        CYCLE(5)
            WRITE(cpu->tr[0], cpu->tr[1]);
    END_INSTRUCTION
}

static unsigned mode_rmw_absolute(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
        CYCLE(3)
            cpu->tr[2] = READ(GET_T16(0));
        CYCLE(4)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->tr[2] = w65c02si_oper_rmw(cpu, oper, cpu->tr[2]);
                    break;
                case OPER_TSB:
                    cpu->tr[2] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[2], 1);
                    break;
                case OPER_TRB:
                    cpu->tr[2] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[2], 0);
                    break;
                default: unreachable();
            }
        }
            READ(GET_T16(0));
            irq_latch(cpu);
        CYCLE(5)
            WRITE(GET_T16(0), cpu->tr[2]);
    END_INSTRUCTION
}

static unsigned mode_rmw_absolute_x(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
        CYCLE(3)
        {
            unsigned overflow;
            overflow = OVERFLOW8(cpu->tr[0], cpu->x);
            cpu->tr[0] += cpu->x;
            if (!overflow && fast_rmw_absx(cpu->oper)) SKIP_TO_NEXT(4);
            cpu->tr[1] += overflow;
        }
            READ(GET_T16(0));
        CYCLE(4)
            cpu->tr[2] = READ(GET_T16(0));
        CYCLE(5)
            READ(GET_T16(0));
            cpu->tr[2] = w65c02si_oper_rmw(cpu, cpu->oper,
                                           cpu->tr[2]);
            irq_latch(cpu);
        CYCLE(6)
            WRITE(GET_T16(0), cpu->tr[2]);
    END_INSTRUCTION
}

static unsigned mode_stack_push(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            READ(cpu->pc);
            irq_latch(cpu);
        CYCLE(2)
        {
            uint8_t tmp;
            switch (cpu->oper) {
                case OPER_PHP: tmp = cpu->p | P_A1 | P_B; break;
                case OPER_PHA: tmp = cpu->a; break;
                case OPER_PHX: tmp = cpu->x; break;
                case OPER_PHY: tmp = cpu->y; break;
                default: unreachable();
            }
            stack_push(cpu, tmp);
        }
    END_INSTRUCTION
}

static unsigned mode_stack_pull(MODE_PARAMS) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            READ(cpu->pc);
        CYCLE(2)
            READ(STACK_ADDR(cpu->s));
            irq_latch(cpu);
        CYCLE(3)
        {
            uint8_t tmp= stack_pull(cpu);
            switch (cpu->oper) {
                case OPER_PHP: cpu->p = tmp | P_A1 | P_B; break;
                case OPER_PHA: cpu->a = update_flags(cpu, tmp); break;
                case OPER_PHX: cpu->x = update_flags(cpu, tmp); break;
                case OPER_PHY: cpu->y = update_flags(cpu, tmp); break;
                default: unreachable();
            }
        }
    END_INSTRUCTION
}

static unsigned mode_subroutine(MODE_PARAMS) {
    /* op = JSR */
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            READ(STACK_ADDR(cpu->s));
        CYCLE(3)
            stack_push(cpu, GET_HI(cpu->pc));
        CYCLE(4)
            stack_push(cpu, GET_LO(cpu->pc));
            irq_latch(cpu);
        CYCLE(5)
            cpu->tr[1] = READ(cpu->pc);
            cpu->pc = GET_T16(0);
    END_INSTRUCTION
}

static unsigned mode_return_sub(MODE_PARAMS) {
    /* op = RTS */
    BEGIN_INSTRUCTION
        CYCLE(1)
            READ(cpu->pc);
        CYCLE(2)
            READ(STACK_ADDR(cpu->s));
        CYCLE(3)
            SET_LO(cpu->pc, stack_pull(cpu));
        CYCLE(4)
            SET_HI(cpu->pc, stack_pull(cpu));
            irq_latch(cpu);
        CYCLE(5)
            READ(cpu->pc++);
    END_INSTRUCTION
}

static unsigned mode_stack_rti(MODE_PARAMS) {
    /* op = RTI */
    BEGIN_INSTRUCTION
        CYCLE(1)
            READ(cpu->pc);
        CYCLE(2)
            READ(STACK_ADDR(cpu->s));
        CYCLE(3)
            cpu->p = stack_pull(cpu) | P_A1 | P_B;
        CYCLE(4)
            SET_LO(cpu->pc, stack_pull(cpu));
            irq_latch(cpu);
        CYCLE(5)
            SET_HI(cpu->pc, stack_pull(cpu));
    END_INSTRUCTION
}

static int mode_stack_brk(MODE_PARAMS) {
    /* op = BRK (possibly via NMI, IRQ or RESET) */
    BEGIN_INSTRUCTION
        CYCLE(1)
        {
            uint8_t tmp = READ(cpu->pc);
            /* is this instruction a true BRK or a hardware interrupt? */
            cpu->take = !(cpu->in_nmi || cpu->in_irq || cpu->in_rst);
            if (cpu->take) {
                ++cpu->pc;
#if W65C02SCE_HOOK_BRK
                if (cpu->hook_brk && (*cpu->hook_brk)(tmp)) SKIP_REST;
#else
                (void)tmp;
#endif
            }
        }
        CYCLE(2)
            if (cpu->in_rst) READ(STACK_ADDR(cpu->s--));
            else             stack_push(cpu, GET_HI(cpu->pc));
        CYCLE(3)
            if (cpu->in_rst) READ(STACK_ADDR(cpu->s--));
            else             stack_push(cpu, GET_LO(cpu->pc));
        CYCLE(4)
            if (cpu->in_rst) READ(STACK_ADDR(cpu->s--));
            else { /* B flag: 0 for NMI/IRQ, 1 for BRK */
                uint8_t p = cpu->p | P_A1;
                if (cpu->take)
                    p |= P_B;
                else
                    p &= ~P_B;
                stack_push(cpu, p);
            }
            SET_P(P_I, 1);
            SET_P(P_D, 0);
        CYCLE(5)
            /* NMI replaces IRQ if one is triggered before this cycle */
            if (cpu->nmi && cpu->in_irq) {
                CPU_STATE_CLEAR_IRQ(cpu);
                cpu->nmi = 0;
                cpu->in_irq = 0;
                cpu->in_nmi = 1;
            }
            { /* select vector */
                uint16_t vec;
                if (cpu->in_rst)        vec = VEC_RST;
                else if (cpu->in_nmi)   vec = VEC_NMI;
                else                    vec = VEC_IRQ;
                cpu->tr[0] = GET_LO(vec);
                cpu->tr[1] = GET_HI(vec);
            }
            SET_LO(cpu->pc, READ(GET_T16(0)));
            irq_reset(cpu);
            irq_latch(cpu);
        CYCLE(6)
            SET_HI(cpu->pc, READ(GET_T16(0) + 1));
        CYCLE(7)
            /* end instantly! */
            /* HW interrupts do not increment the instruction counter */
            if (!cpu->take) --cpu->total_instructions;
#if !W65C02SCE_COARSE
            cpu->cycl = 0;
#endif
            SKIP_REST;
    END_INSTRUCTION
}

static unsigned mode_nop_5c(MODE_PARAMS) {
    /* op = NOP $5C */
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
        CYCLE(3)
            cpu->tr[1] = -1;
            READ(GET_T16(0));
        CYCLE(4)
            cpu->tr[0] = -1;
            READ(GET_T16(0));
        CYCLE(5)
            READ(GET_T16(0));
        CYCLE(6)
            READ(GET_T16(0));
            irq_latch(cpu);
        CYCLE(7)
            READ(GET_T16(0));
    END_INSTRUCTION
}

static unsigned mode_int_wait_stop(MODE_PARAMS) {
    /* op = WAI/STP */
    BEGIN_INSTRUCTION
        CYCLE(1)
            /* STP (1) or WAI (0) */
            cpu->take = cpu->oper == OPER_STP;
            READ(cpu->pc);
#if W65C02SCE_HOOK_STP
            if (cpu->take && cpu->hook_stp && (*cpu->hook_stp)()) SKIP_REST;
#endif
        CYCLE(2)
            READ(cpu->pc);
            if (!cpu->take && cpu->cpu_state == CPU_STATE_RUN) {
                cpu->cpu_state = CPU_STATE_WAIT;
                return 1; /* exit instantly if we entered WAI */
            }
        CYCLE(3)
            if (cpu->take) {
                cpu->cpu_state = CPU_STATE_STOP;
                return 1;
            }
            SKIP_REST;
    END_INSTRUCTION
}

static unsigned mode_implied_1c(MODE_PARAMS) {
    return 0; /* return immediately */
}

INTERNAL_INLINE void w65c02si_irq_latch(struct w65c02s_cpu *cpu) {
    irq_latch_slow(cpu);
}

INTERNAL void w65c02si_prerun_mode(struct w65c02s_cpu *cpu) {
    switch (cpu->mode) {
    case MODE_IMPLIED:
    case MODE_IMPLIED_X:
    case MODE_IMPLIED_Y:
    case MODE_IMMEDIATE:
    case MODE_RELATIVE:
        irq_latch(cpu);
    }
}

/* COARSE=0: true if there is still more to run, false if not */
/* COARSE=1: number of cycles */
INTERNAL unsigned w65c02si_run_mode(MODE_PARAMS) {
#define CALL_MODE(mode)                   mode_##mode(cpu, cont)
    switch (cpu->mode) {
    case MODE_RMW_ZEROPAGE:        return CALL_MODE(rmw_zeropage);
    case MODE_RMW_ZEROPAGE_X:      return CALL_MODE(rmw_zeropage_x);
    case MODE_RMW_ABSOLUTE:        return CALL_MODE(rmw_absolute);
    case MODE_RMW_ABSOLUTE_X:      return CALL_MODE(rmw_absolute_x);
    case MODE_STACK_PUSH:          return CALL_MODE(stack_push);
    case MODE_STACK_PULL:          return CALL_MODE(stack_pull);
    case MODE_IMPLIED_1C:          return CALL_MODE(implied_1c);
    case MODE_IMPLIED_X:           return CALL_MODE(implied_x);
    case MODE_IMPLIED_Y:           return CALL_MODE(implied_y);
    case MODE_IMPLIED:             return CALL_MODE(implied);
    case MODE_IMMEDIATE:           return CALL_MODE(immediate);
    case MODE_ZEROPAGE:            return CALL_MODE(zeropage);
    case MODE_ABSOLUTE:            return CALL_MODE(absolute);
    case MODE_ZEROPAGE_X:          return CALL_MODE(zeropage_x);
    case MODE_ZEROPAGE_Y:          return CALL_MODE(zeropage_y);
    case MODE_ABSOLUTE_X:          return CALL_MODE(absolute_x);
    case MODE_ABSOLUTE_Y:          return CALL_MODE(absolute_y);
    case MODE_ZEROPAGE_INDIRECT:   return CALL_MODE(zeropage_indirect);
    case MODE_ZEROPAGE_INDIRECT_X: return CALL_MODE(zeropage_indirect_x);
    case MODE_ZEROPAGE_INDIRECT_Y: return CALL_MODE(zeropage_indirect_y);
    case MODE_ABSOLUTE_JUMP:       return CALL_MODE(jump_absolute);
    case MODE_ABSOLUTE_INDIRECT:   return CALL_MODE(jump_indirect);
    case MODE_ABSOLUTE_INDIRECT_X: return CALL_MODE(jump_indirect_x);
    case MODE_SUBROUTINE:          return CALL_MODE(subroutine);
    case MODE_RETURN_SUB:          return CALL_MODE(return_sub);
    case MODE_STACK_BRK:           return CALL_MODE(stack_brk);
    case MODE_STACK_RTI:           return CALL_MODE(stack_rti);
    case MODE_RELATIVE:            return CALL_MODE(relative);
    case MODE_RELATIVE_BIT:        return CALL_MODE(relative_bit);
    case MODE_ZEROPAGE_BIT:        return CALL_MODE(zeropage_bit);
    case MODE_INT_WAIT_STOP:       return CALL_MODE(int_wait_stop);
    case MODE_NOP_5C:              return CALL_MODE(nop_5c);
    }
#undef CALL_MODE
    unreachable();
    return 0;
}

#endif /* W65C02SCE_SEPARATE */
