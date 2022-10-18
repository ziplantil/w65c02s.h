/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-18

            oper.h - opcodes and internal operations
*******************************************************************************/

#ifndef W65C02SCE_OPER_H
#define W65C02SCE_OPER_H

#define W65C02SCE
#include "w65c02s.h"

/* get flag of P */
#define GET_P(flag) ((cpu->p & (flag)) ? 1 : 0)
/* set flag of P */
#define SET_P(flag, v) (cpu->p = (v) ? (cpu->p | (flag)) : (cpu->p & ~(flag)))
/* set flag of P_adj */
#define SET_P_ADJ(flag, v) \
    (cpu->p_adj = (v) ? (cpu->p_adj | (flag)) : (cpu->p_adj & ~(flag)))

/* P flags */
#define P_N 0x80
#define P_V 0x40
#define P_A1 0x20 /* N/C, always 1 */
#define P_B 0x10  /* always 1, but there are cases where it is pushed as 0 */
#define P_D 0x08
#define P_I 0x04
#define P_Z 0x02
#define P_C 0x01

/* stack offset */
#define STACK_OFFSET 0x0100U
#define STACK_ADDR(x) (STACK_OFFSET | ((x) & 0xFF))

/* vectors for NMI, RESET, IRQ */
#define VEC_NMI 0xFFFAU
#define VEC_RST 0xFFFCU
#define VEC_IRQ 0xFFFEU

/* helpers for getting/setting hi and lo bytes */
#define GET_HI(x) (((x) >> 8) & 0xFF)
#define GET_LO(x) ((x) & 0xFF)
#define SET_HI(x, v) ((x) = ((x) & 0x00FFU) | ((v) << 8))
#define SET_LO(x, v) ((x) = ((x) & 0xFF00U) | (v))
/* take tr[n] and tr[n + 1] as a 16-bit address. n is always even */
#define GET_T16(n) (cpu->tr[n] | (cpu->tr[n + 1] << 8))

/* returns 1 if a+b overflows, 0 if not (a, b are uint8_t) */
#define OVERFLOW8(a, b) (((unsigned)(a) + (unsigned)(b)) >> 8)

#if W65C02SCE_SEPARATE
INTERNAL uint8_t w65c02si_oper_rmw(struct w65c02s_cpu *cpu,
                                   unsigned op, uint8_t v);
INTERNAL_INLINE uint8_t w65c02si_oper_alu(struct w65c02s_cpu *cpu,
                                          unsigned op, uint8_t a, uint8_t b);
INTERNAL_INLINE void w65c02si_oper_cmp(struct w65c02s_cpu *cpu,
                                       uint8_t a, uint8_t b);
INTERNAL void w65c02si_oper_bit(struct w65c02s_cpu *cpu,
                                uint8_t a, uint8_t b);
INTERNAL_INLINE void w65c02si_oper_bit_imm(struct w65c02s_cpu *cpu,
                                           uint8_t a, uint8_t b);
INTERNAL uint8_t w65c02si_oper_tsb(struct w65c02s_cpu *cpu,
                                   uint8_t a, uint8_t b, bool set);
INTERNAL bool w65c02si_oper_branch(unsigned op, uint8_t p);
INTERNAL uint8_t w65c02si_oper_bitset(unsigned oper, uint8_t v);
INTERNAL bool w65c02si_oper_bitbranch(unsigned oper, uint8_t v);
#endif /* W65C02SCE_SEPARATE */

#endif /* W65C02SCE_OPER_H */
