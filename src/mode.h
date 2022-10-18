/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-18

            mode.h - addressing modes
*******************************************************************************/

#ifndef W65C02SCE_MODE_H
#define W65C02SCE_MODE_H

#define W65C02SCE
#include "w65c02s.h"

#if W65C02SCE_LINK
#define READ(a) w65c02s_read(a)
#define WRITE(a, v) w65c02s_write(a, v)
#else
#define READ(a) (*cpu->mem_read)(cpu, a)
#define WRITE(a, v) (*cpu->mem_write)(cpu, a, v)
#endif

#if W65C02SCE_SEPARATE
INTERNAL_INLINE void w65c02si_irq_latch(struct w65c02s_cpu *cpu);
INTERNAL_INLINE void w65c02si_stall(struct w65c02s_cpu *cpu);
INTERNAL_INLINE void w65c02si_prerun_mode(struct w65c02s_cpu *cpu);
INTERNAL unsigned w65c02si_run_mode(struct w65c02s_cpu *cpu, unsigned cont);

#endif /* W65C02SCE_SEPARATE */

#endif /* W65C02SCE_MODE_H */
