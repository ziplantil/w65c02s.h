/*******************************************************************************
            w65c02sce -- cycle-accurate C emulator of the WDC 65C02S
            by ziplantil 2022 -- under the CC0 license
            version: 2022-10-14

            decode.c - instruction decoder
*******************************************************************************/

#define W65C02SCE
#include "w65c02s.h"
#if W65C02SCE_SEPARATE

#include "decode.h"

#define imp MODE_IMPLIED
#define imm MODE_IMMEDIATE
#define rel MODE_RELATIVE
#define rlb MODE_RELATIVE_BIT
#define zpg MODE_ZEROPAGE
#define zpx MODE_ZEROPAGE_X
#define zpy MODE_ZEROPAGE_Y
#define zpb MODE_ZEROPAGE_BIT
#define abs MODE_ABSOLUTE
#define abx MODE_ABSOLUTE_X
#define aby MODE_ABSOLUTE_Y
#define zpi MODE_ZEROPAGE_INDIRECT
#define zix MODE_ZEROPAGE_INDIRECT_X
#define ziy MODE_ZEROPAGE_INDIRECT_Y
#define ind MODE_ABSOLUTE_INDIRECT
#define idx MODE_ABSOLUTE_INDIRECT_X
#define abj MODE_ABSOLUTE_JUMP
#define im1 MODE_IMPLIED_1C
#define imx MODE_IMPLIED_X
#define imy MODE_IMPLIED_Y
#define wzp MODE_RMW_ZEROPAGE
#define wzx MODE_RMW_ZEROPAGE_X
#define sbj MODE_SUBROUTINE
#define sbr MODE_RETURN_SUB
#define wab MODE_RMW_ABSOLUTE
#define wax MODE_RMW_ABSOLUTE_X
#define wai MODE_INT_WAIT
#define stp MODE_INT_STOP
#define phv MODE_STACK_PUSH
#define plv MODE_STACK_PULL
#define brk MODE_STACK_BRK
#define rti MODE_STACK_RTI

int modes[256] = {
            /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  */
    /*00:0F*/ brk,zix,imm,im1,wzp,zpg,wzp,zpb,phv,imm,imp,im1,wab,abs,wab,rlb,
    /*10:1F*/ rel,ziy,zpi,im1,wzp,zpx,wzx,zpb,imp,aby,imp,im1,wab,abx,wax,rlb,
    /*20:2F*/ sbj,zix,imm,im1,zpg,zpg,wzp,zpb,plv,imm,imp,im1,abs,abs,wab,rlb,
    /*30:3F*/ rel,ziy,zpi,im1,zpx,zpx,wzx,zpb,imp,aby,imp,im1,abx,abx,wax,rlb,
    /*40:4F*/ rti,zix,imm,im1,zpg,zpg,wzp,zpb,phv,imm,imp,im1,abj,abs,wab,rlb,
    /*50:5F*/ rel,ziy,zpi,im1,zpx,zpx,wzx,zpb,imp,aby,phv,im1,abs,abx,wax,rlb,
    /*60:6F*/ sbr,zix,imm,im1,zpg,zpg,wzp,zpb,plv,imm,imp,im1,ind,abs,wab,rlb,
    /*70:7F*/ rel,ziy,zpi,im1,zpx,zpx,wzx,zpb,imp,aby,plv,im1,idx,abx,wax,rlb,
    /*80:8F*/ rel,zix,imm,im1,zpg,zpg,zpg,zpb,imy,imm,imp,im1,abs,abs,abs,rlb,
    /*90:9F*/ rel,ziy,zpi,im1,zpx,zpx,zpy,zpb,imp,aby,imp,im1,abs,abx,abx,rlb,
    /*A0:AF*/ imm,zix,imm,im1,zpg,zpg,zpg,zpb,imp,imm,imp,im1,abs,abs,abs,rlb,
    /*B0:BF*/ rel,ziy,zpi,im1,zpx,zpx,zpy,zpb,imp,aby,imp,im1,abx,abx,aby,rlb,
    /*C0:CF*/ imm,zix,imm,im1,zpg,zpg,wzp,zpb,imy,imm,imx,wai,abs,abs,wab,rlb,
    /*D0:DF*/ rel,ziy,zpi,im1,zpx,zpx,wzx,zpb,imp,aby,phv,stp,abs,abx,wax,rlb,
    /*E0:EF*/ imm,zix,imm,im1,zpg,zpg,wzp,zpb,imx,imm,imp,im1,abs,abs,wab,rlb,
    /*F0:FF*/ rel,ziy,zpi,im1,zpx,zpx,wzx,zpb,imp,aby,plv,im1,abs,abx,wax,rlb,
};

#define BRA OPER_ONLY
#define JSR OPER_ONLY
#define RTS OPER_ONLY
#define WAI OPER_ONLY
#define STP OPER_ONLY
#define BRK OPER_ONLY
#define RTI OPER_ONLY

#define LDA OPER_LDA
#define LDX OPER_LDX
#define LDY OPER_LDY
#define STZ OPER_STZ
#define STA OPER_STA
#define STX OPER_STX
#define STY OPER_STY

#define ORA OPER_ORA
#define AND OPER_AND
#define EOR OPER_EOR
#define ADC OPER_ADC
#define CPX OPER_CPX
#define CPY OPER_CPY
#define CMP OPER_CMP
#define SBC OPER_SBC

#define ASL OPER_ASL
#define ROL OPER_ROL
#define LSR OPER_LSR
#define ROR OPER_ROR
#define BIT OPER_BIT
#define JMP OPER_JMP
#define DEC OPER_DEC
#define INC OPER_INC

#define BPL OPER_BPL
#define BMI OPER_BMI
#define BVC OPER_BVC
#define BVS OPER_BVS
#define BCC OPER_BCC
#define BCS OPER_BCS
#define BNE OPER_BNE
#define BEQ OPER_BEQ

#define PHA OPER_PHA
#define PHX OPER_PHX
#define PHY OPER_PHY
#define PHP OPER_PHP
#define PLA OPER_PHA
#define PLX OPER_PHX
#define PLY OPER_PHY
#define PLP OPER_PHP
#define TSB OPER_TSB
#define TRB OPER_TRB

#define NOP OPER_NOP
#define CLV OPER_CLV
#define CLC OPER_CLC
#define SEC OPER_SEC
#define CLI OPER_CLI
#define SEI OPER_SEI
#define CLD OPER_CLD
#define SED OPER_SED
#define TAX OPER_TAX
#define TXA OPER_TXA
#define TAY OPER_TAY
#define TYA OPER_TYA
#define TSX OPER_TSX
#define TXS OPER_TXS
#define JMP OPER_JMP

int opers[256] = {
            /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  */
    /*00:0F*/ BRK,ORA,NOP,NOP,TSB,ORA,ASL,000,PHP,ORA,ASL,NOP,TSB,ORA,ASL,000,
    /*10:1F*/ BPL,ORA,ORA,NOP,TRB,ORA,ASL,001,CLC,ORA,INC,NOP,TRB,ORA,ASL,001,
    /*20:2F*/ JSR,AND,NOP,NOP,BIT,AND,ROL,002,PLP,AND,ROL,NOP,BIT,AND,ROL,002,
    /*30:3F*/ BMI,AND,AND,NOP,BIT,AND,ROL,003,SEC,AND,DEC,NOP,BIT,AND,ROL,003,
    /*40:4F*/ RTI,EOR,NOP,NOP,NOP,EOR,LSR,004,PHA,EOR,LSR,NOP,JMP,EOR,LSR,004,
    /*50:5F*/ BVC,EOR,EOR,NOP,NOP,EOR,LSR,005,CLI,EOR,PHY,NOP,NOP,EOR,LSR,005,
    /*60:6F*/ RTS,ADC,NOP,NOP,STZ,ADC,ROR,006,PLA,ADC,ROR,NOP,JMP,ADC,ROR,006,
    /*70:7F*/ BVS,ADC,ADC,NOP,STZ,ADC,ROR,007,SEI,ADC,PLY,NOP,JMP,ADC,ROR,007,
    /*80:8F*/ BRA,STA,NOP,NOP,STY,STA,STX,010,DEC,BIT,TXA,NOP,STY,STA,STX,010,
    /*90:9F*/ BCC,STA,STA,NOP,STY,STA,STX,011,TYA,STA,TXS,NOP,STZ,STA,STZ,011,
    /*A0:AF*/ LDY,LDA,LDX,NOP,LDY,LDA,LDX,012,TAY,LDA,TAX,NOP,LDY,LDA,LDX,012,
    /*B0:BF*/ BCS,LDA,LDA,NOP,LDY,LDA,LDX,013,CLV,LDA,TSX,NOP,LDY,LDA,LDX,013,
    /*C0:CF*/ CPY,CMP,NOP,NOP,CPY,CMP,DEC,014,INC,CMP,DEC,WAI,CPY,CMP,DEC,014,
    /*D0:DF*/ BNE,CMP,CMP,NOP,NOP,CMP,DEC,015,CLD,CMP,PHX,STP,NOP,CMP,DEC,015,
    /*E0:EF*/ CPX,SBC,NOP,NOP,CPX,SBC,INC,016,INC,SBC,NOP,NOP,CPX,SBC,INC,016,
    /*F0:FF*/ BEQ,SBC,SBC,NOP,NOP,SBC,INC,017,SED,SBC,PLX,NOP,NOP,SBC,INC,017,
};

INTERNAL void w65c02si_decode(struct w65c02s_cpu *cpu, uint8_t opcode) {
    cpu->mode = modes[opcode];
    cpu->oper = opers[opcode];
    cpu->cycl = 1;
}

#endif /* W65C02SCE_SEPARATE */
