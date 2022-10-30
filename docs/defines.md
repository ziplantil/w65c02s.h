# Compile-time flags (defines)

## W65C02S_COARSE
* **Default**: 0 (disabled)

If disabled, w65c02s.h always runs exactly as many cycles as requested.

If enabled, w65c02s.h may run more or fewer cycles than requested, which
will be reflected in the return value from `w65c02s_run_cycles`. To be
specific, the emulator will only run instructions as atomic units and will
never stop in the middle of an instruction. 

Enabling coarse simulation effectively prevents cycle single-stepping and
causes small deviations in the return value. For example, trying to run
300,000 cycles with `w65c02s_run_cycles` may actually end up running
300,002 cycles, but this is reflected in its return value.

By compromising on emulation resolution, coarse mode considerably increases
emulation performance (perhaps by as much as 30-40%, depending on the
target system and used compiler optimizations).

## W65C02S_LINK
* **Default**: 0 (disabled)

If set to 0, w65c02s.h performs memory accesses through the `mem_read` and
`mem_write` callbacks given to `w65c02s.h_init`.

If set to 1, they are ignored and w65c02s.h instead assumes the following
two functions are defined externally and uses them for memory access:

```c
extern uint8_t w65c02s_read(uint16_t address);
extern void w65c02s_write(uint16_t address, uint8_t value);
```

## W65C02S_HOOK_BRK
* **Default**: 0 (disabled)

If set to 1, the BRK instruction hook is implemented. Required for
`w65c02s_hook_brk` to function.

## W65C02S_HOOK_STP
* **Default**: 0 (disabled)

If set to 1, the STP instruction hook is implemented. Required for
`w65c02s_hook_stp` to function.

## W65C02S_HOOK_EOI
* **Default**: 0 (disabled)

If set to 1, the end-of-instruction hook is implemented. Required for
`w65c02s_hook_end_of_instruction` to function.

## W65C02S_HAS_BOOL
* **Default**: 0 (disabled)

Set to 1 if there is a `bool` data type available without `stdbool.h`
(or a fallback to `int` if C99 is not available).

## W65C02S_HAS_UINTN_T
* **Default**: 0 (disabled)

Set to 1 if there are `uint8_t` and `uint16_t` data types available
without `stdint.h` (or a fallback to `unsigned char`, `unsigned short`
if C99 is not available).
