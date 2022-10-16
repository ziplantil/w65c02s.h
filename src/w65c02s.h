/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-16

            w65c02s.h - main emulator definitions (and external API)
*******************************************************************************/

#ifndef W65C02SCE_W65C02S_H
#define W65C02SCE_W65C02S_H

/* defines */

/* 1: always runs instructions as a whole */
/* 0: accurate cycle counter, can stop and resume mid-instruction. */
#ifndef W65C02SCE_COARSE
#define W65C02SCE_COARSE 0
#endif

/* 1: cycle counter is updated only at the end of an execution loop */
/* 0: cycle counter is accurate even in callbacks */
#ifndef W65C02SCE_COARSE_CYCLE_COUNTER
#define W65C02SCE_COARSE_CYCLE_COUNTER 0
#endif

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
#define W65C02SCE_HOOK_REGS 1
#endif

/* 1: has bool without stdbool.h */
/* 0: does not have bool without stdbool.h */
#ifndef W65C02SCE_HAS_BOOL 
#define W65C02SCE_HAS_BOOL 0
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

#if W65C02SCE_HAS_UINTN_T
/* do nothing, we have these types */
#elif __STDC_VERSION__ >= 199901L || HAS_STDINT_H || HAS_STDINT
#include <stdint.h>
#else
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

#define STATIC static
#if __STDC_VERSION__ >= 199901L
#if __GNUC__
#define INLINE __attribute__((always_inline)) static inline
#elif defined(_MSC_VER)
#define INLINE __forceinline static inline
#else
#define INLINE static inline
#endif
#else
#define INLINE static
#endif

#if !W65C02SCE_SEPARATE
#define INTERNAL STATIC
#define INTERNAL_INLINE INLINE
#else
#define INTERNAL
#define INTERNAL_INLINE
#endif

#if __STDC_VERSION__ >= 202309L
#include <stddef.h>
#elif __GNUC__ >= 4
#define unreachable() __builtin_unreachable()
#elif defined(MSC_VER)
#define unreachable() __assume(0)
#else
#define unreachable()
#endif

#if __GNUC__ >= 5
#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#define CPU_STATE_RUN 0
#define CPU_STATE_RESET 1
#define CPU_STATE_WAIT 2
#define CPU_STATE_STOP 3
#define CPU_STATE_IRQ 4
#define CPU_STATE_NMI 8
#define CPU_STATE_STEP 16

#define CPU_STATE_EXTRACT(cpu)      ((cpu)->cpu_state & 3)
#define CPU_STATE_INSERT(cpu, s)    ((cpu)->cpu_state =                        \
                                            ((cpu)->cpu_state & ~3) | s)
#define CPU_STATE_HAS_NMI(cpu)      ((cpu)->cpu_state & CPU_STATE_NMI)
#define CPU_STATE_HAS_IRQ(cpu)      ((cpu)->cpu_state & CPU_STATE_IRQ)
#define CPU_STATE_ASSERT_NMI(cpu)   ((cpu)->cpu_state |= CPU_STATE_NMI)
#define CPU_STATE_ASSERT_IRQ(cpu)   ((cpu)->cpu_state |= CPU_STATE_IRQ)
#define CPU_STATE_CLEAR_NMI(cpu)    ((cpu)->cpu_state &= ~CPU_STATE_NMI)
#define CPU_STATE_CLEAR_IRQ(cpu)    ((cpu)->cpu_state &= ~CPU_STATE_IRQ)

#endif /* W65C02SCE */

#if W65C02SCE_HAS_BOOL
/* has bool */
#elif __STDC_VERSION__ >= 199901L || HAS_STDBOOL_H
#include <stdbool.h>
#else
typedef int bool;
#endif

/* DO NOT ACCESS THESE FIELDS YOURSELF IN EXTERNAL CODE!
   all values here are internal! do not rely on them! use methods instead! */
struct w65c02s_cpu {
#if !W65C02SCE_COARSE
    unsigned long left_cycles;
#endif
    unsigned cpu_state;

    uint16_t pc;
    uint8_t a, x, y, s, p, p_adj; /* p_adj for decimal mode */

    /* temporary true/false */
    bool take;
#if __STDC_VERSION__ >= 201112L
    _Alignas(uint16_t)
#endif
    /* temporary registers used to store state between cycles. */
    uint8_t tr[5];

    unsigned long total_cycles;
    unsigned long total_instructions;

    /* addressing mode, operation */
    unsigned int mode, oper;
#if !W65C02SCE_COARSE
    /* cycle of current instruction */
    unsigned int cycl;
#endif

    /* NMI, RESET, IRQ interrupt flags */
    unsigned nmi, irq;
    bool in_nmi, in_rst, in_irq;    /* entering NMI, resetting or IRQ? */

    int (*hook_brk)(uint8_t);
    int (*hook_stp)(void);
    void (*hook_eoi)(void);
    void *cpu_data;
#if !W65C02SCE_LINK
    uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t);
    void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t);
#endif
};

/** w65c02s_init
 *
 *  Initializes a new CPU instance.
 *
 *  mem_read and mem_write only need to be specified if W65C02SCE_LINK is set
 *  to 0. If set to 1, they may simply be passed as NULL.
 *
 *  [Parameter: cpu] The CPU instance to run
 *  [Parameter: mem_read] The read callback, ignored if W65C02SCE_LINK is 1.
 *  [Parameter: mem_write] The write callback, ignored if W65C02SCE_LINK is 1.
 *  [Parameter: cpu_data] The pointer to be returned by w65c02s_get_cpu_data().
 *  [Return value] The number of cycles that were actually run
 */
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data);

#if W65C02SCE_LINK
/** w65c02s_read
 *
 *  Reads a value from memory.
 *
 *  This method only exists if the library is compiled with W65C02SCE_LINK.
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
 *  This method only exists if the library is compiled with W65C02SCE_LINK.
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
 *  If w65c02ce is compiled with W65C02SCE_COARSE, the actual number of
 *  cycles executed may not always match the given argument, but the return
 *  value is always correct. Without W65C02SCE_COARSE the return value is
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
 *  If the library has been compiled with W65C02SCE_COARSE_CYCLE_COUNTER,
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
 *  The RESET will begin executing before the next instruction.
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
 *  W65C02SCE_HOOK_BRK.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: brk_hook] The new BRK hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02SCE_HOOK_BRK)
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
 *  W65C02SCE_HOOK_STP.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: stp_hook] The new STP hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02SCE_HOOK_STP)
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
 *  W65C02SCE_HOOK_EOI.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: instruction_hook] The new end-of-instruction hook
 *  [Return value] Whether the hook was set (0 only if the library
 *                 was compiled without W65C02SCE_HOOK_EOI)
 */
int w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                    void (*instruction_hook)(void));

#if W65C02SCE_HOOK_REGS
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
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the X register
 */
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_y
 *
 *  Returns the value of the Y register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the Y register
 */
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_p
 *
 *  Returns the value of the P (processor status) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the P register
 */
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_s
 *
 *  Returns the value of the S (stack pointer) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the S register
 */
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_get_pc
 *
 *  Returns the value of the PC (program counter) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Return value] The value of the PC register
 */
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);

/** w65c02s_reg_set_a
 *
 *  Replaces the value of the A register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the A register
 */
void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_x
 *
 *  Replaces the value of the X register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the X register
 */
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_y
 *
 *  Replaces the value of the Y register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the Y register
 */
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_p
 *
 *  Replaces the value of the P (processor status) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the P register
 */
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_s
 *
 *  Replaces the value of the S (stack pointer) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the S register
 */
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);

/** w65c02s_reg_set_pc
 *
 *  Replaces the value of the PC (program counter) register on the CPU.
 *
 *  This method is only available if the code was compiled
 *  with the flag W65C02SCE_HOOK_REGS.
 *
 *  [Parameter: cpu] The CPU instance
 *  [Parameter: v] The new value of the PC register
 */
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);
#endif

#endif /* W65C02SCE_W65C02S_H */
