/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-15

            mode.c - addressing modes
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "mode.h"
#include "modejump.h"
#include "oper.h"

STATIC void stack_push(struct w65c02s_cpu *cpu, uint8_t v) {
    uint16_t s = STACK_ADDR(cpu->s--);
    WRITE(s, v);
}

STATIC uint8_t stack_pull(struct w65c02s_cpu *cpu) {
    uint16_t s = STACK_ADDR(++cpu->s);
    return READ(s);
}

STATIC uint8_t update_flags(struct w65c02s_cpu *cpu, uint8_t q) {
    /* N: bit 7 of result. Z: whether result is 0 */
    SET_P(P_N, q & 0x80);
    SET_P(P_Z, q == 0);
    return q;
}

STATIC void irq_latch(struct w65c02s_cpu *cpu) {
    cpu->do_nmi |= cpu->nmi;
    cpu->do_irq = cpu->irq && !GET_P(P_I);
}

STATIC void irq_latch_or(struct w65c02s_cpu *cpu) {
    cpu->do_nmi |= cpu->nmi;
    cpu->do_irq |= cpu->irq && !GET_P(P_I);
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
STATIC int oper_is_store(struct w65c02s_cpu *cpu) {
    switch (cpu->oper) {
        case OPER_STA:
        case OPER_STX:
        case OPER_STY:
        case OPER_STZ:
            return 1;
    }
    return 0;
}

INLINE unsigned decimal_penalty(struct w65c02s_cpu *cpu) {
    return GET_P(P_D) && (cpu->oper == OPER_ADC || cpu->oper == OPER_SBC);
}

INLINE unsigned fast_rmw_absx(int oper) {
    return oper != OPER_INC && oper != OPER_DEC;
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
    switch (cpu->oper) {
        case OPER_NOP:
            break;

        case OPER_AND:
        case OPER_EOR:
        case OPER_ORA:
        case OPER_ADC:
        case OPER_SBC:
            cpu->a = w65c02si_oper_alu(cpu, cpu->oper, cpu->a, v);
            return decimal_penalty(cpu);

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
    switch (cpu->oper) {
        case OPER_NOP:
            READ(a);
            break;

        case OPER_AND:
        case OPER_EOR:
        case OPER_ORA:
        case OPER_ADC:
        case OPER_SBC:
            cpu->a = w65c02si_oper_alu(cpu, cpu->oper, cpu->a, READ(a));
            return decimal_penalty(cpu);

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

#define ADC_D_SPURIOUS(ea)                                                     \
    if (!cpu->take) SKIP_REST; /* no penalty cycle => skip last read */        \
    cpu->p = cpu->p_adj;                                                       \
    READ(ea);

STATIC bool mode_implied(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            switch (cpu->oper) {
                case OPER_NOP:
                    break;

                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->a = w65c02si_oper_rmw(cpu, cpu->oper, cpu->a);
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
            READ(cpu->pc);
    END_INSTRUCTION(2)
}

STATIC bool mode_implied_x(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->x = w65c02si_oper_rmw(cpu, cpu->oper, cpu->x);
            READ(cpu->pc);
    END_INSTRUCTION(2)
}

STATIC bool mode_implied_y(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->y = w65c02si_oper_rmw(cpu, cpu->oper, cpu->y);
            READ(cpu->pc);
    END_INSTRUCTION(2)
}

STATIC bool mode_immediate(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->take = oper_imm(cpu, READ(cpu->pc++)); /* penalty cycle? */
        CYCLE(2)
            ADC_D_SPURIOUS(cpu->pc);
    END_INSTRUCTION(3)
}

STATIC bool mode_zeropage(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(2)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(3)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION(4)
}

STATIC bool mode_zeropage_x(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[0] += cpu->x;
            READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION(5)
}

STATIC bool mode_zeropage_y(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc);
        CYCLE(2)
            cpu->tr[0] += cpu->y;
            READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, cpu->tr[0]); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(cpu->tr[0]);
    END_INSTRUCTION(5)
}

STATIC bool mode_absolute(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(3)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(4)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(5)
}

STATIC bool mode_absolute_x(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->x); /* page wrap cycle? */
            cpu->tr[0] += cpu->x;
            cpu->tr[1] = READ(cpu->pc++);
            cpu->take = cpu->tr[4] || oper_is_store(cpu);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(4);
            READ(!cpu->tr[4] ? GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(4)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(6)
}

STATIC bool mode_absolute_y(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->y); /* page wrap cycle? */
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = READ(cpu->pc++);
            cpu->take = cpu->tr[4] || oper_is_store(cpu);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(4);
            READ(!cpu->tr[4] ? GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(4)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(6)
}

STATIC bool mode_zeropage_indirect(struct w65c02s_cpu *cpu) {
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
            if (cpu->take) irq_latch(cpu);
        CYCLE(5)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(6)
}

STATIC bool mode_zeropage_indirect_x(struct w65c02s_cpu *cpu) {
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
            if (cpu->take) irq_latch(cpu);
        CYCLE(6)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(7)
}

STATIC bool mode_zeropage_indirect_y(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[2] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[0] = READ(cpu->tr[2]++);
        CYCLE(3)
            cpu->tr[4] = OVERFLOW8(cpu->tr[0], cpu->y); /* page wrap cycle? */
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = READ(cpu->tr[2]);
            cpu->take = cpu->tr[4] || oper_is_store(cpu);
            if (!cpu->take) irq_latch(cpu);
        CYCLE(4)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) SKIP_TO_NEXT(5);
            READ(cpu->tr[2]);
            cpu->tr[1] += cpu->tr[4];
            irq_latch(cpu);
        CYCLE(5)
            cpu->take = oper_addr(cpu, GET_T16(0)); /* penalty cycle? */
            if (cpu->take) irq_latch(cpu);
        CYCLE(6)
            ADC_D_SPURIOUS(GET_T16(0));
    END_INSTRUCTION(7)
}

STATIC bool mode_jump_absolute(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            irq_latch(cpu);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc);
            cpu->pc = GET_T16(0);
    END_INSTRUCTION(3)
}

STATIC bool mode_jump_indirect(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

STATIC bool mode_jump_indirect_x(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

STATIC bool mode_zeropage_bit(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->tr[0]);
        CYCLE(3)
            cpu->tr[1] = w65c02si_oper_bitset(cpu->oper, cpu->tr[1]);
            READ(cpu->tr[0]);
            irq_latch(cpu);
        CYCLE(4)
            WRITE(cpu->tr[0], cpu->tr[1]);
    END_INSTRUCTION(5)
}

STATIC bool mode_relative(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        /* 0: irq_latch(cpu); (see execute.c) */
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
            compute_branch(cpu);
            irq_latch(cpu);
        CYCLE(2)
            /* skip the rest of the instruction if branch is not taken */
            if (!w65c02si_oper_branch(cpu->oper, cpu->p)) SKIP_REST;
            READ(GET_T16(0));
            cpu->pc = GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) irq_latch_or(cpu);
        CYCLE(3)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) SKIP_REST;
            READ(GET_T16(0));
    END_INSTRUCTION(4)
}

STATIC bool mode_relative_bit(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[3] = READ(cpu->pc++);
        CYCLE(2)
            READ(cpu->tr[3]);
        CYCLE(3)
            cpu->tr[4] = READ(cpu->tr[3]);
        CYCLE(4)
            cpu->take = w65c02si_oper_bitbranch(cpu->oper, cpu->tr[4]);
            cpu->tr[0] = READ(cpu->pc++);
            compute_branch(cpu);
        CYCLE(5)
            /* skip the rest of the instruction if branch is not taken */
            if (!cpu->take) SKIP_REST;
            READ(GET_T16(0));
            cpu->pc = GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) irq_latch_or(cpu);
        CYCLE(6)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) SKIP_REST;
            READ(GET_T16(0));
    END_INSTRUCTION(7)
}

STATIC bool mode_rmw_zeropage(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->tr[0]);
        CYCLE(3)
            READ(cpu->tr[0]);
            switch (cpu->oper) {
                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->tr[1] = w65c02si_oper_rmw(cpu, cpu->oper, cpu->tr[1]);
                    break;
                case OPER_TSB:
                    cpu->tr[1] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[1], 1);
                    break;
                case OPER_TRB:
                    cpu->tr[1] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[1], 0);
                    break;
                default: unreachable();
            }
            irq_latch(cpu);
        CYCLE(4)
            WRITE(cpu->tr[0], cpu->tr[1]);
    END_INSTRUCTION(5)
}

STATIC bool mode_rmw_zeropage_x(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

STATIC bool mode_rmw_absolute(struct w65c02s_cpu *cpu) {
    BEGIN_INSTRUCTION
        CYCLE(1)
            cpu->tr[0] = READ(cpu->pc++);
        CYCLE(2)
            cpu->tr[1] = READ(cpu->pc++);
        CYCLE(3)
            cpu->tr[2] = READ(GET_T16(0));
        CYCLE(4)
            READ(GET_T16(0));
            switch (cpu->oper) {
                case OPER_DEC:
                case OPER_INC:
                case OPER_ASL:
                case OPER_ROL:
                case OPER_LSR:
                case OPER_ROR:
                    cpu->tr[2] = w65c02si_oper_rmw(cpu, cpu->oper, cpu->tr[2]);
                    break;
                case OPER_TSB:
                    cpu->tr[2] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[2], 1);
                    break;
                case OPER_TRB:
                    cpu->tr[2] = w65c02si_oper_tsb(cpu, cpu->a, cpu->tr[2], 0);
                    break;
                default: unreachable();
            }
            irq_latch(cpu);
        CYCLE(5)
            WRITE(GET_T16(0), cpu->tr[2]);
    END_INSTRUCTION(6)
}

STATIC bool mode_rmw_absolute_x(struct w65c02s_cpu *cpu) {
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
            cpu->tr[2] = w65c02si_oper_rmw(cpu, cpu->oper, cpu->tr[2]);
            irq_latch(cpu);
        CYCLE(6)
            WRITE(GET_T16(0), cpu->tr[2]);
    END_INSTRUCTION(7)
}

STATIC bool mode_stack_push(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(3)
}

STATIC bool mode_stack_pull(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(4)
}

STATIC bool mode_subroutine(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

STATIC bool mode_return_sub(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

STATIC bool mode_stack_rti(struct w65c02s_cpu *cpu) {
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
    END_INSTRUCTION(6)
}

static int mode_stack_brk(struct w65c02s_cpu *cpu) {
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
                cpu->do_irq = cpu->in_irq = 0;
                cpu->in_nmi = 1;
                cpu->nmi = 0;
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
        CYCLE(6)
            irq_reset(cpu);
            irq_latch(cpu);
            SET_HI(cpu->pc, READ(GET_T16(0) + 1));
    END_INSTRUCTION(7)
}

STATIC bool mode_nop_5c(struct w65c02s_cpu *cpu) {
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
        CYCLE(7)
            irq_latch(cpu);
            READ(GET_T16(0));
    END_INSTRUCTION(8)
}

STATIC bool mode_int_wait_stop(struct w65c02s_cpu *cpu) {
    /* op = WAI/STP */
    BEGIN_INSTRUCTION
        CYCLE(1)
            READ(cpu->pc);
#if W65C02SCE_HOOK_STP
            if (cpu->oper == OPER_STP && cpu->hook_stp
                                      && (*cpu->hook_stp)()) SKIP_REST;
#endif
        CYCLE(2)
            READ(cpu->pc);
            if (cpu->oper == OPER_WAI && !(cpu->irq || cpu->nmi)) {
                cpu->wai = 1;
                return 1; /* exit instantly if we entered WAI */
            }
        CYCLE(3)
            if (cpu->oper == OPER_STP) {
                cpu->stp = 1;
                return 1;
            }
            SKIP_REST;
    END_INSTRUCTION(4)
}

STATIC bool mode_implied_1c(struct w65c02s_cpu *cpu) {
    return 0; /* return immediately */
}

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

INTERNAL void w65c02si_prerun_mode(struct w65c02s_cpu *cpu) {
    if (irq_latch_on_cycle_0[cpu->oper])
        irq_latch(cpu);
}

/* =0: instruction finished, <>0: instruction did not finish */
INTERNAL bool w65c02si_run_mode(struct w65c02s_cpu *cpu) {
    switch (cpu->mode) {
    case MODE_IMPLIED:             return mode_implied(cpu);
    case MODE_IMMEDIATE:           return mode_immediate(cpu);
    case MODE_RELATIVE:            return mode_relative(cpu);
    case MODE_RELATIVE_BIT:        return mode_relative_bit(cpu);
    case MODE_ZEROPAGE:            return mode_zeropage(cpu);
    case MODE_ZEROPAGE_X:          return mode_zeropage_x(cpu);
    case MODE_ZEROPAGE_Y:          return mode_zeropage_y(cpu);
    case MODE_ZEROPAGE_BIT:        return mode_zeropage_bit(cpu);
    case MODE_ABSOLUTE:            return mode_absolute(cpu);
    case MODE_ABSOLUTE_X:          return mode_absolute_x(cpu);
    case MODE_ABSOLUTE_Y:          return mode_absolute_y(cpu);
    case MODE_ZEROPAGE_INDIRECT:   return mode_zeropage_indirect(cpu);
    case MODE_ZEROPAGE_INDIRECT_X: return mode_zeropage_indirect_x(cpu);
    case MODE_ZEROPAGE_INDIRECT_Y: return mode_zeropage_indirect_y(cpu);
    case MODE_ABSOLUTE_INDIRECT:   return mode_jump_indirect(cpu);
    case MODE_ABSOLUTE_INDIRECT_X: return mode_jump_indirect_x(cpu);
    case MODE_ABSOLUTE_JUMP:       return mode_jump_absolute(cpu);
    case MODE_IMPLIED_1C:          return mode_implied_1c(cpu);
    case MODE_IMPLIED_X:           return mode_implied_x(cpu);
    case MODE_IMPLIED_Y:           return mode_implied_y(cpu);
    case MODE_RMW_ZEROPAGE:        return mode_rmw_zeropage(cpu);
    case MODE_RMW_ZEROPAGE_X:      return mode_rmw_zeropage_x(cpu);
    case MODE_SUBROUTINE:          return mode_subroutine(cpu);
    case MODE_RETURN_SUB:          return mode_return_sub(cpu);
    case MODE_RMW_ABSOLUTE:        return mode_rmw_absolute(cpu);
    case MODE_RMW_ABSOLUTE_X:      return mode_rmw_absolute_x(cpu);
    case MODE_NOP_5C:              return mode_nop_5c(cpu);
    case MODE_INT_WAIT_STOP:       return mode_int_wait_stop(cpu);
    case MODE_STACK_PUSH:          return mode_stack_push(cpu);
    case MODE_STACK_PULL:          return mode_stack_pull(cpu);
    case MODE_STACK_BRK:           return mode_stack_brk(cpu);
    case MODE_STACK_RTI:           return mode_stack_rti(cpu);
    }
    unreachable();
    return 0;
}

#endif /* W65C02SCE_SEPARATE */
