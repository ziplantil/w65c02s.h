/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-15

            execute.h - instruction execution unit
*******************************************************************************/

#ifndef W65C02SCE_EXECUTE_H
#define W65C02SCE_EXECUTE_H

#define W65C02SCE
#include "w65c02s.h"

INTERNAL void w65c02si_execute(struct w65c02s_cpu *cpu);

#endif /* W65C02SCE_EXECUTE_H */
