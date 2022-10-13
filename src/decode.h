/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            decode.h - instruction decoder
*******************************************************************************/

#ifndef W65C02SCE_DECODE_H
#define W65C02SCE_DECODE_H

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
#define MODE_INT_WAIT               26      /*! WAI */
#define MODE_INT_STOP               27      /*! STP */

#define MODE_STACK_PUSH             28      /*  PHA */
#define MODE_STACK_PULL             29      /*  PLA */
#define MODE_STACK_BRK              30      /*! BRK # */
#define MODE_STACK_RTI              31      /*! RTI */

/* all possible values for cpu->oper. note that for
   MODE_ZEROPAGE_BIT and MODE_RELATIVE_BIT, the value is always 0-7 (bit)
                                                                + 8 (S/R) */
#define OPER_ONLY                   0       /* !,BRA,JSR,RTS,WAI,STP,BRK,RTI */
#define OPER_LDA                    1
#define OPER_LDX                    2
#define OPER_LDY                    3
#define OPER_STZ                    4
#define OPER_STA                    5
#define OPER_STX                    6
#define OPER_STY                    7

#define OPER_ORA                    8
#define OPER_AND                    9
#define OPER_EOR                    10
#define OPER_ADC                    11
#define OPER_CPX                    12
#define OPER_CPY                    13
#define OPER_CMP                    14
#define OPER_SBC                    15

#define OPER_ASL                    16
#define OPER_ROL                    17
#define OPER_LSR                    18
#define OPER_ROR                    19
#define OPER_BIT                    20
#define OPER_JMP                    21
#define OPER_DEC                    22
#define OPER_INC                    23

#define OPER_BPL                    24
#define OPER_BMI                    25
#define OPER_BVC                    26
#define OPER_BVS                    27
#define OPER_BCC                    28
#define OPER_BCS                    29
#define OPER_BNE                    30
#define OPER_BEQ                    31

#define OPER_PHA                    32      /* PHA, PLA */
#define OPER_PHX                    33      /* PHX, PLX */
#define OPER_PHY                    34      /* PHY, PLY */
#define OPER_PHP                    35      /* PHP, PLP */
#define OPER_TSB                    36
#define OPER_TRB                    37

#define OPER_NOP                    38
#define OPER_CLV                    39
#define OPER_CLC                    40
#define OPER_SEC                    41
#define OPER_CLI                    42
#define OPER_SEI                    43
#define OPER_CLD                    44
#define OPER_SED                    45
#define OPER_TAX                    46
#define OPER_TXA                    47
#define OPER_TAY                    48
#define OPER_TYA                    49
#define OPER_TSX                    50
#define OPER_TXS                    51

INTERNAL void w65c02si_decode(struct w65c02s_cpu *cpu, uint8_t opcode);

#endif /* W65C02SCE_DECODE_H */
