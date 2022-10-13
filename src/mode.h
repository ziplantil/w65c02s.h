/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            mode.h - addressing modes
*******************************************************************************/

#ifndef W65C02SCE_MODE_H
#define W65C02SCE_MODE_H

#include "w65c02s.h"

#if W65C02SCE_LINK
#define READ(a) w65c02s_read(a)
#define WRITE(a, v) w65c02s_write(a, v)
#else
#define READ(a) (*cpu->mem_read)(a)
#define WRITE(a, v) (*cpu->mem_write)(a, v)
#endif

INTERNAL int w65c02si_run_mode(struct w65c02s_cpu *cpu);

#endif /* W65C02SCE_MODE_H */
