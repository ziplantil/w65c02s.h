/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-18

            execute.h - instruction execution unit
*******************************************************************************/

#ifndef W65C02SCE_EXECUTE_H
#define W65C02SCE_EXECUTE_H

#define W65C02SCE
#include "w65c02s.h"

INTERNAL_INLINE void w65c02si_irq_update_mask(struct w65c02s_cpu *cpu);
#if !W65C02SCE_COARSE
INTERNAL unsigned long w65c02si_execute_c(struct w65c02s_cpu *cpu,
                                          unsigned long maximum_cycles);
#else /* W65C02SCE_COARSE */
INTERNAL unsigned long w65c02si_execute_i(struct w65c02s_cpu *cpu);
#endif /* W65C02SCE_COARSE */

#endif /* W65C02SCE_EXECUTE_H */
