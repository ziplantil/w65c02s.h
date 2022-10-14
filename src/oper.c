/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            oper.c - opcodes and internal operations
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"
#include "oper.h"

INLINE uint8_t update_flags_nz(struct w65c02s_cpu *cpu, uint8_t q) {
    /* N: bit 7 of result. Z: whether result is 0 */
    SET_P(P_N, q & 0x80);
    SET_P(P_Z, q == 0);
    return q;
}

INLINE uint8_t update_flags_nzc(struct w65c02s_cpu *cpu,
                                unsigned q, unsigned c) {
    SET_P(P_C, c);
    return update_flags_nz(cpu, q & 255);
}

INLINE uint8_t update_flags_nzc_adc(struct w65c02s_cpu *cpu, unsigned q) {
    /* q is a 9-bit value with carry at bit 8 */
    return update_flags_nzc(cpu, q, q >> 8);
}

INLINE uint8_t oper_inc(struct w65c02s_cpu *cpu, uint8_t v) {
    return update_flags_nz(cpu, v + 1);
}

INLINE uint8_t oper_dec(struct w65c02s_cpu *cpu, uint8_t v) {
    return update_flags_nz(cpu, v - 1);
}

INLINE uint8_t oper_asl(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    return update_flags_nzc(cpu, v << 1, v >> 7);
}

INLINE uint8_t oper_lsr(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    return update_flags_nzc(cpu, v >> 1, v & 1);
}

INLINE uint8_t oper_rol(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    return update_flags_nzc(cpu, (v << 1) | GET_P(P_C), v >> 7);
}

INLINE uint8_t oper_ror(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    return update_flags_nzc(cpu, (v >> 1) | (GET_P(P_C) << 7), v & 1);
}

INTERNAL uint8_t w65c02si_oper_rmw(struct w65c02s_cpu *cpu,
                                   unsigned op, uint8_t v) {
    switch (op) {
        case OPER_ASL: return oper_asl(cpu, v);
        case OPER_DEC: return oper_dec(cpu, v);
        case OPER_INC: return oper_inc(cpu, v);
        case OPER_LSR: return oper_lsr(cpu, v);
        case OPER_ROL: return oper_rol(cpu, v);
        case OPER_ROR: return oper_ror(cpu, v);
    }
    return v;
}

INTERNAL void w65c02si_oper_cmp(struct w65c02s_cpu *cpu,
                                uint8_t a, uint8_t b) {
    update_flags_nzc_adc(cpu, a + (uint8_t)(~b) + 1);
}

INTERNAL void w65c02si_oper_bit(struct w65c02s_cpu *cpu,
                                uint8_t a, uint8_t b) {
    /* in BIT, N and V are bits 7 and 6 of the memory operand */
    SET_P(P_N, (b >> 7) & 1);
    SET_P(P_V, (b >> 6) & 1);
    SET_P(P_Z, !(a & b));
}

INTERNAL void w65c02si_oper_bit_imm(struct w65c02s_cpu *cpu,
                                    uint8_t a, uint8_t b) {
    SET_P(P_Z, !(a & b));
}

INTERNAL uint8_t w65c02si_oper_tsb(struct w65c02s_cpu *cpu,
                                   uint8_t a, uint8_t b, int set) {
    SET_P(P_Z, !(a & b));
    return set ? b | a : b & ~a;
}

INLINE unsigned oper_adc_v(uint8_t a, uint8_t b, uint8_t c) {
    unsigned c6 = ((a & 0x7F) + (b & 0x7F) + c) >> 7;
    unsigned c7 = (a + b + c) >> 8;
    return c6 ^ c7;
}

STATIC uint8_t oper_adc_d(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b,
                          unsigned c) {
    /* BCD addition one nibble/digit at a time */
    unsigned lo, hi, hc, fc, q;
    cpu->p_adj = cpu->p;

    lo = (a & 15) + (b & 15) + c;
    hc = lo >= 10; /* half carry */
    if (hc) lo = (lo - 10) & 15;

    hi = (a >> 4) + (b >> 4) + hc;
    fc = hi >= 10; /* full carry */
    if (fc) hi = (hi - 10) & 15;

    q = (hi << 4) | lo;
    SET_P_ADJ(P_N, q >> 7);
    SET_P_ADJ(P_Z, q == 0);
    SET_P_ADJ(P_C, fc);
    /* keep P_V as in binary addition */
    return q;
}

STATIC uint8_t oper_sbc_d(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b,
                          unsigned c) {
    /* BCD subtraction one nibble/digit at a time */
    unsigned lo, hi, hc, fc, q;
    cpu->p_adj = cpu->p;
    
    lo = (a & 15) + (b & 15) + c;
    hc = lo >= 16; /* half carry */
    lo = (hc ? lo : lo + 10) & 15;

    hi = (a >> 4) + (b >> 4) + hc;
    fc = hi >= 16; /* full carry */
    hi = (fc ? hi : hi + 10) & 15;

    q = (hi << 4) | lo;
    SET_P_ADJ(P_N, q >> 7);
    SET_P_ADJ(P_Z, q == 0);
    SET_P_ADJ(P_C, fc);
    /* keep P_V as in binary addition */
    return q;
}

INLINE uint8_t oper_adc(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b) {
    uint8_t r;
    unsigned c = GET_P(P_C);
    SET_P(P_V, oper_adc_v(a, b, c));
    r = update_flags_nzc_adc(cpu, a + b + c);
    if (!GET_P(P_D)) return r;
    return oper_adc_d(cpu, a, b, c);
}

INLINE uint8_t oper_sbc(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b) {
    uint8_t r;
    unsigned c = GET_P(P_C);
    b = ~b;
    SET_P(P_V, oper_adc_v(a, b, c));
    r = update_flags_nzc_adc(cpu, a + b + c);
    if (!GET_P(P_D)) return r;
    return oper_sbc_d(cpu, a, b, c);
}

INTERNAL uint8_t w65c02si_oper_alu(struct w65c02s_cpu *cpu,
                                   unsigned op, uint8_t a, uint8_t b) {
    switch (op) {
        case OPER_AND: return update_flags_nz(cpu, a & b);
        case OPER_EOR: return update_flags_nz(cpu, a ^ b);
        case OPER_ORA: return update_flags_nz(cpu, a | b);
        case OPER_ADC: return oper_adc(cpu, a, b);
        case OPER_SBC: return oper_sbc(cpu, a, b);
    }
    return update_flags_nz(cpu, b);
}

INTERNAL unsigned w65c02si_oper_branch(unsigned op, uint8_t p) {
    /* whether to take the branch? */
    switch (op) {
        case OPER_BPL: return !(p & P_N);
        case OPER_BMI: return  (p & P_N);
        case OPER_BVC: return !(p & P_V);
        case OPER_BVS: return  (p & P_V);
        case OPER_BCC: return !(p & P_C);
        case OPER_BCS: return  (p & P_C);
        case OPER_BNE: return !(p & P_Z);
        case OPER_BEQ: return  (p & P_Z);
    }
    return 1; /* BRA */
}

INTERNAL uint8_t w65c02si_oper_bitset(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v | mask : v & ~mask;
}

INTERNAL unsigned w65c02si_oper_bitbranch(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v & mask : !(v & mask);
}

#endif /* W65C02SCE_SEPARATE */
