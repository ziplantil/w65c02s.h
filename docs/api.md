## w65c02s_cpu_size
Returns the size of `struct w65c02s_cpu` for allocation purposes.

```c
size_t w65c02s_cpu_size(void);
```

struct w65c02s_cpu is an opaque type and its fields should not be accessed
directly. The intended purpose is to define `W65C20S_IMPL` in one file and then
use a CPU instance registered in such a file, so that the underlying library is
not visible to the rest of the code.

However, in some cases, the size of the struct may be required, such a when
performing dynamic allocation. This function helps with that; it returns the
space needed by the struct. This value can then be passed to malloc or another
similar allocator.

* **Return value**: The number of chars needed by the struct

## w65c02s_init
Initializes a new CPU instance.

```c
void w65c02s_init(struct w65c02s_cpu *cpu,
                  uint8_t (*mem_read)(struct w65c02s_cpu *, uint16_t),
                  void (*mem_write)(struct w65c02s_cpu *, uint16_t, uint8_t),
                  void *cpu_data);
```

mem_read and mem_write only need to be specified if `W65C02S_LINK` is set to 0.
If set to 1, they may simply be passed as NULL.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `mem_read`: The read callback, ignored if `W65C02S_LINK` is 1.
* **Parameter** `mem_write`: The write callback, ignored if `W65C02S_LINK` is
  1.
* **Parameter** `cpu_data`: The pointer to be returned by
  w65c02s_get_cpu_data().
* **Return value**: The number of cycles that were actually run

## w65c02s_read
Reads a value from memory.

```c
extern uint8_t w65c02s_read(uint16_t address);
```

This method only exists if the library is compiled with `W65C02S_LINK`. In that
case, you must provide an implementation.

* **Parameter** `address`: The 16-bit address to read from
* **Return value**: The value read from memory

## w65c02s_write
Writes a value to memory.

```c
extern void w65c02s_write(uint16_t address, uint8_t value);
```

This method only exists if the library is compiled with `W65C02S_LINK`. In that
case, you must provide an implementation.

* **Parameter** `address`: The 16-bit address to write to
* **Parameter** `value`: The value to write

## w65c02s_run_cycles
Runs the CPU for the given number of cycles.

```c
unsigned long w65c02s_run_cycles(struct w65c02s_cpu *cpu, unsigned long cycles);
```

If w65c02ce is compiled with `W65C02S_COARSE`, the actual number of cycles
executed may not always match the given argument, but the return value is
always correct. Without `W65C02S_COARSE` the return value is always the same as
`cycles`.

This function is not reentrant. Calling it from a callback (for a memory read,
write, STP, etc.) will result in undefined behavior.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `cycles`: The number of cycles to run
* **Return value**: The number of cycles that were actually run

## w65c02s_step_instruction
Runs the CPU for one instruction, or if an instruction is already running,
finishes that instruction.

```c
unsigned long w65c02s_step_instruction(struct w65c02s_cpu *cpu);
```

Prefer using w65c02s_run_instructions or w65c02s_run_cycles instead if you can
to run many instructions at once.

This function is not reentrant. Calling it from a callback (for a memory read,
write, STP, etc.) will result in undefined behavior.

* **Parameter** `cpu`: The CPU instance to run
* **Return value**: The number of cycles that were actually run

## w65c02s_run_instructions
Runs the CPU for the given number of instructions.

```c
unsigned long w65c02s_run_instructions(struct w65c02s_cpu *cpu,
                                       unsigned long instructions,
                                       bool finish_existing);
```

finish_existing only matters if the CPU is currently mid-instruction. If it is,
specifying finish_existing as true will finish the current instruction before
running any new ones, and the existing instruction will not count towards the
instruction count limit. If finish_existing is false, the current instruction
will be counted towards the limit.

Entering an interrupt counts as an instruction here.

This function is not reentrant. Calling it from a callback (for a memory read,
write, STP, etc.) will result in undefined behavior.

* **Parameter** `cpu`: The CPU instance to run
* **Parameter** `instructions`: The number of instructions to run
* **Parameter** `finish_existing`: Whether to finish the current instruction
* **Return value**: The number of cycles that were actually run (mind the
  overflow with large values of instructions!)

## w65c02s_get_cycle_count
Gets the total number of cycles executed by this CPU.

```c
unsigned long w65c02s_get_cycle_count(const struct w65c02s_cpu *cpu);
```

If it hasn't, the value returned by this function reflects how many cycles have
been run. For example, on the first reset cycle (spurious read of the PC) of a
new CPU instance, this will return 0.

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

## w65c02s_break
Triggers a CPU break.

```c
void w65c02s_break(struct w65c02s_cpu *cpu);
```

If w65c02s_run_cycles or w65c02s_run_instructions is running, it will return as
soon as possible. In coarse mode, this will finish the current instruction.

In non-coarse mode, it will finish the current cycle. Note that this means that
if `w65c02s_get_cycle_count` returns 999 (i.e. 999 cycles have been run), the
cycle count will tick over to 1000 before the run call returns. This is because
the thousandth cycle will still get finished.

If the CPU is not running, the function does nothing.

This function is designed to be called from callbacks (e.g. memory access or
end of instruction).

* **Parameter** `cpu`: The CPU instance

## w65c02s_stall
Stalls the CPU stall for the given number of cycles. This feature is intended
for use with e.g. memory accesses that require additional cycles (which, on a
real chip, would generally work by pulling RDY low).

```c
void w65c02s_stall(struct w65c02s_cpu *cpu, unsigned long cycles);
```

The stall cycles will run at the beginning of the next w65c02s_run_cycles or
w65c02s_run_instructions. w65c02s_stall will automatically call w65c02s_break
to break execution if any is running.

This function is designed to be called from callbacks (e.g. memory access or
end of instruction).

* **Parameter** `cpu`: The CPU instance
* **Parameter** `cycles`: The number of cycles to stall

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

The RESET will begin executing before the next instruction. The RESET does not
behave exactly like in the original chip, and will not carry out any extra
cycles in between the end of the instruction and beginning of the reset (like
some chips do). There is also no requirement that RESET be asserted before the
last cycle, like with NMI or IRQ.

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
bool w65c02s_hook_brk(struct w65c02s_cpu *cpu, bool (*brk_hook)(uint8_t));
```

The hook function should take a single uint8_t parameter, which corresponds to
the immediate parameter after the BRK opcode. If the hook function returns a
non-zero value, the BRK instruction is skipped, and otherwise it is treated as
normal.

Passing NULL as the hook disables the hook.

This function does nothing if the library was not compiled with
`W65C02S_HOOK_BRK`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `brk_hook`: The new BRK hook
* **Return value**: Whether the hook was set (false only if the library was
  compiled without `W65C02S_HOOK_BRK`)

## w65c02s_hook_stp
Hooks the STP instruction on the CPU.

```c
bool w65c02s_hook_stp(struct w65c02s_cpu *cpu, bool (*stp_hook)(void));
```

The hook function should take no parameters. If it returns a non-zero value,
the STP instruction is skipped, and otherwise it is treated as normal.

Passing NULL as the hook disables the hook.

This function does nothing if the library was not compiled with
`W65C02S_HOOK_STP`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `stp_hook`: The new STP hook
* **Return value**: Whether the hook was set (0 only if the library was
  compiled without `W65C02S_HOOK_STP`)

## w65c02s_hook_end_of_instruction
Hooks the end-of-instruction on the CPU.

```c
bool w65c02s_hook_end_of_instruction(struct w65c02s_cpu *cpu,
                                     void (*instruction_hook)(void));
```

The hook function should take no parameters. It is called when an instruction
finishes. The interrupt entering routine counts as an instruction here.

Passing NULL as the hook disables the hook.

This function does nothing if the library was not compiled with
`W65C02S_HOOK_EOI`.

* **Parameter** `cpu`: The CPU instance
* **Parameter** `instruction_hook`: The new end-of-instruction hook
* **Return value**: Whether the hook was set (0 only if the library was
  compiled without `W65C02S_HOOK_EOI`)

## w65c02s_reg_get_a
Returns the value of the A register on the CPU.

```c
uint8_t w65c02s_reg_get_a(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the A register

## w65c02s_reg_get_x
Returns the value of the X register on the CPU.

```c
uint8_t w65c02s_reg_get_x(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the X register

## w65c02s_reg_get_y
Returns the value of the Y register on the CPU.

```c
uint8_t w65c02s_reg_get_y(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the Y register

## w65c02s_reg_get_p
Returns the value of the P (processor status) register on the CPU.

```c
uint8_t w65c02s_reg_get_p(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the P register

## w65c02s_reg_get_s
Returns the value of the S (stack pointer) register on the CPU.

```c
uint8_t w65c02s_reg_get_s(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the S register

## w65c02s_reg_get_pc
Returns the value of the PC (program counter) register on the CPU.

```c
uint16_t w65c02s_reg_get_pc(const struct w65c02s_cpu *cpu);
```

* **Parameter** `cpu`: The CPU instance
* **Return value**: The value of the PC register

## w65c02s_reg_set_a
Replaces the value of the A register on the CPU.

```c
void w65c02s_reg_set_a(struct w65c02s_cpu *cpu, uint8_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the A register

## w65c02s_reg_set_x
Replaces the value of the X register on the CPU.

```c
void w65c02s_reg_set_x(struct w65c02s_cpu *cpu, uint8_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the X register

## w65c02s_reg_set_y
Replaces the value of the Y register on the CPU.

```c
void w65c02s_reg_set_y(struct w65c02s_cpu *cpu, uint8_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the Y register

## w65c02s_reg_set_p
Replaces the value of the P (processor status) register on the CPU.

```c
void w65c02s_reg_set_p(struct w65c02s_cpu *cpu, uint8_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the P register

## w65c02s_reg_set_s
Replaces the value of the S (stack pointer) register on the CPU.

```c
void w65c02s_reg_set_s(struct w65c02s_cpu *cpu, uint8_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the S register

## w65c02s_reg_set_pc
Replaces the value of the PC (program counter) register on the CPU.

```c
void w65c02s_reg_set_pc(struct w65c02s_cpu *cpu, uint16_t v);
```

* **Parameter** `cpu`: The CPU instance
* **Parameter** `v`: The new value of the PC register

