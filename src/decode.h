/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            decode.h - instruction decoder
*******************************************************************************/

#ifndef W65C02SCE_DECODE_H
#define W65C02SCE_DECODE_H

#define W65C02SCE
#include "w65c02s.h"

/* all possible values for cpu->mode        (example opcode, ! = only) */
#define MODE_IMPLIED                0       /*  CLD, DEC A */
#define MODE_IMMEDIATE              1       /*  LDA # */
#define MODE_RELATIVE               2       /*  BRA # */
#define MODE_RELATIVE_BIT           3       /*  BBR0 # */
#define MODE_ZEROPAGE               4       /*  LDA zp */
#define MODE_ZEROPAGE_X             5       /*  LDA zp,x */
#define MODE_ZEROPAGE_Y             6       /*  LDA zp,y */
#define MODE_ZEROPAGE_BIT           7       /*  RMB0 zp */
#define MODE_ABSOLUTE               8       /*  LDA abs */
#define MODE_ABSOLUTE_X             9       /*  LDA abs,x */
#define MODE_ABSOLUTE_Y             10      /*  LDA abs,y */
#define MODE_ZEROPAGE_INDIRECT      11      /*  ORA (zp) */

#define MODE_ZEROPAGE_INDIRECT_X    12      /*  LDA (zp,x) */
#define MODE_ZEROPAGE_INDIRECT_Y    13      /*  LDA (zp),y */
#define MODE_ABSOLUTE_INDIRECT      14      /*! JMP (abs) */
#define MODE_ABSOLUTE_INDIRECT_X    15      /*! JMP (abs,x) */
#define MODE_ABSOLUTE_JUMP          16      /*! JMP abs */

#define MODE_IMPLIED_1C             17      /*! NOP */
#define MODE_IMPLIED_X              18      /*  INX */
#define MODE_IMPLIED_Y              19      /*  INY */

#define MODE_RMW_ZEROPAGE           20      /*  LSR zp */
#define MODE_RMW_ZEROPAGE_X         21      /*  LSR zp,x */
#define MODE_SUBROUTINE             22      /*! JSR abs */
#define MODE_RETURN_SUB             23      /*! RTS */
#define MODE_RMW_ABSOLUTE           24      /*  LSR abs */
#define MODE_RMW_ABSOLUTE_X         25      /*  LSR abs,x */
#define MODE_NOP_5C                 26      /*! NOP ($5C) */
#define MODE_INT_WAIT_STOP          27      /*  WAI, STP */

#define MODE_STACK_PUSH             28      /*  PHA */
#define MODE_STACK_PULL             29      /*  PLA */
#define MODE_STACK_BRK              30      /*! BRK # */
#define MODE_STACK_RTI              31      /*! RTI */

/* all possible values for cpu->oper. note that for
   MODE_ZEROPAGE_BIT and MODE_RELATIVE_BIT, the value is always 0-7 (bit)
                                                                + 8 (S/R) */
/* NOP */
#define OPER_NOP                    0

/* read/store instrs */
/* NOP = 0 */
#define OPER_AND                    1
#define OPER_EOR                    2
#define OPER_ORA                    3
#define OPER_ADC                    4
#define OPER_SBC                    5
#define OPER_CMP                    6
#define OPER_CPX                    7
#define OPER_CPY                    8
#define OPER_BIT                    9
#define OPER_LDA                    10
#define OPER_LDX                    11
#define OPER_LDY                    12
#define OPER_STZ                    13
#define OPER_STA                    14
#define OPER_STX                    15
#define OPER_STY                    16

/* RMW instrs */
/* NOP = 0 */
#define OPER_DEC                    1  /* RMW, A, X, Y */
#define OPER_INC                    2  /* RMW, A, X, Y */
#define OPER_ASL                    3  /* RMW, A */
#define OPER_ROL                    4  /* RMW, A */
#define OPER_LSR                    5  /* RMW, A */
#define OPER_ROR                    6  /* RMW, A */
#define OPER_TSB                    7  /* RMW */
#define OPER_TRB                    8  /* RMW */
/* implied instrs */
#define OPER_CLV                    7  /*      impl */
#define OPER_CLC                    8  /*      impl */
#define OPER_SEC                    9  /*      impl */
#define OPER_CLI                    10 /*      impl */
#define OPER_SEI                    11 /*      impl */
#define OPER_CLD                    12 /*      impl */
#define OPER_SED                    13 /*      impl */
#define OPER_TAX                    14 /*      impl */
#define OPER_TXA                    15 /*      impl */
#define OPER_TAY                    16 /*      impl */
#define OPER_TYA                    17 /*      impl */
#define OPER_TSX                    18 /*      impl */
#define OPER_TXS                    19 /*      impl */

/* branch instrs. no NOPs use this mode, so 0 is ok */
#define OPER_BRA                    0
#define OPER_BPL                    1
#define OPER_BMI                    2
#define OPER_BVC                    3
#define OPER_BVS                    4
#define OPER_BCC                    5
#define OPER_BCS                    6
#define OPER_BNE                    7
#define OPER_BEQ                    8

/* stack instrs. no NOPs use this mode, so 0 is ok */
#define OPER_PHP                    0
#define OPER_PHA                    1
#define OPER_PHX                    2
#define OPER_PHY                    3
#define OPER_PLP                    0
#define OPER_PLA                    1
#define OPER_PLX                    2
#define OPER_PLY                    3

/* wait/stop instrs. no NOPs use this mode, so 0 is ok */
#define OPER_WAI                    0
#define OPER_STP                    1

/* ! (only instructions for their modes, set them as 0) */
#define OPER_JMP                    0
#define OPER_JSR                    0
#define OPER_RTS                    0
#define OPER_BRK                    0
#define OPER_RTI                    0

INTERNAL void w65c02si_decode(struct w65c02s_cpu *cpu, uint8_t opcode);

#endif /* W65C02SCE_DECODE_H */
