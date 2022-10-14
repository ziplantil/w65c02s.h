/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            w65c02s.h - main emulator definitions (and external API)
*******************************************************************************/

#ifndef W65C02SCE_W65C02S_H
#define W65C02SCE_W65C02S_H

/* defines */
/* 1: define uint8_t w65c02s_read(uint16_t); 
         and    void w65c02s_write(uint16_t, uint8_t); through linking. */
/* 0: define them through function pointers passed to w65c02s_init. */
#ifndef W65C02SCE_LINK
#define W65C02SCE_LINK 0
#endif

/* 1: allow hook_brk */
/* 0: do not allow hook_brk */
#ifndef W65C02SCE_HOOK_BRK
#define W65C02SCE_HOOK_BRK 0
#endif

/* 1: allow hook_stp */
/* 0: do not allow hook_stp */
#ifndef W65C02SCE_HOOK_STP
#define W65C02SCE_HOOK_STP 0
#endif

/* 1: allow hook_end_of_instruction */
/* 0: do not allow hook_end_of_instruction */
#ifndef W65C02SCE_HOOK_EOI
#define W65C02SCE_HOOK_EOI 0
#endif

/* 1: allow register access */
/* 0: do not allow register access */
#ifndef W65C02SCE_HOOK_REGS
#define W65C02SCE_HOOK_REGS 0
#endif

/* 1: has uint8_t, uint16_t without stdint.h */
/* 0: does not have uint8_t, uint16_t without stdint.h */
#ifndef W65C02SCE_HAS_UINTN_T
#define W65C02SCE_HAS_UINTN_T 0
#endif

/* 1: compile source files separately. */
/* 0: compiling w65c02s.c is enough, it includes the rest. */
/*    useful to simulate link-time optimizations, which is why it is default */
#ifndef W65C02SCE_SEPARATE
#define W65C02SCE_SEPARATE 0
#endif

#if __STDC_VERSION__ >= 199901L || HAS_STDINT_H || HAS_STDINT
#include <stdint.h>
#elif !W65C02SCE_HAS_UINTN_T
#include <limits.h>
#if UCHAR_MAX == 255
typedef unsigned char uint8_t;
#else
#error no uint8_t available
#endif
#if USHRT_MAX == 65535
typedef unsigned short uint16_t;
#else
#error no uint16_t available
#endif
#endif

#ifdef W65C02SCE
#if __STDC_VERSION__ >= 199901L
#define STATIC static inline
#if __GNUC__
#define INLINE __attribute__((always_inline)) STATIC
#elif defined(_MSC_VER)
#define INLINE __forceinline STATIC
#else
#define INLINE STATIC
#endif
#else
#define STATIC static
#define INLINE STATIC
#endif

#if W65C02SCE_SINGLEFILE
#define INTERNAL STATIC
#define INTERNAL_INLINE INLINE
#else
#define INTERNAL
#define INTERNAL_INLINE
#endif
#endif

/* DO NOT ACCESS THESE FIELDS YOURSELF IN EXTERNAL CODE!
   all values here are internal! do not rely on them! use methods instead! */
struct w65c02s_cpu {
    unsigned long total_cycles;
    unsigned long total_instructions;

    unsigned long left_cycles;

    uint16_t pc;
    uint8_t a, x, y, s, p, p_adj; /* p_adj for decimal mode */

    /* temporary registers used to store state between cycles */
    uint8_t tr[6];

#if !W65C02SCE_LINK
    uint8_t (*mem_read)(uint16_t);
    void (*mem_write)(uint16_t, uint8_t);
#endif
#if W65C02SCE_HOOK_BRK
    int (*hook_brk)(uint8_t);
#endif
#if W65C02SCE_HOOK_STP
    int (*hook_stp)(void);
#endif
#if W65C02SCE_HOOK_EOI
    void (*hook_eoi)(void);
#endif

    unsigned int nmi, reset, irq;   /* NMI, RESET, IRQ interrupt flags */
    unsigned int do_nmi, do_irq;    /* pending NMI or IRQ? */
    unsigned int in_nmi, in_irq;    /* currently entering NMI or IRQ? */
    unsigned int do_reset;          /* whether we are lining up for a reset */
    unsigned int stp, wai;          /* STP, flags */

    /* addressing mode, operation, cycle of current instruction */
    unsigned int mode, oper, cycl;
    /* running only one instruction? did we just run an instruction? */
    unsigned int step, had_instruction;
};

#if W65C02SCE_LINK
/* Reads value from memory at address */
extern uint8_t w65c02s_read(uint16_t address);
/* Writes value into memory at address */
extern void w65c02s_write(uint16_t address, uint8_t value);

/* Initialize w65c02s_cpu */
void w65c02s_init(struct w65c02s_cpu *cpu);
#else
/* Initialize w65c02s_cpu */
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(uint16_t),
                  void (*mem_write)(uint16_t, uint8_t));
#endif

/* Run CPU for given number of cycles */
void w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles);
/* Run CPU for given number of instructions.
   If finish_existing is specified, the current instruction, if any is ongoing,
   is executed to the end and only then the number of instructions specified. */
void w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                              unsigned long instructions,
                              int finish_existing);

/* get total number of cycles executed */
unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu);
/* get total number of instructions executed */
unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu);

/* reset total cycle counter */
void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu);
/* reset total instruction counter */
void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu);

/* checks if CPU is in WAI (waiting for instruction) */
int w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu);
/* checks if CPU is in STP (stopped execution) */
int w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu);

/* trigger CPU NMI */
void w65c02s_nmi(struct w65c02s_cpu *cpu);
/* trigger CPU RESET */
void w65c02s_reset(struct w65c02s_cpu *cpu);
/* trigger CPU IRQ */
void w65c02s_irq(struct w65c02s_cpu *cpu);
/* cancel CPU IRQ */
void w65c02s_irq_cancel(struct w65c02s_cpu *cpu);

#if W65C02SCE_HOOK_BRK
/* hook BRK instruction. BRK immediate value given as parameter. */
/* brk_hook return value: 0 = treat BRK as normal, <>0 = treat it as NOP */
void w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t));
#endif
#if W65C02SCE_HOOK_STP
/* hook STP instruction. */
/* stp_hook return value: 0 = treat STP as normal, <>0 = treat it as NOP */
void w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void));
#endif
#if W65C02SCE_HOOK_EOI
/* hook end-of-instruction. */
void w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                     void (*instruction_hook)(void));
#endif
/* sets the overflow bit (pin S/O) */
void w65c02s_set_overflow(struct w65c02s_cpu *cpu);

#if W65C02SCE_HOOK_REGS
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu);
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);

void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);
#endif

#endif /* W65C02SCE_W65C02S_H */
