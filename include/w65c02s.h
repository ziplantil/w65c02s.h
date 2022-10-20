/*******************************************************************************
            w65c02s.h -- cycle-accurate C emulator of the WDC 65C02S
                         as a single-header library
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-20
*******************************************************************************/

#ifndef W65C02S_H
#define W65C02S_H

/* defines */

/* 1: always runs instructions as a whole */
/* 0: accurate cycle counter, can stop and resume mid-instruction. */
#ifndef W65C02S_COARSE
#define W65C02S_COARSE 0
#endif

/* 1: cycle counter is updated only at the end of an execution loop */
/* 0: cycle counter is accurate even in callbacks */
#ifndef W65C02S_COARSE_CYCLE_COUNTER
#define W65C02S_COARSE_CYCLE_COUNTER 0
#endif

/* 1: define uint8_t w65c02s_read(uint16_t); 
         and    void w65c02s_write(uint16_t, uint8_t); through linking. */
/* 0: define them through function pointers passed to w65c02s_init. */
#ifndef W65C02S_LINK
#define W65C02S_LINK 0
#endif

/* 1: allow hook_brk */
/* 0: do not allow hook_brk */
#ifndef W65C02S_HOOK_BRK
#define W65C02S_HOOK_BRK 0
#endif

/* 1: allow hook_stp */
/* 0: do not allow hook_stp */
#ifndef W65C02S_HOOK_STP
#define W65C02S_HOOK_STP 0
#endif

/* 1: allow hook_end_of_instruction */
/* 0: do not allow hook_end_of_instruction */
#ifndef W65C02S_HOOK_EOI
#define W65C02S_HOOK_EOI 0
#endif

/* 1: has bool without stdbool.h */
/* 0: does not have bool without stdbool.h */
#ifndef W65C02S_HAS_BOOL 
#define W65C02S_HAS_BOOL 0
#endif

/* 1: has uint8_t, uint16_t without stdint.h */
/* 0: does not have uint8_t, uint16_t without stdint.h */
#ifndef W65C02S_HAS_UINTN_T
#define W65C02S_HAS_UINTN_T 0
#endif

#if __STDC_VERSION__ >= 199901L
#define W65C02S_C99 1
#endif

#if __STDC_VERSION__ >= 201112L
#define W65C02S_C11 1
#endif

#if __STDC_VERSION__ >= 202309L
#define W65C02S_C23 1
#endif

#if __GNUC__ >= 5
#define W65C02S_GNUC 1
#endif

#if defined(_MSC_VER)
#define W65C02S_MSVC 1
#endif

#include <limits.h>

#if W65C02S_HAS_UINTN_T
/* do nothing, we have these types */
#elif W65C02S_C99 || HAS_STDINT_H || HAS_STDINT
#include <stdint.h>
#else
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

#if W65C02S_HAS_BOOL
/* has bool */
#elif W65C02S_C99 || HAS_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#endif

struct w65c02s_cpu;

/** w65c02s_init
 *
 *  Initializes a new CPU instance.
 *
 *  mem_read and mem_write only need to be specified if W65C02S_LINK is set
 *  to 0. If set to 1, they may simply be passed as NULL.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: mem_read] The read callback, ignored if W65C02S_LINK is 1.
 *  [Parameter: mem_write] The write callback, ignored if W65C02S_LINK is 1.
 *  [Parameter: cpu_data] The pointer to be returned by w65c02s_get_cpu_data().
 *  [Return value] The number of cycles that were actually run
 */
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data);

#if W65C02S_LINK
/** w65c02s_read
 *
 *  Reads a value from memory.
 *
 *  This method only exists if the library is compiled with W65C02S_LINK.
 *  In that case, you must provide an implementation.
 *
 *  [Parameter: address] The 16-bit address to read from
 *  [Return value] The value read from memory
 */
extern uint8_t w65c02s_read(uint16_t address);

/** w65c02s_write
 *
 *  Writes a value to memory.
 *
 *  This method only exists if the library is compiled with W65C02S_LINK.
 *  In that case, you must provide an implementation.
 *
 *  [Parameter: address] The 16-bit address to write to
 *  [Parameter: value] The value to write
 */
extern void w65c02s_write(uint16_t address, uint8_t value);
#endif

/** w65c02s_run_cycles
 *
 *  Runs the CPU for the given number of cycles.
 *
 *  If w65c02ce is compiled with W65C02S_COARSE, the actual number of
 *  cycles executed may not always match the given argument, but the return
 *  value is always correct. Without W65C02S_COARSE the return value is
 *  always the same as `cycles`.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: cycles] The number of cycles to run
 *  [Return value] The number of cycles that were actually run
 */
unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles);

/** w65c02s_step_instruction
 *
 *  Runs the CPU for one instruction, or if an instruction is already running,
 *  finishes that instruction.
 *
 *  Prefer using w65c02s_run_instructions or w65c02s_run_cycles instead if you
 *  can to run many instructions at once.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles that were actually run
 */
unsigned long w65c02s_step_instruction(struct w65c02s_cpu *cpu);

/** w65c02s_run_instructions
 *
 *  Runs the CPU for the given number of instructions.
 *
 *  finish_existing only matters if the CPU is currently mid-instruction.
 *  If it is, specifying finish_existing as true will finish the current
 *  instruction before running any new ones, and the existing instruction
 *  will not count towards the instruction count limit. If finish_existing is
 *  false, the current instruction will be counted towards the limit.
 *
 *  Entering an interrupt counts as an instruction here.
 *
 *  This function is not reentrant. Calling it from a callback (for a memory
 *  read, write, STP, etc.) will result in undefined behavior.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: instructions] The number of instructions to run
 *  [Parameter: finish_existing] Whether to finish the current instruction
 *  [Return value] The number of cycles that were actually run
 *                 (mind the overflow with large values of instructions!)
 */
unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       int finish_existing);

/** w65c02s_get_cycle_count
 *
 *  Gets the total number of cycles executed by this CPU.
 *
 *  If the library has been compiled with W65C02S_COARSE_CYCLE_COUNTER,
 *  this value might not be updated between calls to `w65c02s_run_cycles`
 *  or `w65c02s_run_instructions`.
 *
 *  If it hasn't, the value returned by this function reflects how many
 *  cycles have been run. For example, on the first reset cycle (spurious
 *  read of the PC) of a new CPU instance, this will return 0.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles run in total
 */
unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu);

/** w65c02s_get_instruction_count
 *
 *  Gets the total number of instructions executed by this CPU.
 *
 *  Entering an interrupt counts does not count as an instruction here.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Return value] The number of cycles run in total
 */
unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu);

/** w65c02s_get_cpu_data
 *
 *  Gets the cpu_data pointer associated to the CPU with w65c02s_init.
 *
 *  [Parameter: cpu] The CPU instance to get the pointer from
 *  [Return value] The cpu_data pointer
 */
void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu);

/** w65c02s_reset_cycle_count
 *
 *  Resets the total cycle counter (w65c02s_get_cycle_count).
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu);

/** w65c02s_reset_instruction_count
 *
 *  Resets the total instruction counter (w65c02s_get_instruction_count).
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu);

/** w65c02s_is_cpu_waiting
 *
 *  Checks whether the CPU is currently waiting for an interrupt (WAI).
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] Whether the CPU ran a WAI instruction and is waiting
 */
bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu);

/** w65c02s_is_cpu_stopped
 *
 *  Checks whether the CPU is currently stopped and waiting for a reset.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] Whether the CPU ran a STP instruction
 */
bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu);

/** w65c02s_nmi
 *
 *  Queues a NMI (non-maskable interrupt) on the CPU.
 *
 *  The NMI will generally execute before the next instruction, but it must
 *  be triggered before the final cycle of an instruction, or it will be
 *  postponed to after the next instruction. NMIs cannot be masked.
 *  Calling this function will trigger an NMI only once.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_nmi(struct w65c02s_cpu *cpu);

/** w65c02s_reset
 *
 *  Triggers a CPU reset.
 *
 *  The RESET will begin executing before the next instruction. The RESET
 *  does not behave exactly like in the original chip, and will not carry out
 *  any extra cycles in between the end of the instruction and beginning of
 *  the reset (like some chips do). There is also no requirement that RESET be
 *  asserted before the last cycle, like with NMI or IRQ.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_reset(struct w65c02s_cpu *cpu);

/** w65c02s_irq
 *
 *  Pulls the IRQ line high (logically) on the CPU.
 *
 *  If interrupts are enabled on the CPU, the IRQ handler will execute before
 *  the next instruction. Like with NMI, an IRQ on the last cycle will be
 *  delayed until after the next instruction. The IRQ line is held until
 *  w65c02s_irq_cancel is called.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_irq(struct w65c02s_cpu *cpu);

/** w65c02s_irq_cancel
 *
 *  Pulls the IRQ line low (logically) on the CPU.
 *
 *  The IRQ is cancelled and will no longer be triggered. Triggering an IRQ
 *  only once is tricky. w65c02s_irq and w65c02s_irq_cancel immediately will
 *  not cause an IRQ (since the CPU never samples it). It must be held for
 *  at least 7-8 cycles, but that may still not be enough if interrupts are
 *  disabled from the CPU side.
 *
 *  Generally you would want to hold the IRQ line high and cancel the IRQ
 *  only once it has been acknowledged by the CPU (e.g. through MMIO). 
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_irq_cancel(struct w65c02s_cpu *cpu);

/** w65c02s_set_overflow
 *
 *  Sets the overflow (V) flag on the status register (P) of the CPU.
 *
 *  This corresponds to the S/O pin on the physical CPU.
 *
 *  [Parameter: cpu] The CPU instance
 */
void w65c02s_set_overflow(struct w65c02s_cpu *cpu);

/** w65c02s_hook_brk
 *
 *  Hooks the BRK instruction on the CPU.
 *
 *  The hook function should take a single uint8_t parameter, which corresponds
 *  to the immediate parameter after the BRK opcode. If the hook function
 *  returns a non-zero value, the BRK instruction is skipped, and otherwise
 *  it is treated as normal.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_BRK.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: brk_hook] The new BRK hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02S_HOOK_BRK)
 */
int w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t));

/** w65c02s_hook_stp
 *
 *  Hooks the STP instruction on the CPU.
 *
 *  The hook function should take no parameters. If it returns a non-zero
 *  value, the STP instruction is skipped, and otherwise it is treated
 *  as normal.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_STP.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: stp_hook] The new STP hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02S_HOOK_STP)
 */
int w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void));

/** w65c02s_hook_end_of_instruction
 *
 *  Hooks the end-of-instruction on the CPU.
 *
 *  The hook function should take no parameters. It is called when an
 *  instruction finishes. The interrupt entering routine counts
 *  as an instruction here.
 *
 *  Passing NULL as the hook disables the hook.
 *
 *  This function does nothing if the library was not compiled with
 *  W65C02S_HOOK_EOI.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: instruction_hook] The new end-of-instruction hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02S_HOOK_EOI)
 */
int w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                    void (*instruction_hook)(void));

/** w65c02s_reg_get_a
 *
 *  Returns the value of the A register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the A register
 */
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_x
 *
 *  Returns the value of the X register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the X register
 */
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_y
 *
 *  Returns the value of the Y register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the Y register
 */
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_p
 *
 *  Returns the value of the P (processor status) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the P register
 */
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_s
 *
 *  Returns the value of the S (stack pointer) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the S register
 */
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_pc
 *
 *  Returns the value of the PC (program counter) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the PC register
 */
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_set_a
 *
 *  Replaces the value of the A register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the A register
 */
void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_x
 *
 *  Replaces the value of the X register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the X register
 */
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_y
 *
 *  Replaces the value of the Y register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the Y register
 */
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_p
 *
 *  Replaces the value of the P (processor status) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the P register
 */
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_s
 *
 *  Replaces the value of the S (stack pointer) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the S register
 */
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_pc
 *
 *  Replaces the value of the PC (program counter) register on the CPU.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the PC register
 */
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);

#endif /* W65C02S_H */

#if W65C02S_IMPL



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                                 STRUCT                                 | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* please treat w65c02s_cpu as an opaque type */
struct w65c02s_cpu {
#if !W65C02S_COARSE_CYCLE_COUNTER
    unsigned long total_cycles;
#endif
#if !W65C02S_COARSE
#if W65C02S_COARSE_CYCLE_COUNTER
    unsigned long left_cycles;
#else
    unsigned long target_cycles;
#endif
#endif
    unsigned cpu_state;
    /* currently active interrupts, interrupt mask */
    unsigned int_trig, int_mask;

    /* temporary true/false */
    bool take;

#if W65C02S_C11
    _Alignas(2)
#endif
    /* temporary registers used to store state between cycles. */
    uint8_t tr[5];

    /* 6502 registers: PC, A, X, Y, S (stack pointer), P (flags). */
    uint16_t pc;
    uint8_t a, x, y, s, p, p_adj;
    /* p_adj for decimal mode; it contains the "correct" flags. */

    /* addressing mode, operation */
    unsigned int mode, oper;
#if !W65C02S_COARSE
    /* cycle of current instruction */
    unsigned int cycl;
#endif

    unsigned long total_instructions;
#if W65C02S_COARSE_CYCLE_COUNTER
    unsigned long total_cycles;
#endif

    /* entering NMI, resetting or IRQ? */
    bool in_nmi, in_rst, in_irq;

    /* BRK, STP, end of instruction hooks */
    int (*hook_brk)(uint8_t);
    int (*hook_stp)(void);
    void (*hook_eoi)(void);
    /* data pointer from w65c02s_init */
    void *cpu_data;
#if !W65C02S_LINK
    /* memory read/write callbacks */
    uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t);
    void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t);
#endif
};



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                         MACROS AND ENUMERATIONS                        | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



#if W65C02S_C99
#if W65C02S_GNUC
#define W65C02S_INLINE __attribute__((always_inline)) static inline
#elif W65C02S_MSVC
#define W65C02S_INLINE __forceinline static inline
#else
#define W65C02S_INLINE static inline
#endif
#else
#define W65C02S_INLINE static
#endif

#if W65C02S_C23
#include <stddef.h>
#define W65C02S_UNREACHABLE() unreachable()
#elif W65C02S_GNUC
#define W65C02S_UNREACHABLE() __builtin_unreachable()
#elif W65C02S_MSVC
#define W65C02S_UNREACHABLE() __assume(0)
#else
#define W65C02S_UNREACHABLE()
#endif

#if W65C02S_GNUC
#define W65C02S_LIKELY(x) __builtin_expect((x), 1)
#define W65C02S_UNLIKELY(x) __builtin_expect((x), 0)
#else
#define W65C02S_LIKELY(x) (x)
#define W65C02S_UNLIKELY(x) (x)
#endif

#define W65C02S_CPU_STATE_RUN 0
#define W65C02S_CPU_STATE_RESET 1
#define W65C02S_CPU_STATE_WAIT 2
#define W65C02S_CPU_STATE_STOP 3
#define W65C02S_CPU_STATE_IRQ 4
#define W65C02S_CPU_STATE_NMI 8

#define W65C02S_CPU_STATE_EXTRACT(cpu)              ((cpu)->cpu_state & 3)
#define W65C02S_CPU_STATE_EXTRACT_WITH_IRQ(cpu)     ((cpu)->cpu_state & 15)
#define W65C02S_CPU_STATE_INSERT(cpu, s)                                       \
            ((cpu)->cpu_state = ((cpu)->cpu_state & ~3) | s)
#define W65C02S_CPU_STATE_HAS_FLAG(cpu, f)          ((cpu)->cpu_state & (f))
#define W65C02S_CPU_STATE_SET_FLAG(cpu, f)          ((cpu)->cpu_state |= (f))
#define W65C02S_CPU_STATE_RST_FLAG(cpu, f)          ((cpu)->cpu_state &= ~(f))

#define W65C02S_CPU_STATE_HAS_NMI(cpu)                                         \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_NMI)
#define W65C02S_CPU_STATE_HAS_IRQ(cpu)                                         \
        W65C02S_CPU_STATE_HAS_FLAG(cpu, W65C02S_CPU_STATE_IRQ)
#define W65C02S_CPU_STATE_ASSERT_NMI(cpu)                                      \
        W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_NMI)
#define W65C02S_CPU_STATE_ASSERT_IRQ(cpu)                                      \
        W65C02S_CPU_STATE_SET_FLAG(cpu, W65C02S_CPU_STATE_IRQ)
#define W65C02S_CPU_STATE_CLEAR_NMI(cpu)                                       \
        W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_NMI)
#define W65C02S_CPU_STATE_CLEAR_IRQ(cpu)                                       \
        W65C02S_CPU_STATE_RST_FLAG(cpu, W65C02S_CPU_STATE_IRQ)

#if W65C02S_LINK
#define W65C02S_READ(a) w65c02s_read(a)
#define W65C02S_WRITE(a, v) w65c02s_write(a, v)
#else
#define W65C02S_READ(a) (*cpu->mem_read)(cpu, a)
#define W65C02S_WRITE(a, v) (*cpu->mem_write)(cpu, a, v)
#endif

#if W65C02S_COARSE
#if W65C02S_COARSE_CYCLE_COUNTER
#define W65C02S_CYCLE_TOTAL_INC         
#else
#define W65C02S_CYCLE_TOTAL_INC     ++cpu->total_cycles;
#endif

#define W65C02S_SKIP_REST           return cyc
#define W65C02S_SKIP_TO_NEXT(n)     goto cycle_##n;
#define W65C02S_BEGIN_INSTRUCTION   unsigned cyc = 1;
#define W65C02S_CYCLE_END           ++cyc; W65C02S_CYCLE_TOTAL_INC
#define W65C02S_CYCLE_LABEL_1(n)        
#define W65C02S_CYCLE_LABEL_X(n)    goto cycle_##n; cycle_##n:
#define W65C02S_END_INSTRUCTION     W65C02S_CYCLE_END                          \
                                W65C02S_SKIP_REST;
#else /* !W65C02S_COARSE */

#if W65C02S_COARSE_CYCLE_COUNTER
#define W65C02S_CYCLE_CONDITION     !--cpu->left_cycles
#else
#define W65C02S_CYCLE_CONDITION     ++cpu->total_cycles == cpu->target_cycles
#endif

#define W65C02S_SKIP_REST           return 0
#define W65C02S_SKIP_TO_NEXT(n)     do { ++cpu->cycl; goto cycle_##n; }        \
                                                            while (0)
#define W65C02S_BEGIN_INSTRUCTION   if (W65C02S_LIKELY(!cont))                 \
                                        goto cycle_1;                          \
                                    switch (cpu->cycl) {
#define W65C02S_CYCLE_END           if (W65C02S_UNLIKELY(                      \
                                        W65C02S_CYCLE_CONDITION                \
                                    )) return 1;
#define W65C02S_CYCLE_END_LAST      cpu->cycl = 0; W65C02S_CYCLE_END
                              
#define W65C02S_CYCLE_LABEL_1(n)    cycle_##n: case n:
#define W65C02S_CYCLE_LABEL_X(n)    goto cycle_##n; cycle_##n: case n:
#define W65C02S_END_INSTRUCTION     W65C02S_CYCLE_END_LAST                     \
                                    W65C02S_SKIP_REST;                         \
                                }                                              \
                                W65C02S_UNREACHABLE();                         \
                                W65C02S_SKIP_REST;
#endif

#define W65C02S_CYCLE_1                           W65C02S_CYCLE_LABEL_1(1)
#define W65C02S_CYCLE_2         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(2)
#define W65C02S_CYCLE_3         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(3)
#define W65C02S_CYCLE_4         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(4)
#define W65C02S_CYCLE_5         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(5)
#define W65C02S_CYCLE_6         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(6)
#define W65C02S_CYCLE_7         W65C02S_CYCLE_END W65C02S_CYCLE_LABEL_X(7)

#define W65C02S_CYCLE(n)        W65C02S_CYCLE_##n

/* example instruction:

    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            foo();
        W65C02S_CYCLE(2)
            bar();
    W65C02S_END_INSTRUCTION

becomes

    switch (cpu->cycl) {
        case 1:
            foo();
            if (!--cpu->left_cycles) return 1;
        case 2:
            bar();
            if (!--cpu->left_cycles) {
                cpu->cycl = 0;
                return 1;
            }
    }
    return 0;

*/

/* get flag of P */
#define W65C02S_GET_P(flag) ((cpu->p & (flag)) ? 1 : 0)
/* set flag of P */
#define W65C02S_SET_P(flag, v) \
    (cpu->p = (v) ? (cpu->p | (flag)) : (cpu->p & ~(flag)))
/* set flag of P_adj */
#define W65C02S_SET_P_ADJ(flag, v) \
    (cpu->p_adj = (v) ? (cpu->p_adj | (flag)) : (cpu->p_adj & ~(flag)))

/* P flags */
#define W65C02S_P_N 0x80
#define W65C02S_P_V 0x40
#define W65C02S_P_A1 0x20 /* N/C, always 1 */
#define W65C02S_P_B 0x10  /* always 1, but sometimes pushed as 0 */
#define W65C02S_P_D 0x08
#define W65C02S_P_I 0x04
#define W65C02S_P_Z 0x02
#define W65C02S_P_C 0x01

/* stack offset */
#define W65C02S_STACK_OFFSET 0x0100U
#define W65C02S_STACK_ADDR(x) (W65C02S_STACK_OFFSET | ((x) & 0xFF))

/* vectors for NMI, RESET, IRQ */
#define W65C02S_VEC_NMI 0xFFFAU
#define W65C02S_VEC_RST 0xFFFCU
#define W65C02S_VEC_IRQ 0xFFFEU

/* helpers for getting/setting hi and lo bytes */
#define W65C02S_GET_HI(x) (((x) >> 8) & 0xFF)
#define W65C02S_GET_LO(x) ((x) & 0xFF)
#define W65C02S_SET_HI(x, v) ((x) = ((x) & 0x00FFU) | ((v) << 8))
#define W65C02S_SET_LO(x, v) ((x) = ((x) & 0xFF00U) | (v))
/* take tr[n] and tr[n + 1] as a 16-bit address. n is always even */
#define W65C02S_GET_T16(n) (cpu->tr[n] | (cpu->tr[n + 1] << 8))

/* returns 1 if a+b overflows, 0 if not (a, b are uint8_t) */
#define W65C02S_OVERFLOW8(a, b) (((unsigned)(a) + (unsigned)(b)) >> 8)



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |               ADDRESSING MODE AND OPERATION ENUMERATIONS               | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* all possible values for cpu->mode        (example opcode, ! = only) */
#define W65C02S_MODE_IMPLIED                0       /*  CLD, DEC A */
#define W65C02S_MODE_IMPLIED_X              1       /*  INX */
#define W65C02S_MODE_IMPLIED_Y              2       /*  INY */

#define W65C02S_MODE_IMMEDIATE              3       /*  LDA # */
#define W65C02S_MODE_RELATIVE               4       /*  BRA # */
#define W65C02S_MODE_RELATIVE_BIT           5       /*  BBR0 # */
#define W65C02S_MODE_ZEROPAGE               6       /*  LDA zp */
#define W65C02S_MODE_ZEROPAGE_X             7       /*  LDA zp,x */
#define W65C02S_MODE_ZEROPAGE_Y             8       /*  LDA zp,y */
#define W65C02S_MODE_ZEROPAGE_BIT           9       /*  RMB0 zp */
#define W65C02S_MODE_ABSOLUTE               10       /*  LDA abs */
#define W65C02S_MODE_ABSOLUTE_X             11       /*  LDA abs,x */
#define W65C02S_MODE_ABSOLUTE_Y             12      /*  LDA abs,y */
#define W65C02S_MODE_ZEROPAGE_INDIRECT      13      /*  ORA (zp) */

#define W65C02S_MODE_ZEROPAGE_INDIRECT_X    14      /*  LDA (zp,x) */
#define W65C02S_MODE_ZEROPAGE_INDIRECT_Y    15      /*  LDA (zp),y */
#define W65C02S_MODE_ABSOLUTE_INDIRECT      16      /*! JMP (abs) */
#define W65C02S_MODE_ABSOLUTE_INDIRECT_X    17      /*! JMP (abs,x) */
#define W65C02S_MODE_ABSOLUTE_JUMP          18      /*! JMP abs */

#define W65C02S_MODE_RMW_ZEROPAGE           19      /*  LSR zp */
#define W65C02S_MODE_RMW_ZEROPAGE_X         20      /*  LSR zp,x */
#define W65C02S_MODE_SUBROUTINE             21      /*! JSR abs */
#define W65C02S_MODE_RETURN_SUB             22      /*! RTS */
#define W65C02S_MODE_RMW_ABSOLUTE           23      /*  LSR abs */
#define W65C02S_MODE_RMW_ABSOLUTE_X         24      /*  LSR abs,x */
#define W65C02S_MODE_NOP_5C                 25      /*! NOP ($5C) */
#define W65C02S_MODE_INT_WAIT_STOP          26      /*  WAI, STP */

#define W65C02S_MODE_STACK_PUSH             27      /*  PHA */
#define W65C02S_MODE_STACK_PULL             28      /*  PLA */
#define W65C02S_MODE_STACK_BRK              29      /*! BRK # */
#define W65C02S_MODE_STACK_RTI              30      /*! RTI */

#define W65C02S_MODE_IMPLIED_1C             31      /*! NOP */

/* all possible values for cpu->oper. note that for
   W65C02S_MODE_ZEROPAGE_BIT and W65C02S_MODE_RELATIVE_BIT,
   the value is always 0-7 (bit) + 8 (S/R) */
/* NOP */
#define W65C02S_OPER_NOP                    0

/* read/store instrs */
/* NOP = 0 */
#define W65C02S_OPER_AND                    1
#define W65C02S_OPER_EOR                    2
#define W65C02S_OPER_ORA                    3
#define W65C02S_OPER_ADC                    4
#define W65C02S_OPER_SBC                    5
#define W65C02S_OPER_CMP                    6
#define W65C02S_OPER_CPX                    7
#define W65C02S_OPER_CPY                    8
#define W65C02S_OPER_BIT                    9
#define W65C02S_OPER_LDA                    10
#define W65C02S_OPER_LDX                    11
#define W65C02S_OPER_LDY                    12
#define W65C02S_OPER_STA                    13
#define W65C02S_OPER_STX                    14
#define W65C02S_OPER_STY                    15
#define W65C02S_OPER_STZ                    16

/* RMW instrs */
/* NOP = 0 */
#define W65C02S_OPER_DEC                    1  /* RMW, A, X, Y */
#define W65C02S_OPER_INC                    2  /* RMW, A, X, Y */
#define W65C02S_OPER_ASL                    3  /* RMW, A */
#define W65C02S_OPER_ROL                    4  /* RMW, A */
#define W65C02S_OPER_LSR                    5  /* RMW, A */
#define W65C02S_OPER_ROR                    6  /* RMW, A */
#define W65C02S_OPER_TSB                    7  /* RMW */
#define W65C02S_OPER_TRB                    8  /* RMW */
/* implied instrs */
#define W65C02S_OPER_CLV                    7  /*      impl */
#define W65C02S_OPER_CLC                    8  /*      impl */
#define W65C02S_OPER_SEC                    9  /*      impl */
#define W65C02S_OPER_CLI                    10 /*      impl */
#define W65C02S_OPER_SEI                    11 /*      impl */
#define W65C02S_OPER_CLD                    12 /*      impl */
#define W65C02S_OPER_SED                    13 /*      impl */
#define W65C02S_OPER_TAX                    14 /*      impl */
#define W65C02S_OPER_TXA                    15 /*      impl */
#define W65C02S_OPER_TAY                    16 /*      impl */
#define W65C02S_OPER_TYA                    17 /*      impl */
#define W65C02S_OPER_TSX                    18 /*      impl */
#define W65C02S_OPER_TXS                    19 /*      impl */

/* branch instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_BPL                    0
#define W65C02S_OPER_BMI                    1
#define W65C02S_OPER_BVC                    2
#define W65C02S_OPER_BVS                    3
#define W65C02S_OPER_BCC                    4
#define W65C02S_OPER_BCS                    5
#define W65C02S_OPER_BNE                    6
#define W65C02S_OPER_BEQ                    7
#define W65C02S_OPER_BRA                    8

/* stack instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_PHP                    0
#define W65C02S_OPER_PHA                    1
#define W65C02S_OPER_PHX                    2
#define W65C02S_OPER_PHY                    3
#define W65C02S_OPER_PLP                    0
#define W65C02S_OPER_PLA                    1
#define W65C02S_OPER_PLX                    2
#define W65C02S_OPER_PLY                    3

/* wait/stop instrs. no NOPs use this mode, so 0 is ok */
#define W65C02S_OPER_WAI                    0
#define W65C02S_OPER_STP                    1

/* ! (only instructions for their modes, set them as 0) */
#define W65C02S_OPER_JMP                    0
#define W65C02S_OPER_JSR                    0
#define W65C02S_OPER_RTS                    0
#define W65C02S_OPER_BRK                    0
#define W65C02S_OPER_RTI                    0



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                           INTERNAL OPERATIONS                          | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



W65C02S_INLINE uint8_t w65c02s_mark_nz(struct w65c02s_cpu *cpu, uint8_t q) {
    /* N: bit 7 of result. Z: whether result is 0 */
    W65C02S_SET_P(W65C02S_P_N, q & 0x80);
    W65C02S_SET_P(W65C02S_P_Z, q == 0);
    return q;
}

W65C02S_INLINE uint8_t w65c02s_mark_nzc(struct w65c02s_cpu *cpu,
                                        unsigned q, unsigned c) {
    W65C02S_SET_P(W65C02S_P_C, c);
    return w65c02s_mark_nz(cpu, q & 255);
}

W65C02S_INLINE uint8_t w65c02s_mark_nzc8(struct w65c02s_cpu *cpu, unsigned q) {
    /* q is a 9-bit value with carry at bit 8 */
    return w65c02s_mark_nzc(cpu, q, q >> 8);
}

W65C02S_INLINE uint8_t w65c02s_oper_inc(struct w65c02s_cpu *cpu, uint8_t v) {
    return w65c02s_mark_nz(cpu, v + 1);
}

W65C02S_INLINE uint8_t w65c02s_oper_dec(struct w65c02s_cpu *cpu, uint8_t v) {
    return w65c02s_mark_nz(cpu, v - 1);
}

W65C02S_INLINE uint8_t w65c02s_oper_asl(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    return w65c02s_mark_nzc(cpu, v << 1, v >> 7);
}

W65C02S_INLINE uint8_t w65c02s_oper_lsr(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    return w65c02s_mark_nzc(cpu, v >> 1, v & 1);
}

W65C02S_INLINE uint8_t w65c02s_oper_rol(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the highest bit */
    return w65c02s_mark_nzc(cpu,
        (v << 1) | W65C02S_GET_P(W65C02S_P_C),
        v >> 7);
}

W65C02S_INLINE uint8_t w65c02s_oper_ror(struct w65c02s_cpu *cpu, uint8_t v) {
    /* new carry is the lowest bit */
    return w65c02s_mark_nzc(cpu,
        (v >> 1) | (W65C02S_GET_P(W65C02S_P_C) << 7),
        v & 1);
}

W65C02S_INLINE unsigned w65c02s_oper_adc_v(uint8_t a, uint8_t b, uint8_t c) {
    unsigned c6 = ((a & 0x7F) + (b & 0x7F) + c) >> 7;
    unsigned c7 = (a + b + c) >> 8;
    return c6 ^ c7;
}

static uint8_t w65c02s_oper_adc_d(struct w65c02s_cpu *cpu,
                                  uint8_t a, uint8_t b, unsigned c) {
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
    W65C02S_SET_P_ADJ(W65C02S_P_N, q >> 7);
    W65C02S_SET_P_ADJ(W65C02S_P_Z, q == 0);
    W65C02S_SET_P_ADJ(W65C02S_P_C, fc);
    /* keep W65C02S_P_V as in binary addition */
    W65C02S_SET_P(W65C02S_P_C, fc);
    return q;
}

static uint8_t w65c02s_oper_sbc_d(struct w65c02s_cpu *cpu,
                                  uint8_t a, uint8_t b, unsigned c) {
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
    W65C02S_SET_P_ADJ(W65C02S_P_N, q >> 7);
    W65C02S_SET_P_ADJ(W65C02S_P_Z, q == 0);
    W65C02S_SET_P_ADJ(W65C02S_P_C, fc);
    /* keep W65C02S_P_V as in binary addition */
    W65C02S_SET_P(W65C02S_P_C, fc);
    return q;
}

W65C02S_INLINE uint8_t w65c02s_oper_adc(struct w65c02s_cpu *cpu,
                                        uint8_t a, uint8_t b) {
    uint8_t r;
    unsigned c = W65C02S_GET_P(W65C02S_P_C);
    W65C02S_SET_P(W65C02S_P_V, w65c02s_oper_adc_v(a, b, c));
    r = w65c02s_mark_nzc8(cpu, a + b + c);
    if (!W65C02S_GET_P(W65C02S_P_D)) return r;
    return w65c02s_oper_adc_d(cpu, a, b, c);
}

W65C02S_INLINE uint8_t w65c02s_oper_sbc(struct w65c02s_cpu *cpu,
                                        uint8_t a, uint8_t b) {
    uint8_t r;
    unsigned c = W65C02S_GET_P(W65C02S_P_C);
    b = ~b;
    W65C02S_SET_P(W65C02S_P_V, w65c02s_oper_adc_v(a, b, c));
    r = w65c02s_mark_nzc8(cpu, a + b + c);
    if (!W65C02S_GET_P(W65C02S_P_D)) return r;
    return w65c02s_oper_sbc_d(cpu, a, b, c);
}

W65C02S_INLINE void w65c02s_oper_cmp(struct w65c02s_cpu *cpu,
                                     uint8_t a, uint8_t b) {
    w65c02s_mark_nzc8(cpu, a + (uint8_t)(~b) + 1);
}

static void w65c02s_oper_bit(struct w65c02s_cpu *cpu, uint8_t a, uint8_t b) {
    /* in BIT, N and V are bits 7 and 6 of the memory operand */
    W65C02S_SET_P(W65C02S_P_N, (b >> 7) & 1);
    W65C02S_SET_P(W65C02S_P_V, (b >> 6) & 1);
    W65C02S_SET_P(W65C02S_P_Z, !(a & b));
}

W65C02S_INLINE void w65c02s_oper_bit_imm(struct w65c02s_cpu *cpu,
                                         uint8_t a, uint8_t b) {
    W65C02S_SET_P(W65C02S_P_Z, !(a & b));
}

static uint8_t w65c02s_oper_tsb(struct w65c02s_cpu *cpu,
                                uint8_t a, uint8_t b, bool set) {
    W65C02S_SET_P(W65C02S_P_Z, !(a & b));
    return set ? b | a : b & ~a;
}

static uint8_t w65c02s_oper_rmw(struct w65c02s_cpu *cpu,
                                unsigned op, uint8_t v) {
    switch (op) {
        case W65C02S_OPER_ASL: return w65c02s_oper_asl(cpu, v);
        case W65C02S_OPER_DEC: return w65c02s_oper_dec(cpu, v);
        case W65C02S_OPER_INC: return w65c02s_oper_inc(cpu, v);
        case W65C02S_OPER_LSR: return w65c02s_oper_lsr(cpu, v);
        case W65C02S_OPER_ROL: return w65c02s_oper_rol(cpu, v);
        case W65C02S_OPER_ROR: return w65c02s_oper_ror(cpu, v);
    }
    W65C02S_UNREACHABLE();
    return v;
}

W65C02S_INLINE uint8_t w65c02s_oper_alu(struct w65c02s_cpu *cpu,
                                        unsigned op, uint8_t a, uint8_t b) {
    switch (op) {
        case W65C02S_OPER_AND: return w65c02s_mark_nz(cpu, a & b);
        case W65C02S_OPER_EOR: return w65c02s_mark_nz(cpu, a ^ b);
        case W65C02S_OPER_ORA: return w65c02s_mark_nz(cpu, a | b);
        case W65C02S_OPER_ADC: return w65c02s_oper_adc(cpu, a, b);
        case W65C02S_OPER_SBC: return w65c02s_oper_sbc(cpu, a, b);
    }
    W65C02S_UNREACHABLE();
    return w65c02s_mark_nz(cpu, b);
}

static bool w65c02s_oper_branch(unsigned op, uint8_t p) {
    /* whether to take the branch? */
    switch (op) {
        case W65C02S_OPER_BPL: return !(p & W65C02S_P_N);
        case W65C02S_OPER_BMI: return  (p & W65C02S_P_N);
        case W65C02S_OPER_BVC: return !(p & W65C02S_P_V);
        case W65C02S_OPER_BVS: return  (p & W65C02S_P_V);
        case W65C02S_OPER_BCC: return !(p & W65C02S_P_C);
        case W65C02S_OPER_BCS: return  (p & W65C02S_P_C);
        case W65C02S_OPER_BNE: return !(p & W65C02S_P_Z);
        case W65C02S_OPER_BEQ: return  (p & W65C02S_P_Z);
        case W65C02S_OPER_BRA: return 1;
    }
    W65C02S_UNREACHABLE();
    return 0;
}

static uint8_t w65c02s_oper_bitset(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v | mask : v & ~mask;
}

static bool w65c02s_oper_bitbranch(unsigned oper, uint8_t v) {
    uint8_t mask = 1 << (oper & 7);
    return (oper & 8) ? v & mask : !(v & mask);
}

W65C02S_INLINE void w65c02s_stack_push(struct w65c02s_cpu *cpu, uint8_t v) {
    uint16_t s = W65C02S_STACK_ADDR(cpu->s--);
    W65C02S_WRITE(s, v);
}

W65C02S_INLINE uint8_t w65c02s_stack_pull(struct w65c02s_cpu *cpu) {
    uint16_t s = W65C02S_STACK_ADDR(++cpu->s);
    return W65C02S_READ(s);
}

W65C02S_INLINE void w65c02s_irq_update_mask(struct w65c02s_cpu *cpu) {
    cpu->int_mask = W65C02S_GET_P(W65C02S_P_I) ? ~W65C02S_CPU_STATE_IRQ : ~0;
}

W65C02S_INLINE void w65c02s_irq_latch(struct w65c02s_cpu *cpu) {
    cpu->cpu_state |= cpu->int_trig & cpu->int_mask;
}

W65C02S_INLINE void w65c02s_irq_latch_slow(struct w65c02s_cpu *cpu) {
    cpu->cpu_state &= ~W65C02S_CPU_STATE_IRQ;
    w65c02s_irq_latch(cpu);
}

static void w65c02s_irq_reset(struct w65c02s_cpu *cpu) {
    if (cpu->in_rst) {
        cpu->in_rst = 0;
    } else if (cpu->in_nmi) {
        cpu->in_nmi = 0;
    } else if (cpu->in_irq) {
        cpu->in_irq = 0;
    }
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                            ADDRESSING MODES                            | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/* 0 = no store penalty, 1 = store penalty */
W65C02S_INLINE int w65c02s_oper_is_store(unsigned oper) {
    switch (oper) {
        case W65C02S_OPER_STA:
        case W65C02S_OPER_STX:
        case W65C02S_OPER_STY:
        case W65C02S_OPER_STZ:
            return 1;
    }
    return 0;
}

W65C02S_INLINE unsigned w65c02s_fast_rmw_absx(unsigned oper) {
    switch (oper) {
        case W65C02S_OPER_INC:
        case W65C02S_OPER_DEC:
            return 0;
    }
    return 1;
}

static void w65c02s_compute_branch(struct w65c02s_cpu *cpu) {
    /* offset in tr[0] */
    /* old PC in tr[0], tr[1] */
    /* new PC in tr[2], tr[3] */
    uint8_t offset = cpu->tr[0];
    uint16_t pc_old = cpu->pc;
    uint16_t pc_new = pc_old + offset - (offset & 0x80 ? 0x100 : 0);
    cpu->tr[0] = W65C02S_GET_LO(pc_old);
    cpu->tr[1] = W65C02S_GET_HI(pc_old);
    cpu->tr[2] = W65C02S_GET_LO(pc_new);
    cpu->tr[3] = W65C02S_GET_HI(pc_new);
}

/* false = no penalty, true = decimal mode penalty */
static bool w65c02s_oper_imm(struct w65c02s_cpu *cpu, uint8_t v) {
    unsigned oper = cpu->oper;
    switch (oper) {
        case W65C02S_OPER_NOP:
            break;

        case W65C02S_OPER_AND:
        case W65C02S_OPER_EOR:
        case W65C02S_OPER_ORA:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, v);
            return 0;

        case W65C02S_OPER_ADC:
        case W65C02S_OPER_SBC:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, v);
            return W65C02S_GET_P(W65C02S_P_D); /* decimal mode penalty */

        case W65C02S_OPER_CMP: w65c02s_oper_cmp(cpu, cpu->a, v);     break;
        case W65C02S_OPER_CPX: w65c02s_oper_cmp(cpu, cpu->x, v);     break;
        case W65C02S_OPER_CPY: w65c02s_oper_cmp(cpu, cpu->y, v);     break;
        case W65C02S_OPER_BIT: w65c02s_oper_bit_imm(cpu, cpu->a, v); break;
        
        case W65C02S_OPER_LDA: cpu->a = w65c02s_mark_nz(cpu, v);     break;
        case W65C02S_OPER_LDX: cpu->x = w65c02s_mark_nz(cpu, v);     break;
        case W65C02S_OPER_LDY: cpu->y = w65c02s_mark_nz(cpu, v);     break;
        default: W65C02S_UNREACHABLE();
    }
    return 0;
}

/* false = no penalty, true = decimal mode penalty */
static bool w65c02s_oper_addr(struct w65c02s_cpu *cpu, uint16_t a) {
    unsigned oper = cpu->oper;
    switch (oper) {
        case W65C02S_OPER_NOP:
            W65C02S_READ(a);
            break;

        case W65C02S_OPER_AND:
        case W65C02S_OPER_EOR:
        case W65C02S_OPER_ORA:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, W65C02S_READ(a));
            return 0;

        case W65C02S_OPER_ADC:
        case W65C02S_OPER_SBC:
            cpu->a = w65c02s_oper_alu(cpu, oper, cpu->a, W65C02S_READ(a));
            return W65C02S_GET_P(W65C02S_P_D); /* decimal mode penalty */

        case W65C02S_OPER_CMP:
            w65c02s_oper_cmp(cpu, cpu->a, W65C02S_READ(a));
            break;
        case W65C02S_OPER_CPX:
            w65c02s_oper_cmp(cpu, cpu->x, W65C02S_READ(a));
            break;
        case W65C02S_OPER_CPY:
            w65c02s_oper_cmp(cpu, cpu->y, W65C02S_READ(a));
            break;
        case W65C02S_OPER_BIT:
            w65c02s_oper_bit(cpu, cpu->a, W65C02S_READ(a));
            break;

        case W65C02S_OPER_LDA:
            cpu->a = w65c02s_mark_nz(cpu, W65C02S_READ(a));
            break;
        case W65C02S_OPER_LDX:
            cpu->x = w65c02s_mark_nz(cpu, W65C02S_READ(a));
            break;
        case W65C02S_OPER_LDY:
            cpu->y = w65c02s_mark_nz(cpu, W65C02S_READ(a));
            break;

        case W65C02S_OPER_STA: W65C02S_WRITE(a, cpu->a); break;
        case W65C02S_OPER_STX: W65C02S_WRITE(a, cpu->x); break;
        case W65C02S_OPER_STY: W65C02S_WRITE(a, cpu->y); break;
        case W65C02S_OPER_STZ: W65C02S_WRITE(a, 0);      break;
        default: W65C02S_UNREACHABLE();
    }
    return 0;
}

#if !W65C02S_COARSE
/* current CPU, whether we are continuing an instruction */
#define W65C02S_PARAMS_MODE struct w65c02s_cpu *cpu, unsigned cont
#define W65C02S_GO_MODE(mode) return w65c02s_mode_##mode(cpu, cont)
#else
#define W65C02S_PARAMS_MODE struct w65c02s_cpu *cpu
#define W65C02S_GO_MODE(mode) return w65c02s_mode_##mode(cpu)
#endif

#define W65C02S_ADC_D_SPURIOUS(ea)                                             \
    if (!cpu->take) W65C02S_SKIP_REST; /* no penalty cycle => skip last read */\
    cpu->p = cpu->p_adj;                                                       \
    W65C02S_READ(ea);
    /* no need to update mask - p_adj never changes I */

static unsigned w65c02s_mode_implied(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (see execute.c) */
        W65C02S_CYCLE(1)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case W65C02S_OPER_NOP:
                    break;

                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    cpu->a = w65c02s_oper_rmw(cpu, oper, cpu->a);
                    break;

#define W65C02S_TRANSFER(src, dst) cpu->dst = w65c02s_mark_nz(cpu, cpu->src)
                case W65C02S_OPER_CLV: W65C02S_SET_P(W65C02S_P_V, 0);   break;
                case W65C02S_OPER_CLC: W65C02S_SET_P(W65C02S_P_C, 0);   break;
                case W65C02S_OPER_SEC: W65C02S_SET_P(W65C02S_P_C, 1);   break;
                case W65C02S_OPER_CLD: W65C02S_SET_P(W65C02S_P_D, 0);   break;
                case W65C02S_OPER_SED: W65C02S_SET_P(W65C02S_P_D, 1);   break;
                case W65C02S_OPER_CLI:
                    W65C02S_SET_P(W65C02S_P_I, 0);
                    w65c02s_irq_update_mask(cpu);
                    break;
                case W65C02S_OPER_SEI:
                    W65C02S_SET_P(W65C02S_P_I, 1);
                    w65c02s_irq_update_mask(cpu);
                    break;
                case W65C02S_OPER_TAX: W65C02S_TRANSFER(a, x);  break;
                case W65C02S_OPER_TXA: W65C02S_TRANSFER(x, a);  break;
                case W65C02S_OPER_TAY: W65C02S_TRANSFER(a, y);  break;
                case W65C02S_OPER_TYA: W65C02S_TRANSFER(y, a);  break;
                case W65C02S_OPER_TSX: W65C02S_TRANSFER(s, x);  break;
                case W65C02S_OPER_TXS: cpu->s = cpu->x; break;
                default: W65C02S_UNREACHABLE();
            }
            W65C02S_READ(cpu->pc);
        }
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_implied_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (see execute.c) */
        W65C02S_CYCLE(1)
            cpu->x = w65c02s_oper_rmw(cpu, cpu->oper, cpu->x);
            W65C02S_READ(cpu->pc);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_implied_y(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (see execute.c) */
        W65C02S_CYCLE(1)
            cpu->y = w65c02s_oper_rmw(cpu, cpu->oper, cpu->y);
            W65C02S_READ(cpu->pc);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_immediate(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (see execute.c) */
        W65C02S_CYCLE(1)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_imm(cpu, W65C02S_READ(cpu->pc++));
        W65C02S_CYCLE(2)
            W65C02S_ADC_D_SPURIOUS(cpu->pc);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, cpu->tr[0]);
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(3)
            W65C02S_ADC_D_SPURIOUS(cpu->tr[0]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            cpu->tr[0] += cpu->x;
            W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, cpu->tr[0]);
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(cpu->tr[0]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_y(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            cpu->tr[0] += cpu->y;
            W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, cpu->tr[0]);
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(cpu->tr[0]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_absolute(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(4)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_absolute_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            /* page wrap cycle? */
            cpu->tr[4] = W65C02S_OVERFLOW8(cpu->tr[0], cpu->x);
            cpu->tr[0] += cpu->x;
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
            cpu->take = cpu->tr[4] || w65c02s_oper_is_store(cpu->oper);
            if (!cpu->take) w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) W65C02S_SKIP_TO_NEXT(4);
            W65C02S_READ(!cpu->tr[4] ? W65C02S_GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_absolute_y(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            /* page wrap cycle? */
            cpu->tr[4] = W65C02S_OVERFLOW8(cpu->tr[0], cpu->y);
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
            cpu->take = cpu->tr[4] || w65c02s_oper_is_store(cpu->oper);
            if (!cpu->take) w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) W65C02S_SKIP_TO_NEXT(4);
            W65C02S_READ(!cpu->tr[4] ? W65C02S_GET_T16(0) : cpu->pc - 1);
            cpu->tr[1] += cpu->tr[4];
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_indirect(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[2] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[0] = W65C02S_READ(cpu->tr[2]++);
        W65C02S_CYCLE(3)
            cpu->tr[1] = W65C02S_READ(cpu->tr[2]);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(5)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_indirect_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[2] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            cpu->tr[2] += cpu->x;
            W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            cpu->tr[0] = W65C02S_READ(cpu->tr[2]++);
        W65C02S_CYCLE(4)
            cpu->tr[1] = W65C02S_READ(cpu->tr[2]++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(6)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_indirect_y(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[2] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[0] = W65C02S_READ(cpu->tr[2]++);
        W65C02S_CYCLE(3)
            /* page wrap cycle? */
            cpu->tr[4] = W65C02S_OVERFLOW8(cpu->tr[0], cpu->y);
            cpu->tr[0] += cpu->y;
            cpu->tr[1] = W65C02S_READ(cpu->tr[2]);
            cpu->take = cpu->tr[4] || w65c02s_oper_is_store(cpu->oper);
            if (!cpu->take) w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            /* if we did not cross the page boundary, skip this cycle */
            if (!cpu->take) W65C02S_SKIP_TO_NEXT(5);
            W65C02S_READ(cpu->tr[2]);
            cpu->tr[1] += cpu->tr[4];
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            /* penalty cycle? */
            cpu->take = w65c02s_oper_addr(cpu, W65C02S_GET_T16(0));
            if (cpu->take) w65c02s_irq_latch_slow(cpu);
        W65C02S_CYCLE(6)
            W65C02S_ADC_D_SPURIOUS(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_jump_absolute(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc);
            cpu->pc = W65C02S_GET_T16(0);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_jump_indirect(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(3)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(4)
            cpu->tr[2] = W65C02S_READ(W65C02S_GET_T16(0));
            /* increment with overflow */
            if (!++cpu->tr[0]) ++cpu->tr[1];
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->tr[3] = W65C02S_READ(W65C02S_GET_T16(0));
            cpu->pc = W65C02S_GET_T16(2);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_jump_indirect_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(3)
            /* add X to tr0,1 */
            cpu->tr[1] += W65C02S_OVERFLOW8(cpu->tr[0], cpu->x);
            cpu->tr[0] += cpu->x;
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(4)
            cpu->tr[2] = W65C02S_READ(W65C02S_GET_T16(0));
            /* increment with overflow */
            if (!++cpu->tr[0]) ++cpu->tr[1];
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->tr[3] = W65C02S_READ(W65C02S_GET_T16(0));
            cpu->pc = W65C02S_GET_T16(2);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_zeropage_bit(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->tr[0]);
        W65C02S_CYCLE(3)
            cpu->tr[1] = w65c02s_oper_bitset(cpu->oper, cpu->tr[1]);
            W65C02S_READ(cpu->tr[0]);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            W65C02S_WRITE(cpu->tr[0], cpu->tr[1]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_relative(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        /* 0: w65c02s_irq_latch(cpu); (see execute.c) */
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
            w65c02s_compute_branch(cpu);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
            /* skip the rest of the instruction if branch is not taken */
            if (!w65c02s_oper_branch(cpu->oper, cpu->p))
                W65C02S_SKIP_REST;
            W65C02S_READ(W65C02S_GET_T16(0));
            cpu->pc = W65C02S_GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) W65C02S_SKIP_REST;
            W65C02S_READ(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_relative_bit(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[3] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->tr[3]);
        W65C02S_CYCLE(3)
            cpu->tr[4] = W65C02S_READ(cpu->tr[3]);
        W65C02S_CYCLE(4)
            cpu->take = w65c02s_oper_bitbranch(cpu->oper, cpu->tr[4]);
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
            w65c02s_compute_branch(cpu);
        W65C02S_CYCLE(5)
            /* skip the rest of the instruction if branch is not taken */
            if (!cpu->take) W65C02S_SKIP_REST;
            W65C02S_READ(W65C02S_GET_T16(0));
            cpu->pc = W65C02S_GET_T16(2);
            if (cpu->tr[1] != cpu->tr[3]) w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(6)
            /* skip the rest of the instruction if no page boundary crossed */
            if (cpu->tr[1] == cpu->tr[3]) W65C02S_SKIP_REST;
            W65C02S_READ(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_rmw_zeropage(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->tr[0]);
        W65C02S_CYCLE(3)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    cpu->tr[1] = w65c02s_oper_rmw(cpu, oper, cpu->tr[1]);
                    break;
                case W65C02S_OPER_TSB:
                    cpu->tr[1] = w65c02s_oper_tsb(cpu, cpu->a, cpu->tr[1], 1);
                    break;
                case W65C02S_OPER_TRB:
                    cpu->tr[1] = w65c02s_oper_tsb(cpu, cpu->a, cpu->tr[1], 0);
                    break;
                default: W65C02S_UNREACHABLE();
            }
        }
            W65C02S_READ(cpu->tr[0]);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(4)
            W65C02S_WRITE(cpu->tr[0], cpu->tr[1]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_rmw_zeropage_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            cpu->tr[0] += cpu->x;
            W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            cpu->tr[1] = W65C02S_READ(cpu->tr[0]);
        W65C02S_CYCLE(4)
            W65C02S_READ(cpu->tr[0]);
            cpu->tr[1] = w65c02s_oper_rmw(cpu, cpu->oper, cpu->tr[1]);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_WRITE(cpu->tr[0], cpu->tr[1]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_rmw_absolute(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            cpu->tr[2] = W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(4)
        {
            unsigned oper = cpu->oper;
            switch (oper) {
                case W65C02S_OPER_DEC:
                case W65C02S_OPER_INC:
                case W65C02S_OPER_ASL:
                case W65C02S_OPER_ROL:
                case W65C02S_OPER_LSR:
                case W65C02S_OPER_ROR:
                    cpu->tr[2] = w65c02s_oper_rmw(cpu, oper, cpu->tr[2]);
                    break;
                case W65C02S_OPER_TSB:
                    cpu->tr[2] = w65c02s_oper_tsb(cpu, cpu->a, cpu->tr[2], 1);
                    break;
                case W65C02S_OPER_TRB:
                    cpu->tr[2] = w65c02s_oper_tsb(cpu, cpu->a, cpu->tr[2], 0);
                    break;
                default: W65C02S_UNREACHABLE();
            }
        }
            W65C02S_READ(W65C02S_GET_T16(0));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_WRITE(W65C02S_GET_T16(0), cpu->tr[2]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_rmw_absolute_x(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
        {
            unsigned overflow;
            overflow = W65C02S_OVERFLOW8(cpu->tr[0], cpu->x);
            cpu->tr[0] += cpu->x;
            if (!overflow && w65c02s_fast_rmw_absx(cpu->oper))
                W65C02S_SKIP_TO_NEXT(4);
            cpu->tr[1] += overflow;
        }
            W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(4)
            cpu->tr[2] = W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(5)
            W65C02S_READ(W65C02S_GET_T16(0));
            cpu->tr[2] = w65c02s_oper_rmw(cpu, cpu->oper,
                                           cpu->tr[2]);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(6)
            W65C02S_WRITE(W65C02S_GET_T16(0), cpu->tr[2]);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_stack_push(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(2)
        {
            uint8_t tmp;
            switch (cpu->oper) {
                case W65C02S_OPER_PHP:
                    tmp = cpu->p | W65C02S_P_A1 | W65C02S_P_B;
                    break;
                case W65C02S_OPER_PHA: tmp = cpu->a; break;
                case W65C02S_OPER_PHX: tmp = cpu->x; break;
                case W65C02S_OPER_PHY: tmp = cpu->y; break;
                default: W65C02S_UNREACHABLE();
            }
            w65c02s_stack_push(cpu, tmp);
        }
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_stack_pull(W65C02S_PARAMS_MODE) {
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(3)
        {
            uint8_t tmp = w65c02s_stack_pull(cpu);
            switch (cpu->oper) {
                case W65C02S_OPER_PHP:
                    cpu->p = tmp | W65C02S_P_A1 | W65C02S_P_B;
                    w65c02s_irq_update_mask(cpu);
                    break;
                case W65C02S_OPER_PHA:
                    cpu->a = w65c02s_mark_nz(cpu, tmp);
                    break;
                case W65C02S_OPER_PHX:
                    cpu->x = w65c02s_mark_nz(cpu, tmp);
                    break;
                case W65C02S_OPER_PHY:
                    cpu->y = w65c02s_mark_nz(cpu, tmp);
                    break;
                default: W65C02S_UNREACHABLE();
            }
        }
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_subroutine(W65C02S_PARAMS_MODE) {
    /* op = JSR */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            w65c02s_stack_push(cpu, W65C02S_GET_HI(cpu->pc));
        W65C02S_CYCLE(4)
            w65c02s_stack_push(cpu, W65C02S_GET_LO(cpu->pc));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            cpu->tr[1] = W65C02S_READ(cpu->pc);
            cpu->pc = W65C02S_GET_T16(0);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_return_sub(W65C02S_PARAMS_MODE) {
    /* op = RTS */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            W65C02S_SET_LO(cpu->pc, w65c02s_stack_pull(cpu));
        W65C02S_CYCLE(4)
            W65C02S_SET_HI(cpu->pc, w65c02s_stack_pull(cpu));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_READ(cpu->pc++);
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_stack_rti(W65C02S_PARAMS_MODE) {
    /* op = RTI */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(2)
            W65C02S_READ(W65C02S_STACK_ADDR(cpu->s));
        W65C02S_CYCLE(3)
            cpu->p = w65c02s_stack_pull(cpu) | W65C02S_P_A1 | W65C02S_P_B;
            w65c02s_irq_update_mask(cpu);
        W65C02S_CYCLE(4)
            W65C02S_SET_LO(cpu->pc, w65c02s_stack_pull(cpu));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(5)
            W65C02S_SET_HI(cpu->pc, w65c02s_stack_pull(cpu));
    W65C02S_END_INSTRUCTION
}

static int w65c02s_mode_stack_brk(W65C02S_PARAMS_MODE) {
    /* op = BRK (possibly via NMI, IRQ or RESET) */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
        {
            uint8_t tmp = W65C02S_READ(cpu->pc);
            /* is this instruction a true BRK or a hardware interrupt? */
            cpu->take = !(cpu->in_nmi || cpu->in_irq || cpu->in_rst);
            if (cpu->take) {
                ++cpu->pc;
#if W65C02S_HOOK_BRK
                if (cpu->hook_brk && (*cpu->hook_brk)(tmp)) W65C02S_SKIP_REST;
#else
                (void)tmp;
#endif
            }
        }
        W65C02S_CYCLE(2)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else             w65c02s_stack_push(cpu, W65C02S_GET_HI(cpu->pc));
        W65C02S_CYCLE(3)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else             w65c02s_stack_push(cpu, W65C02S_GET_LO(cpu->pc));
        W65C02S_CYCLE(4)
            if (cpu->in_rst) W65C02S_READ(W65C02S_STACK_ADDR(cpu->s--));
            else { /* B flag: 0 for NMI/IRQ, 1 for BRK */
                uint8_t p = cpu->p | W65C02S_P_A1;
                if (cpu->take)
                    p |= W65C02S_P_B;
                else
                    p &= ~W65C02S_P_B;
                w65c02s_stack_push(cpu, p);
            }
            W65C02S_SET_P(W65C02S_P_I, 1);
            W65C02S_SET_P(W65C02S_P_D, 0);
            w65c02s_irq_update_mask(cpu);
        W65C02S_CYCLE(5)
            /* NMI replaces IRQ if one is triggered before this cycle */
            if ((cpu->int_trig & W65C02S_CPU_STATE_NMI) && cpu->in_irq) {
                W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
                cpu->int_trig &= ~W65C02S_CPU_STATE_NMI;
                cpu->in_irq = 0;
                cpu->in_nmi = 1;
            }
            { /* select vector */
                uint16_t vec;
                if (cpu->in_rst)        vec = W65C02S_VEC_RST;
                else if (cpu->in_nmi)   vec = W65C02S_VEC_NMI;
                else                    vec = W65C02S_VEC_IRQ;
                cpu->tr[0] = W65C02S_GET_LO(vec);
                cpu->tr[1] = W65C02S_GET_HI(vec);
            }
            W65C02S_SET_LO(cpu->pc, W65C02S_READ(W65C02S_GET_T16(0)));
            w65c02s_irq_reset(cpu);
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(6)
            W65C02S_SET_HI(cpu->pc, W65C02S_READ(W65C02S_GET_T16(0) + 1));
        W65C02S_CYCLE(7)
            /* end instantly! */
            /* HW interrupts do not increment the instruction counter */
            if (!cpu->take) --cpu->total_instructions;
            W65C02S_SKIP_REST;
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_nop_5c(W65C02S_PARAMS_MODE) {
    /* op = NOP $5C */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            cpu->tr[0] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(2)
            cpu->tr[1] = W65C02S_READ(cpu->pc++);
        W65C02S_CYCLE(3)
            cpu->tr[1] = -1;
            W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(4)
            cpu->tr[0] = -1;
            W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(5)
            W65C02S_READ(W65C02S_GET_T16(0));
        W65C02S_CYCLE(6)
            W65C02S_READ(W65C02S_GET_T16(0));
            w65c02s_irq_latch(cpu);
        W65C02S_CYCLE(7)
            W65C02S_READ(W65C02S_GET_T16(0));
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_int_wait_stop(W65C02S_PARAMS_MODE) {
    /* op = WAI/STP */
    W65C02S_BEGIN_INSTRUCTION
        W65C02S_CYCLE(1)
            W65C02S_READ(cpu->pc);
            /* STP (1) or WAI (0) */
            cpu->take = cpu->oper == W65C02S_OPER_STP;
#if W65C02S_HOOK_STP
            if (cpu->take && cpu->hook_stp && (*cpu->hook_stp)())
                W65C02S_SKIP_REST;
#endif
        W65C02S_CYCLE(2)
            W65C02S_READ(cpu->pc);
        W65C02S_CYCLE(3)
            W65C02S_READ(cpu->pc);
            if (cpu->take) {
                W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_STOP);
            } else if (!cpu->take && W65C02S_CPU_STATE_EXTRACT_WITH_IRQ(cpu)
                                        == W65C02S_CPU_STATE_RUN) {
                W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_WAIT);
            }
#if !W65C02S_COARSE
            cpu->cycl = 0;
#endif
    W65C02S_END_INSTRUCTION
}

static unsigned w65c02s_mode_implied_1c(W65C02S_PARAMS_MODE) {
    return 0; /* return immediately */
}

static void w65c02s_prerun_mode(struct w65c02s_cpu *cpu) {
    switch (cpu->mode) {
    case W65C02S_MODE_IMPLIED:
    case W65C02S_MODE_IMPLIED_X:
    case W65C02S_MODE_IMPLIED_Y:
    case W65C02S_MODE_IMMEDIATE:
    case W65C02S_MODE_RELATIVE:
        w65c02s_irq_latch(cpu);
    }
}

/* COARSE=0: true if there is still more to run, false if not */
/* COARSE=1: number of cycles */
#if W65C02S_COARSE
static unsigned w65c02s_run_mode(struct w65c02s_cpu *cpu) {
#else
static unsigned w65c02s_run_mode(struct w65c02s_cpu *cpu, unsigned cont) {
#endif
    switch (cpu->mode) {
    case W65C02S_MODE_RMW_ZEROPAGE:        W65C02S_GO_MODE(rmw_zeropage);
    case W65C02S_MODE_RMW_ZEROPAGE_X:      W65C02S_GO_MODE(rmw_zeropage_x);
    case W65C02S_MODE_RMW_ABSOLUTE:        W65C02S_GO_MODE(rmw_absolute);
    case W65C02S_MODE_RMW_ABSOLUTE_X:      W65C02S_GO_MODE(rmw_absolute_x);
    case W65C02S_MODE_STACK_PUSH:          W65C02S_GO_MODE(stack_push);
    case W65C02S_MODE_STACK_PULL:          W65C02S_GO_MODE(stack_pull);
    case W65C02S_MODE_IMPLIED_1C:          W65C02S_GO_MODE(implied_1c);
    case W65C02S_MODE_IMPLIED_X:           W65C02S_GO_MODE(implied_x);
    case W65C02S_MODE_IMPLIED_Y:           W65C02S_GO_MODE(implied_y);
    case W65C02S_MODE_IMPLIED:             W65C02S_GO_MODE(implied);
    case W65C02S_MODE_IMMEDIATE:           W65C02S_GO_MODE(immediate);
    case W65C02S_MODE_ZEROPAGE:            W65C02S_GO_MODE(zeropage);
    case W65C02S_MODE_ABSOLUTE:            W65C02S_GO_MODE(absolute);
    case W65C02S_MODE_ZEROPAGE_X:          W65C02S_GO_MODE(zeropage_x);
    case W65C02S_MODE_ZEROPAGE_Y:          W65C02S_GO_MODE(zeropage_y);
    case W65C02S_MODE_ABSOLUTE_X:          W65C02S_GO_MODE(absolute_x);
    case W65C02S_MODE_ABSOLUTE_Y:          W65C02S_GO_MODE(absolute_y);
    case W65C02S_MODE_ZEROPAGE_INDIRECT:   W65C02S_GO_MODE(zeropage_indirect);
    case W65C02S_MODE_ZEROPAGE_INDIRECT_X: W65C02S_GO_MODE(zeropage_indirect_x);
    case W65C02S_MODE_ZEROPAGE_INDIRECT_Y: W65C02S_GO_MODE(zeropage_indirect_y);
    case W65C02S_MODE_ABSOLUTE_JUMP:       W65C02S_GO_MODE(jump_absolute);
    case W65C02S_MODE_ABSOLUTE_INDIRECT:   W65C02S_GO_MODE(jump_indirect);
    case W65C02S_MODE_ABSOLUTE_INDIRECT_X: W65C02S_GO_MODE(jump_indirect_x);
    case W65C02S_MODE_SUBROUTINE:          W65C02S_GO_MODE(subroutine);
    case W65C02S_MODE_RETURN_SUB:          W65C02S_GO_MODE(return_sub);
    case W65C02S_MODE_STACK_BRK:           W65C02S_GO_MODE(stack_brk);
    case W65C02S_MODE_STACK_RTI:           W65C02S_GO_MODE(stack_rti);
    case W65C02S_MODE_RELATIVE:            W65C02S_GO_MODE(relative);
    case W65C02S_MODE_RELATIVE_BIT:        W65C02S_GO_MODE(relative_bit);
    case W65C02S_MODE_ZEROPAGE_BIT:        W65C02S_GO_MODE(zeropage_bit);
    case W65C02S_MODE_INT_WAIT_STOP:       W65C02S_GO_MODE(int_wait_stop);
    case W65C02S_MODE_NOP_5C:              W65C02S_GO_MODE(nop_5c);
    }
    W65C02S_UNREACHABLE();
    return 0;
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                         INSTRUCTION  DECODING                          | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



/*             0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  
      00:0F   brk zix imm im1 wzp zpg wzp zpb phv imm imp im1 wab abs wab rlb 
      10:1F   rel ziy zpi im1 wzp zpx wzx zpb imp aby imp im1 wab abx wax rlb 
      20:2F   sbj zix imm im1 zpg zpg wzp zpb plv imm imp im1 abs abs wab rlb 
      30:3F   rel ziy zpi im1 zpx zpx wzx zpb imp aby imp im1 abx abx wax rlb 
      40:4F   rti zix imm im1 zpg zpg wzp zpb phv imm imp im1 abj abs wab rlb 
      50:5F   rel ziy zpi im1 zpx zpx wzx zpb imp aby phv im1 x5c abx wax rlb 
      60:6F   sbr zix imm im1 zpg zpg wzp zpb plv imm imp im1 ind abs wab rlb 
      70:7F   rel ziy zpi im1 zpx zpx wzx zpb imp aby plv im1 idx abx wax rlb 
      80:8F   rel zix imm im1 zpg zpg zpg zpb imy imm imp im1 abs abs abs rlb 
      90:9F   rel ziy zpi im1 zpx zpx zpy zpb imp aby imp im1 abs abx abx rlb 
      A0:AF   imm zix imm im1 zpg zpg zpg zpb imp imm imp im1 abs abs abs rlb 
      B0:BF   rel ziy zpi im1 zpx zpx zpy zpb imp aby imp im1 abx abx aby rlb 
      C0:CF   imm zix imm im1 zpg zpg wzp zpb imy imm imx wai abs abs wab rlb 
      D0:DF   rel ziy zpi im1 zpx zpx wzx zpb imp aby phv wai abs abx wax rlb 
      E0:EF   imm zix imm im1 zpg zpg wzp zpb imx imm imp im1 abs abs wab rlb 
      F0:FF   rel ziy zpi im1 zpx zpx wzx zpb imp aby plv im1 abs abx wax rlb 
*/

#if W65C02S_C11
_Alignas(256)
#endif
static const unsigned char w65c02s_modes[256] = {
    W65C02S_MODE_STACK_BRK,           W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_STACK_PUSH,          W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_SUBROUTINE,          W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_STACK_PULL,          W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE_X,          W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_STACK_RTI,           W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_STACK_PUSH,          W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE_JUMP,       W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_STACK_PUSH,          W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_NOP_5C,              W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RETURN_SUB,          W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_STACK_PULL,          W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE_INDIRECT,   W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_STACK_PULL,          W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE_INDIRECT_X, W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED_Y,           W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_ZEROPAGE_Y,          W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_ABSOLUTE_X,          W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_ZEROPAGE_Y,          W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE_X,          W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_ABSOLUTE_Y,          W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED_Y,           W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED_X,           W65C02S_MODE_INT_WAIT_STOP,       
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_STACK_PUSH,          W65C02S_MODE_INT_WAIT_STOP,       
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_ZEROPAGE_INDIRECT_X, 
    W65C02S_MODE_IMMEDIATE,           W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE,            W65C02S_MODE_ZEROPAGE,            
    W65C02S_MODE_RMW_ZEROPAGE,        W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED_X,           W65C02S_MODE_IMMEDIATE,           
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE,            
    W65C02S_MODE_RMW_ABSOLUTE,        W65C02S_MODE_RELATIVE_BIT,        
    W65C02S_MODE_RELATIVE,            W65C02S_MODE_ZEROPAGE_INDIRECT_Y, 
    W65C02S_MODE_ZEROPAGE_INDIRECT,   W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ZEROPAGE_X,          W65C02S_MODE_ZEROPAGE_X,          
    W65C02S_MODE_RMW_ZEROPAGE_X,      W65C02S_MODE_ZEROPAGE_BIT,        
    W65C02S_MODE_IMPLIED,             W65C02S_MODE_ABSOLUTE_Y,          
    W65C02S_MODE_STACK_PULL,          W65C02S_MODE_IMPLIED_1C,          
    W65C02S_MODE_ABSOLUTE,            W65C02S_MODE_ABSOLUTE_X,          
    W65C02S_MODE_RMW_ABSOLUTE_X,      W65C02S_MODE_RELATIVE_BIT,        
};

/*
               0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F 
      00:0F   BRK ORA NOP NOP TSB ORA ASL 000 PHP ORA ASL NOP TSB ORA ASL 000 
      10:1F   BPL ORA ORA NOP TRB ORA ASL 001 CLC ORA INC NOP TRB ORA ASL 001 
      20:2F   JSR AND NOP NOP BIT AND ROL 002 PLP AND ROL NOP BIT AND ROL 002 
      30:3F   BMI AND AND NOP BIT AND ROL 003 SEC AND DEC NOP BIT AND ROL 003 
      40:4F   RTI EOR NOP NOP NOP EOR LSR 004 PHA EOR LSR NOP JMP EOR LSR 004 
      50:5F   BVC EOR EOR NOP NOP EOR LSR 005 CLI EOR PHY NOP NOP EOR LSR 005 
      60:6F   RTS ADC NOP NOP STZ ADC ROR 006 PLA ADC ROR NOP JMP ADC ROR 006 
      70:7F   BVS ADC ADC NOP STZ ADC ROR 007 SEI ADC PLY NOP JMP ADC ROR 007 
      80:8F   BRA STA NOP NOP STY STA STX 010 DEC BIT TXA NOP STY STA STX 010 
      90:9F   BCC STA STA NOP STY STA STX 011 TYA STA TXS NOP STZ STA STZ 011 
      A0:AF   LDY LDA LDX NOP LDY LDA LDX 012 TAY LDA TAX NOP LDY LDA LDX 012 
      B0:BF   BCS LDA LDA NOP LDY LDA LDX 013 CLV LDA TSX NOP LDY LDA LDX 013 
      C0:CF   CPY CMP NOP NOP CPY CMP DEC 014 INC CMP DEC WAI CPY CMP DEC 014 
      D0:DF   BNE CMP CMP NOP NOP CMP DEC 015 CLD CMP PHX STP NOP CMP DEC 015 
      E0:EF   CPX SBC NOP NOP CPX SBC INC 016 INC SBC NOP NOP CPX SBC INC 016 
      F0:FF   BEQ SBC SBC NOP NOP SBC INC 017 SED SBC PLX NOP NOP SBC INC 017 
*/

#if W65C02S_C11
_Alignas(256)
#endif
static const unsigned char w65c02s_opers[256] = {
    W65C02S_OPER_BRK, W65C02S_OPER_ORA, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_TSB, W65C02S_OPER_ORA, W65C02S_OPER_ASL, 000,
    W65C02S_OPER_PHP, W65C02S_OPER_ORA, W65C02S_OPER_ASL, W65C02S_OPER_NOP,
    W65C02S_OPER_TSB, W65C02S_OPER_ORA, W65C02S_OPER_ASL, 000,
    W65C02S_OPER_BPL, W65C02S_OPER_ORA, W65C02S_OPER_ORA, W65C02S_OPER_NOP,
    W65C02S_OPER_TRB, W65C02S_OPER_ORA, W65C02S_OPER_ASL, 001,
    W65C02S_OPER_CLC, W65C02S_OPER_ORA, W65C02S_OPER_INC, W65C02S_OPER_NOP,
    W65C02S_OPER_TRB, W65C02S_OPER_ORA, W65C02S_OPER_ASL, 001,
    W65C02S_OPER_JSR, W65C02S_OPER_AND, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_BIT, W65C02S_OPER_AND, W65C02S_OPER_ROL, 002,
    W65C02S_OPER_PLP, W65C02S_OPER_AND, W65C02S_OPER_ROL, W65C02S_OPER_NOP,
    W65C02S_OPER_BIT, W65C02S_OPER_AND, W65C02S_OPER_ROL, 002,
    W65C02S_OPER_BMI, W65C02S_OPER_AND, W65C02S_OPER_AND, W65C02S_OPER_NOP,
    W65C02S_OPER_BIT, W65C02S_OPER_AND, W65C02S_OPER_ROL, 003,
    W65C02S_OPER_SEC, W65C02S_OPER_AND, W65C02S_OPER_DEC, W65C02S_OPER_NOP,
    W65C02S_OPER_BIT, W65C02S_OPER_AND, W65C02S_OPER_ROL, 003,
    W65C02S_OPER_RTI, W65C02S_OPER_EOR, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_EOR, W65C02S_OPER_LSR, 004,
    W65C02S_OPER_PHA, W65C02S_OPER_EOR, W65C02S_OPER_LSR, W65C02S_OPER_NOP,
    W65C02S_OPER_JMP, W65C02S_OPER_EOR, W65C02S_OPER_LSR, 004,
    W65C02S_OPER_BVC, W65C02S_OPER_EOR, W65C02S_OPER_EOR, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_EOR, W65C02S_OPER_LSR, 005,
    W65C02S_OPER_CLI, W65C02S_OPER_EOR, W65C02S_OPER_PHY, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_EOR, W65C02S_OPER_LSR, 005,
    W65C02S_OPER_RTS, W65C02S_OPER_ADC, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_STZ, W65C02S_OPER_ADC, W65C02S_OPER_ROR, 006,
    W65C02S_OPER_PLA, W65C02S_OPER_ADC, W65C02S_OPER_ROR, W65C02S_OPER_NOP,
    W65C02S_OPER_JMP, W65C02S_OPER_ADC, W65C02S_OPER_ROR, 006,
    W65C02S_OPER_BVS, W65C02S_OPER_ADC, W65C02S_OPER_ADC, W65C02S_OPER_NOP,
    W65C02S_OPER_STZ, W65C02S_OPER_ADC, W65C02S_OPER_ROR, 007,
    W65C02S_OPER_SEI, W65C02S_OPER_ADC, W65C02S_OPER_PLY, W65C02S_OPER_NOP,
    W65C02S_OPER_JMP, W65C02S_OPER_ADC, W65C02S_OPER_ROR, 007,
    W65C02S_OPER_BRA, W65C02S_OPER_STA, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_STY, W65C02S_OPER_STA, W65C02S_OPER_STX, 010,
    W65C02S_OPER_DEC, W65C02S_OPER_BIT, W65C02S_OPER_TXA, W65C02S_OPER_NOP,
    W65C02S_OPER_STY, W65C02S_OPER_STA, W65C02S_OPER_STX, 010,
    W65C02S_OPER_BCC, W65C02S_OPER_STA, W65C02S_OPER_STA, W65C02S_OPER_NOP,
    W65C02S_OPER_STY, W65C02S_OPER_STA, W65C02S_OPER_STX, 011,
    W65C02S_OPER_TYA, W65C02S_OPER_STA, W65C02S_OPER_TXS, W65C02S_OPER_NOP,
    W65C02S_OPER_STZ, W65C02S_OPER_STA, W65C02S_OPER_STZ, 011,
    W65C02S_OPER_LDY, W65C02S_OPER_LDA, W65C02S_OPER_LDX, W65C02S_OPER_NOP,
    W65C02S_OPER_LDY, W65C02S_OPER_LDA, W65C02S_OPER_LDX, 012,
    W65C02S_OPER_TAY, W65C02S_OPER_LDA, W65C02S_OPER_TAX, W65C02S_OPER_NOP,
    W65C02S_OPER_LDY, W65C02S_OPER_LDA, W65C02S_OPER_LDX, 012,
    W65C02S_OPER_BCS, W65C02S_OPER_LDA, W65C02S_OPER_LDA, W65C02S_OPER_NOP,
    W65C02S_OPER_LDY, W65C02S_OPER_LDA, W65C02S_OPER_LDX, 013,
    W65C02S_OPER_CLV, W65C02S_OPER_LDA, W65C02S_OPER_TSX, W65C02S_OPER_NOP,
    W65C02S_OPER_LDY, W65C02S_OPER_LDA, W65C02S_OPER_LDX, 013,
    W65C02S_OPER_CPY, W65C02S_OPER_CMP, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_CPY, W65C02S_OPER_CMP, W65C02S_OPER_DEC, 014,
    W65C02S_OPER_INC, W65C02S_OPER_CMP, W65C02S_OPER_DEC, W65C02S_OPER_WAI,
    W65C02S_OPER_CPY, W65C02S_OPER_CMP, W65C02S_OPER_DEC, 014,
    W65C02S_OPER_BNE, W65C02S_OPER_CMP, W65C02S_OPER_CMP, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_CMP, W65C02S_OPER_DEC, 015,
    W65C02S_OPER_CLD, W65C02S_OPER_CMP, W65C02S_OPER_PHX, W65C02S_OPER_STP,
    W65C02S_OPER_NOP, W65C02S_OPER_CMP, W65C02S_OPER_DEC, 015,
    W65C02S_OPER_CPX, W65C02S_OPER_SBC, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_CPX, W65C02S_OPER_SBC, W65C02S_OPER_INC, 016,
    W65C02S_OPER_INC, W65C02S_OPER_SBC, W65C02S_OPER_NOP, W65C02S_OPER_NOP,
    W65C02S_OPER_CPX, W65C02S_OPER_SBC, W65C02S_OPER_INC, 016,
    W65C02S_OPER_BEQ, W65C02S_OPER_SBC, W65C02S_OPER_SBC, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_SBC, W65C02S_OPER_INC, 017,
    W65C02S_OPER_SED, W65C02S_OPER_SBC, W65C02S_OPER_PLX, W65C02S_OPER_NOP,
    W65C02S_OPER_NOP, W65C02S_OPER_SBC, W65C02S_OPER_INC, 017,
};

W65C02S_INLINE void w65c02s_decode(struct w65c02s_cpu *cpu, uint8_t ir) {
    cpu->mode = w65c02s_modes[ir];
    cpu->oper = w65c02s_opers[ir]; 
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                             EXECUTION LOOP                             | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



W65C02S_INLINE void w65c02s_handle_reset(struct w65c02s_cpu *cpu) {
    /* do RESET */
    cpu->in_rst = 1;
    cpu->in_nmi = cpu->in_irq = 0;
    W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RUN);
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
    W65C02S_SET_P(W65C02S_P_A1, 1);
    W65C02S_SET_P(W65C02S_P_B, 1);
}

W65C02S_INLINE void w65c02s_handle_nmi(struct w65c02s_cpu *cpu) {
    cpu->in_nmi = 1;
    cpu->int_trig &= ~W65C02S_CPU_STATE_NMI;
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
}

W65C02S_INLINE void w65c02s_handle_irq(struct w65c02s_cpu *cpu) {
    cpu->in_irq = 1;
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
}

W65C02S_INLINE int w65c02s_handle_interrupt(struct w65c02s_cpu *cpu) {
    if (W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_RESET)
        w65c02s_handle_reset(cpu);
    else if (W65C02S_CPU_STATE_HAS_NMI(cpu))
        w65c02s_handle_nmi(cpu);
    else if (W65C02S_CPU_STATE_HAS_IRQ(cpu))
        w65c02s_handle_irq(cpu);
    else
        return 0;
    W65C02S_READ(cpu->pc); /* stall for a cycle */
    w65c02s_decode(cpu, 0); /* force BRK to handle interrupt */
    return 1;
}

W65C02S_INLINE void w65c02s_handle_end_of_instruction(struct w65c02s_cpu *cpu) {
    /* increment instruction tally */
    ++cpu->total_instructions;
#if W65C02S_HOOK_EOI
    if (cpu->hook_eoi) (cpu->hook_eoi)();
#endif
}

#if !W65C02S_COARSE_CYCLE_COUNTER
#define W65C02S_SPENT_CYCLE             ++cpu->total_cycles
#else
#define W65C02S_SPENT_CYCLE
#endif

#define W65C02S_STARTING_INSTRUCTION 0
#define W65C02S_CONTINUE_INSTRUCTION 1

static int w65c02s_handle_stp_wai_i(struct w65c02s_cpu *cpu) {
    switch (W65C02S_CPU_STATE_EXTRACT(cpu)) {
        case W65C02S_CPU_STATE_WAIT:
            /* if there is an IRQ or NMI, latch it immediately and continue */
            if (cpu->int_trig) {
                w65c02s_irq_latch_slow(cpu);
                W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RUN);
                return 0;
            }
        case W65C02S_CPU_STATE_STOP:
            /* spurious read to waste a cycle */
            W65C02S_READ(cpu->pc); /* stall for a cycle */
            W65C02S_SPENT_CYCLE;
            return 1;
    }
    return 0;
}

#if !W65C02S_COARSE

static int w65c02s_handle_stp_wai_c(struct w65c02s_cpu *cpu) {
    switch (W65C02S_CPU_STATE_EXTRACT(cpu)) {
        case W65C02S_CPU_STATE_WAIT:
            for (;;) {
                if (W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_RESET) {
                    return 0;
                } else if (cpu->int_trig) {
                    w65c02s_irq_latch_slow(cpu);
                    W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RUN);
                    return 0;
                }
                W65C02S_READ(cpu->pc); /* stall for a cycle */
                if (W65C02S_CYCLE_CONDITION) return 1;
            }
        case W65C02S_CPU_STATE_STOP:
            for (;;) {
                if (W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_RESET)
                    return 0;
                W65C02S_READ(cpu->pc); /* stall for a cycle */
                if (W65C02S_CYCLE_CONDITION) return 1;
            }
    }
    return 0;
}

static unsigned long w65c02s_execute_c(struct w65c02s_cpu *cpu,
                                       unsigned long maximum_cycles) {
    if (W65C02S_UNLIKELY(!maximum_cycles)) return 0;

#if W65C02S_COARSE_CYCLE_COUNTER
    cpu->left_cycles = maximum_cycles;
#else
    cpu->target_cycles = cpu->total_cycles + maximum_cycles;
#endif
    if (cpu->cycl) {
        /* continue running instruction */
        if (w65c02s_run_mode(cpu, W65C02S_CONTINUE_INSTRUCTION)) {
            if (cpu->cycl) cpu->cycl += maximum_cycles;
            return maximum_cycles;
        }
        goto end_of_instruction;
    }

    /* new instruction, handle special states now */
    if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN))
        goto check_special_state;

    for (;;) {
        unsigned long cyclecount;

        w65c02s_decode(cpu, W65C02S_READ(cpu->pc++));
        w65c02s_prerun_mode(cpu);
        cpu->cycl = 1; /* interrupts don't need this -- no cycle skip in BRK */

decoded:
#if W65C02S_COARSE_CYCLE_COUNTER
        cyclecount = cpu->left_cycles;
#else
        cyclecount = cpu->total_cycles;
#endif
        if (W65C02S_UNLIKELY(W65C02S_CYCLE_CONDITION))
            return maximum_cycles;

        if (W65C02S_UNLIKELY(w65c02s_run_mode(cpu,
                             W65C02S_STARTING_INSTRUCTION))) {
            if (cpu->cycl) {
#if W65C02S_COARSE_CYCLE_COUNTER
                cpu->cycl += cyclecount - cpu->left_cycles;
#else
                cpu->cycl += cpu->total_cycles - cyclecount;
#endif
            } else {
                w65c02s_handle_end_of_instruction(cpu);
            }
            return maximum_cycles;
        }

#if W65C02S_COARSE_CYCLE_COUNTER
#define W65C02S_CYCLES_NOW (maximum_cycles - cpu->left_cycles)
#else
#define W65C02S_CYCLES_NOW \
    (maximum_cycles - (cpu->target_cycles - cpu->total_cycles))
#endif
end_of_instruction:
        w65c02s_handle_end_of_instruction(cpu);
        if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN)) {
check_special_state:
            if (w65c02s_handle_stp_wai_c(cpu)) return W65C02S_CYCLES_NOW;
            if (w65c02s_handle_interrupt(cpu)) goto decoded;
        }
    }
}

W65C02S_INLINE unsigned w65c02s_run_mode_c(struct w65c02s_cpu *cpu,
                                           unsigned cont) {
#if W65C02S_COARSE_CYCLE_COUNTER
    cpu->left_cycles = cont ? 0 : -1;
#else
    cpu->target_cycles = cpu->total_cycles - (cont ? 0 : 1);
#endif
    w65c02s_run_mode(cpu, cont);
#if W65C02S_COARSE_CYCLE_COUNTER
    return -cpu->left_cycles;
#else
    return cpu->total_cycles - cpu->target_cycles;
#endif
}

static unsigned long w65c02s_execute_ic(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    cycles = w65c02s_run_mode_c(cpu, W65C02S_CONTINUE_INSTRUCTION);
    w65c02s_handle_end_of_instruction(cpu);
    return cycles;
}

#endif /* W65C02S_COARSE */

static unsigned long w65c02s_execute_i(struct w65c02s_cpu *cpu) {
    unsigned cycles;
    if (W65C02S_UNLIKELY(cpu->cpu_state != W65C02S_CPU_STATE_RUN)) {
        if (w65c02s_handle_stp_wai_i(cpu)) return 1;
        if (w65c02s_handle_interrupt(cpu)) goto decoded;
    }

    w65c02s_decode(cpu, W65C02S_READ(cpu->pc++));    
decoded:
    w65c02s_prerun_mode(cpu);
    W65C02S_SPENT_CYCLE;

#if !W65C02S_COARSE
    cpu->cycl = 1; /* make sure we start at the first cycle */
    cycles = w65c02s_run_mode_c(cpu, W65C02S_STARTING_INSTRUCTION);
#else
    cycles = w65c02s_run_mode(cpu);
#endif
    w65c02s_handle_end_of_instruction(cpu);
    return cycles;
}



/* +------------------------------------------------------------------------+ */
/* |                                                                        | */
/* |                            MAIN  INTERFACE                             | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */



#if !W65C02S_LINK
static uint8_t w65c02s_openbus_read(struct w65c02s_cpu *cpu,
                                    uint16_t addr) {
    return 0xFF;
}

static void w65c02s_openbus_write(struct w65c02s_cpu *cpu,
                                  uint16_t addr, uint8_t value) { }
#endif

void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data) {
    cpu->total_cycles = cpu->total_instructions = 0;
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    cpu->int_trig = 0;
    cpu->in_nmi = cpu->in_rst = cpu->in_irq = 0;

#if !W65C02S_LINK
    cpu->mem_read = mem_read ? mem_read : &w65c02s_openbus_read;
    cpu->mem_write = mem_write ? mem_write : &w65c02s_openbus_write;
#endif
    cpu->hook_brk = NULL;
    cpu->hook_stp = NULL;
    cpu->hook_eoi = NULL;
    cpu->cpu_data = cpu_data;

    cpu->cpu_state = W65C02S_CPU_STATE_RESET;
}

unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu,
                                 unsigned long cycles) {
    unsigned long c = 0;
#if W65C02S_COARSE
    /* we may overflow otherwise */
    if (cycles > ULONG_MAX - 8) cycles = ULONG_MAX - 8;
    while (c < cycles)
        c += w65c02s_execute_i(cpu);
#else
    c = w65c02s_execute_c(cpu, cycles);
#endif
#if W65C02S_COARSE_CYCLE_COUNTER
    cpu->total_cycles += c;
#endif
    return c;
}

unsigned long w65c02s_step_instruction(struct w65c02s_cpu *cpu) {
    unsigned cycles;
#if !W65C02S_COARSE
    if (W65C02S_UNLIKELY(cpu->cycl))
        cycles = w65c02s_execute_ic(cpu);
#endif
        cycles = w65c02s_execute_i(cpu);
#if W65C02S_COARSE_CYCLE_COUNTER
    cpu->total_cycles += cycles;
#endif
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    return cycles;
}

unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       int finish_existing) {
    unsigned long total_cycles = 0;
    if (W65C02S_UNLIKELY(!instructions)) return 0;
#if !W65C02S_COARSE
    if (W65C02S_UNLIKELY(cpu->cycl)) {
        total_cycles += w65c02s_execute_ic(cpu);
        if (!finish_existing && !--instructions)
            return total_cycles;
    }
#else
    (void)finish_existing;
#endif
    while (instructions--)
        total_cycles += w65c02s_execute_i(cpu);
#if W65C02S_COARSE_CYCLE_COUNTER
    cpu->total_cycles += total_cycles;
#endif
#if !W65C02S_COARSE
    cpu->cycl = 0;
#endif
    return total_cycles;
}

void w65c02s_nmi(struct w65c02s_cpu *cpu) {
    cpu->int_trig |= W65C02S_CPU_STATE_NMI;
    if (W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_WAIT) {
        W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RUN);
        W65C02S_CPU_STATE_ASSERT_NMI(cpu);
    }
}

void w65c02s_reset(struct w65c02s_cpu *cpu) {
    W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RESET);
    W65C02S_CPU_STATE_CLEAR_IRQ(cpu);
    W65C02S_CPU_STATE_CLEAR_NMI(cpu);
}

void w65c02s_irq(struct w65c02s_cpu *cpu) {
    cpu->int_trig |= W65C02S_CPU_STATE_IRQ;
    if (W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_WAIT) {
        W65C02S_CPU_STATE_INSERT(cpu, W65C02S_CPU_STATE_RUN);
        cpu->cpu_state |= W65C02S_CPU_STATE_IRQ & cpu->int_mask;
    }
}

void w65c02s_irq_cancel(struct w65c02s_cpu *cpu) {
    cpu->int_trig &= ~W65C02S_CPU_STATE_IRQ;
}

/* brk_hook: 0 = treat BRK as normal, <>0 = treat it as NOP */
int w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t)) {
#if W65C02S_HOOK_BRK
    cpu->hook_brk = brk_hook;
    return 1;
#else
    return 0;
#endif
}

/* stp_hook: 0 = treat STP as normal, <>0 = treat it as NOP */
int w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void)) {
#if W65C02S_HOOK_STP
    cpu->hook_stp = stp_hook;
    return 1;
#else
    return 0;
#endif
}

int w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                    void (*instruction_hook)(void)) {
#if W65C02S_HOOK_EOI
    cpu->hook_eoi = instruction_hook;
    return 1;
#else
    return 0;
#endif
}

unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_cycles;
}

unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu) {
    return cpu->total_instructions;
}

void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu) {
    cpu->total_cycles = 0;
}

void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu) {
    cpu->total_instructions = 0;
}

void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu) {
    return cpu->cpu_data;
}

bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu) {
    return W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_WAIT;
}

bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu) {
    return W65C02S_CPU_STATE_EXTRACT(cpu) == W65C02S_CPU_STATE_STOP;
}

void w65c02s_set_overflow(struct w65c02s_cpu *cpu) {
    cpu->p |= W65C02S_P_V;
}

uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu) {
    return cpu->a;
}

uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu) {
    return cpu->x;
}

uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu) {
    return cpu->y;
}

uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu) {
    return cpu->p | W65C02S_P_A1 | W65C02S_P_B;
}

uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu) {
    return cpu->s;
}

uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu) {
    return cpu->pc;
}

void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->a = v;
}

void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->x = v;
}

void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->y = v;
}

void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->p = v | W65C02S_P_A1 | W65C02S_P_B;
    w65c02s_irq_update_mask(cpu);
}

void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v) {
    cpu->s = v;
}

void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v) {
    cpu->pc = v;
}

#endif /* W65C02S_IMPL */
