# Compile-time flags (defines)

## W65C02SCE_ACCURATE
* **Default**: 1 (enabled)

If enabled, w65c02sce always runs exactly as many cycles as requested.

If disabled, w65c02sce may run more or fewer cycles than requested, which
will be reflected in the return value from `w65c02s_run_cycles`. Disabling
`W65C02SCE_ACCURATE` may improve performance.

## W65C02SCE_LINK
* **Default**: 0 (disabled)

If set to 0, w65c02sce performs memory accesses through the `mem_read` and
`mem_write` callbacks given to `w65c02sce_init`.

If set to 1, they are ignored and w65c02sce instead assumes the following
two functions are defined externally and uses them for memory access:

```c
extern uint8_t w65c02s_read(uint16_t address);
extern void w65c02s_write(uint16_t address, uint8_t value);
```

## W65C02SCE_HOOK_BRK
* **Default**: 0 (disabled)

If set to 1, the BRK instruction hook is implemented. Required for
`w65c02s_hook_brk` to function.

## W65C02SCE_HOOK_STP
* **Default**: 0 (disabled)

If set to 1, the STP instruction hook is implemented. Required for
`w65c02s_hook_stp` to function.

## W65C02SCE_HOOK_EOI
* **Default**: 0 (disabled)

If set to 1, the end-of-instruction hook is implemented. Required for
`w65c02s_hook_end_of_instruction` to function.

## W65C02SCE_HOOK_REGS
* **Default**: 1 (enabled)

If set to 1, the register access getter and setter methods are compiled into
the library. They are not by default.

## W65C02SCE_SEPARATE
* **Default**: 0 (disabled)

If set to 0, only w65c02s.c should be compiled, and it will automatically
include everything it needs. The other .c files are included, but if compiled,
will result in empty objects. Compiling all of w65c02sce as one source file
can simulate link-time optimizations within the library even if none would
otherwise be performed.

If set to 1, all .c files are compiled separately.

## W65C02SCE_HAS_BOOL
* **Default**: 0 (disabled)

Set to 1 if there is a `bool` data type available without `stdbool.h`
(or a fallback to `int` if C99 is not available).

## W65C02SCE_HAS_UINTN_T
* **Default**: 0 (disabled)

Set to 1 if there are `uint8_t` and `uint16_t` data types available
without `stdint.h` (or a fallback to `unsigned char`, `unsigned short`
if C99 is not available).
