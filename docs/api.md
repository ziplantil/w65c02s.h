```c
#include "w65c02s.h"
```

## w65c02s_init
Initializes a new CPU instance.

```c
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data);
```

mem_read and mem_write only need to be specified if `W65C02SCE_LINK` is set to
0. If set to 1, they may simply be passed as NULL.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `mem_read`: The read callback, ignored if `W65C02SCE_LINK` is
  1.
* **Parameter** `mem_write`: The write callback, ignored if `W65C02SCE_LINK` is
  1.
* **Parameter** `cpu_data`: The pointer to be returned by
  w65c02s_get_cpu_data().
* **Return value**: The number of cycles that were actually run

## w65c02s_read
Reads a value from memory.

```c
extern uint8_t w65c02s_read(uint16_t address);
```

This method only exists if the library is compiled with `W65C02SCE_LINK`. In
that case, you must provide an implementation.

* **Parameter** `address`: The 16-bit address to read from
* **Return value**: The value read from memory

## w65c02s_write
Writes a value to memory.

```c
extern void w65c02s_write(uint16_t address, uint8_t value);
```

This method only exists if the library is compiled with `W65C02SCE_LINK`. In
that case, you must provide an implementation.

* **Parameter** `address`: The 16-bit address to write to
* **Parameter** `value`: The value to write

## w65c02s_run_cycles
Runs the CPU for the given number of cycles.

```c
unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles);
```

If w65c02ce is compiled with `W65C02SCE_ACCURATE`, the CPU is always run
exactly as many cycles as specified, but otherwise the actual number of cycles
executed may not always match the specified value.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `cycles`: The number of cycles to run
* **Return value**: The number of cycles that were actually run

## w65c02s_run_instructions
Runs the CPU for the given number of instructions.

```c
unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       int finish_existing);
```

finish_existing only matters if the CPU is currently mid-instruction. If it is,
specifying finish_existing as true will finish the current instruction before
running any new ones, and the existing instruction will not count towards the
instruction count limit. If finish_existing is false, the current instruction
will be counted towards the limit.

Entering an interrupt counts as an instruction here.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `instructions`: The number of instructions to run
* **Parameter** `finish_existing`: Whether to finish the current instruction
* **Return value**: The number of cycles that were actually run

## w65c02s_get_cycle_count
Gets the total number of cycles executed by this CPU.

```c
unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance to run
* **Return value**: The number of cycles run in total

## w65c02s_get_instruction_count
Gets the total number of instructions executed by this CPU.

```c
unsigned long w65c02s_get_instruction_count(const struct w65c02s_cpu *cpu);
```

Entering an interrupt counts does not count as an instruction here.

* **Parameter** `cpu`: The CPU instance to run
* **Return value**: The number of cycles run in total

## w65c02s_get_cpu_data
Gets the cpu_data pointer associated to the CPU with w65c02s_init.

```c
void *w65c02s_get_cpu_data(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance to get the pointer from
* **Return value**: The cpu_data pointer

## w65c02s_reset_cycle_count
Resets the total cycle counter (w65c02s_get_cycle_count).

```c
void w65c02s_reset_cycle_count(struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance

## w65c02s_reset_instruction_count
Resets the total instruction counter (w65c02s_get_instruction_count).

```c
void w65c02s_reset_instruction_count(struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance

## w65c02s_is_cpu_waiting
Checks whether the CPU is currently waiting for an interrupt (WAI).

```c
bool w65c02s_is_cpu_waiting(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: Whether the CPU ran a WAI instruction and is waiting

## w65c02s_is_cpu_stopped
Checks whether the CPU is currently stopped and waiting for a reset.

```c
bool w65c02s_is_cpu_stopped(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: Whether the CPU ran a STP instruction

## w65c02s_nmi
Queues a NMI (non-maskable interrupt) on the CPU.

```c
void w65c02s_nmi(struct w65c02s_cpu *cpu);
```

The NMI will generally execute before the next instruction, but it must be
triggered before the final cycle of an instruction, or it will be postponed to
after the next instruction. NMIs cannot be masked. Calling this function will
trigger an NMI only once.

* **Parameter** `cpu`: The CPU instance

## w65c02s_reset
Triggers a CPU reset.

```c
void w65c02s_reset(struct w65c02s_cpu *cpu);
```

The RESET will begin executing before the next instruction.

* **Parameter** `cpu`: The CPU instance

## w65c02s_irq
Pulls the IRQ line high (logically) on the CPU.

```c
void w65c02s_irq(struct w65c02s_cpu *cpu);
```

If interrupts are enabled on the CPU, the IRQ handler will execute before the
next instruction. Like with NMI, an IRQ on the last cycle will be delayed until
after the next instruction. The IRQ line is held until w65c02s_irq_cancel is
called.

* **Parameter** `cpu`: The CPU instance

## w65c02s_irq_cancel
Pulls the IRQ line low (logically) on the CPU.

```c
void w65c02s_irq_cancel(struct w65c02s_cpu *cpu);
```

The IRQ is cancelled and will no longer be triggered. Triggering an IRQ only
once is tricky. w65c02s_irq and w65c02s_irq_cancel immediately will not cause
an IRQ (since the CPU never samples it). It must be held for at least 7-8
cycles, but that may still not be enough if interrupts are disabled from the
CPU side.

Generally you would want to hold the IRQ line high and cancel the IRQ only once
it has been acknowledged by the CPU (e.g. through MMIO).

* **Parameter** `cpu`: The CPU instance

## w65c02s_set_overflow
Sets the overflow (V) flag on the status register (P) of the CPU.

```c
void w65c02s_set_overflow(struct w65c02s_cpu *cpu);
```

This corresponds to the S/O pin on the physical CPU.

* **Parameter** `cpu`: The CPU instance

## w65c02s_hook_brk
Hooks the BRK instruction on the CPU.

```c
void w65c02s_hook_brk(struct w65c02s_cpu *cpu, int (*brk_hook)(uint8_t));
```

The hook function should take a single uint8_t parameter, which corresponds to
the immediate parameter after the BRK opcode. If the hook function returns a
non-zero value, the BRK instruction is skipped, and otherwise it is treated as
normal.

Passing NULL as the hook disables the hook.

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_BRK`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `brk_hook`: The new BRK hook

## w65c02s_hook_stp
Hooks the STP instruction on the CPU.

```c
void w65c02s_hook_stp(struct w65c02s_cpu *cpu, int (*stp_hook)(void));
```

The hook function should take no parameters. If it returns a non-zero value,
the STP instruction is skipped, and otherwise it is treated as normal.

Passing NULL as the hook disables the hook.

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_STP`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `stp_hook`: The new STP hook

## w65c02s_hook_end_of_instruction
Hooks the end-of-instruction on the CPU.

```c
void w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                     void (*instruction_hook)(void));
```

The hook function should take no parameters. It is called when an instruction
finishes. The interrupt entering routine counts as an instruction here.

Passing NULL as the hook disables the hook.

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_EOI`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `instruction_hook`: The new end-of-instruction hook

## w65c02s_reg_get_a
Returns the value of the A register on the CPU.

```c
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the A register

## w65c02s_reg_get_x
Returns the value of the X register on the CPU.

```c
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the X register

## w65c02s_reg_get_y
Returns the value of the Y register on the CPU.

```c
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the Y register

## w65c02s_reg_get_p
Returns the value of the P (processor status) register on the CPU.

```c
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the P register

## w65c02s_reg_get_s
Returns the value of the S (stack pointer) register on the CPU.

```c
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the S register

## w65c02s_reg_get_pc
Returns the value of the PC (program counter) register on the CPU.

```c
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the PC register

## w65c02s_reg_set_a
Replaces the value of the A register on the CPU.

```c
void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the A register

## w65c02s_reg_set_x
Replaces the value of the X register on the CPU.

```c
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the X register

## w65c02s_reg_set_y
Replaces the value of the Y register on the CPU.

```c
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the Y register

## w65c02s_reg_set_p
Replaces the value of the P (processor status) register on the CPU.

```c
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the P register

## w65c02s_reg_set_s
Replaces the value of the S (stack pointer) register on the CPU.

```c
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the S register

## w65c02s_reg_set_pc
Replaces the value of the PC (program counter) register on the CPU.

```c
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);
```

This method is only available if the code was compiled with the flag
`W65C02SCE_HOOK_REGS`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the PC register

