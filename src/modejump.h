/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            modejump.h - addressing mode jump table implementation
*******************************************************************************/

#ifndef W65C02SCE_MODEJUMP_H
#define W65C02SCE_MODEJUMP_H

#if __STDC_VERSION__ >= 202309L
#include <stddef.h>
#elif __GNUC__ >= 4
#define unreachable() __builtin_unreachable()
#elif defined(MSC_VER)
#define unreachable() __assume(0)
#else
#define unreachable()
#endif

#define BEGIN_INSTRUCTION     switch (cpu->cycl) { case 0: unreachable();
#define CYCLE_END             if (!--cpu->left_cycles) return 1;
#define CYCLE(n)                  CYCLE_END                                    \
                              goto cycle_##n; cycle_##n: case n
#define END_INSTRUCTION(n)        CYCLE_END                                    \
                              case n: break;                                   \
                              }                                                \
                              return 0;

#define SKIP_REST             return 0
#define SKIP_TO_NEXT(n)       do { ++cpu->cycl; goto cycle_##n; } while (0)

/* example instruction:

    BEGIN_INSTRUCTION
        CYCLE(1):
            foo();
        CYCLE(2):
            bar();
    END_INSTRUCTION

becomes

    switch (cpu->cycl) {
        case 1:
            foo();
            if (!--cpu->left_cycles) return 1;
        case 2:
            bar();
            if (!--cpu->left_cycles) return 1;
    }
    return 0;

*/

#endif /* W65C02SCE_MODEJUMP_H */
