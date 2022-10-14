/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            mjump.h - addressing mode jump table implementation
*******************************************************************************/

#ifndef W65C02SCE_MJUMP_H
#define W65C02SCE_MJUMP_H

#define W65C02SCE
#include "w65c02s.h"

#define SKIP_REST             return 0
#define SKIP_TO_NEXT(n)       do { ++cpu->cycl; goto cycle_##n; } while (0)
#define BEGIN_INSTRUCTION     switch (cpu->cycl) { case 0: unreachable();
#define CYCLE_END             if (!--cpu->left_cycles) return 1;
#define CYCLE(n)                  CYCLE_END                                    \
                              goto cycle_##n; cycle_##n: case n
#define END_INSTRUCTION(n)        CYCLE_END                                    \
                              case n: break;                                   \
                              }                                                \
                              SKIP_REST;

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

#endif /* W65C02SCE_MJUMP_H */
