#include <assembly.h>

#define NOOP
#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define leftBit(i)  (bit(i & BIT7))
#define rightBit(i) (bit(i & BIT0))

//absolute from rom
static u16 rread16(void){
    u16 ret = memory->PGROM[programCounter++] << 0;
    ret |= memory->PGROM[programCounter++] << 8;
    return ret;
}

static u8 rread8(void){
    return memory->PGROM[programCounter++];
}

//absolute from ram
static u16 aread16(u16 loc){
    u16 ret = memory->RAM[loc++] << 0;
    ret |= memory->RAM[loc] << 8;
    return ret;
}

static u8 aread8(u16 loc){
    return memory->RAM[loc];
}

//zero-page absolute from ram
static u16 areadz16(u8 loc){
    u16 ret = memory->zeroPage[loc++] << 0;
    ret |= memory->zeroPage[loc] << 8;
    return ret;
}

static u8 areadz8(u8 loc){
    return memory->zeroPage[loc];
}

//In this mode a zer0-page address is added to the contents of the X-register
//  to give the address of the bytes holding the address of the operand.
//  eg.  LDA ($3E, X)
//       $A1 $3E
//Assume the following -        byte      value
//                                X-reg.    $05
//                                $0043     $15
//                                $0044     $24
//                                $2415     $6E
//
//  Then the instruction is executed by:
//  (i)   adding $3E and $05 = $0043
//  (ii)  getting address contained in bytes $0043, $0044 = $2415
//  (iii) loading contents of $2415 - i.e. $6E - into accumulator
static u16 preii(u8 off){
    return areadz16(rX + off); //the possible overflow is not a bug
}

//In this mode the contents of a zero-page address (and the following byte)
//  give the indirect addressm which is added to the contents of the Y-register
//  to yield the actual address of the operand.
//  eg.  LDA ($4C), Y
//  Assume the following -        byte       value
//                                $004C      $00
//                                $004D      $21
//                                Y-reg.     $05
//                                $2105      $6D
//  Then the instruction above executes by:
//  (i)   getting the address in bytes $4C, $4D = $2100
//  (ii)  adding the contents of the Y-register = $2105
//  (111) loading the contents of the byte $2105 - i.e. $6D into the
//        accumulator.
//  Note: only the Y-register is used in this mode.
static u16 postii(u8 off){
    return areadz16(off) + rY;
}

static void jmp(u8 diff){
    if(diff & BIT7){ //treat as negative
        programCounter -= diff ^ BIT7;
    }else{
        programCounter += diff;
    }
}

static void stackPush16(u16 i){
    memory->stack[stackPointer++] = i;
    memory->stack[stackPointer++] = i >> 8;
}

static void stackPush8(u8 i){
    memory->stack[stackPointer++] = i;
}

static u16 stackPull16(void){
    u16 ret = memory->stack[--stackPointer];
    ret |= memory->stack[--stackPointer] << 8;
    return ret;
}

static u8 stackPull8(void){
    return memory->stack[--stackPointer];
}

static u8 ASL_helper(u8 t){
   statusRegister.C = leftBit(t);
   statusRegister.Z = zbit(t <<= 1);
   return t;
}

static u8 LSR_helper(u8 t){
    statusRegister.C = rightBit(t);
    statusRegister.Z = zbit(t >>= 1);
    return t;
}

static u8 t8, tt8, ttt8; //temp
static u16 t16, tt16, ttt16; //temp
void execute(void){
    (void) (t8 + tt8 + ttt8);
    (void) (t16 + tt16 + ttt16);

    //Move forward in rom and execute command
    switch(memory->PGROM[programCounter++]){

    //TODO
    //ADC
    //

    //What does N do?
    //AND section, A /\ M -> A     Z/ N/

#define X(cs, reg, and) case cs: \
        reg &= and; \
        statusRegister.Z = zbit(reg); \
    break;

    X(0x29, rA, rread8())

    X(0x25, rA, areadz8(rread8()));
    X(0x35, rX, areadz8(rread8()));

    X(0x2D, rA, aread8(rread16()));
    X(0x3D, rX, aread8(rread16()));
    X(0x39, rY, aread8(rread16()));

    X(0x21, rX, aread8(preii(rread8())));
    X(0x31, rY, aread8(postii(rread8())));

#undef X

    //Still no idea about N
    //ASL section, C <- Q <- 0   Z/ C/ N/
    case 0x0A: statusRegister.C = leftBit(rA); rA <<= 1; statusRegister.Z = zbit(rA); break;

    case 0x06: t8  = rread8();             tt8 = areadz8(t8); memory->zeroPage[t8] = ASL_helper(tt8); break;
    case 0x16: t8  = rX + rread8();        tt8 = areadz8(t8); memory->zeroPage[t8] = ASL_helper(tt8); break;
    case 0x0E: t16 = rread16();            t8  = aread8(t16); memory->RAM[t16]     = ASL_helper(t8); break;
    case 0x1E: t16 = (u16) rX + rread16(); t8  = aread8(t16); memory->RAM[t16]     = ASL_helper(t8); break;

    //Conditional branching
#define X(cs, check) case cs: \
        if(check) jmp(rread8()); \
    break;

    X(0x90, !statusRegister.C) //BCC, branch if C = 0
    X(0xB0,  statusRegister.C) //BCS, branch if C = 1
    X(0xF0, !statusRegister.Z) //BNE, branch if Z = 0
    X(0x30,  statusRegister.Z) //BEQ, branch if Z = 1
    X(0xD0, !statusRegister.N) //BMI, branch if N = 0
    X(0x10,  statusRegister.N) //BPL, branch if N = 1
    X(0x50, !statusRegister.V) //BVC, branch if V = 0
    X(0x70,  statusRegister.V) //BVS, branch if V = 1
#undef X

    //BIT, A /\ M, M7 -> N, M6 -> V   Z/
    case 0x24:
        t8 = rA & areadz8(rread8());
        statusRegister.N = bit(t8 & BIT7);
        statusRegister.V = bit(t8 & BIT6);
        statusRegister.Z = zbit(t8);
    break;
    case 0x2C:
        t8 = rA & aread8(rread16());
        statusRegister.N = bit(t8 & BIT7);
        statusRegister.V = bit(t8 & BIT6);
        statusRegister.Z = zbit(t8);
    break;

    //BRK, force break
    case 0x00:
        stackPush16(programCounter + 2);
        stackPush8(statusRegister.byte);
        setBit(statusRegister.I);
    break;

    case 0x18: clearBit(statusRegister.C); break; //CLC
    case 0xD8: clearBit(statusRegister.D); break; //CLD
    case 0x58: clearBit(statusRegister.I); break; //CLI
    case 0xB8: clearBit(statusRegister.V); break; //CLV

    //None of the registers work
    //I dont even know how do do this
    //CMP + CPX + CPY

#define X(cs, data, cmpto) case cs: \
        t8 = data - cmpto; \
    break;

    X(0xC9, rA, rread8());
    X(0xC5, rA, areadz8(rread8()));
    X(0xD5, rA, areadz8(rread8() + rX));
    X(0xCD, rA, aread8(rread16()));
    X(0xDD, rA, aread8(rread16() + rX));
    X(0xD9, rA, aread8(rread16() + rY));
    X(0xC1, rA, areadz8(preii(rread8())));
    X(0xD1, rA, areadz8(postii(rread8())));

    X(0xE0, rX, rread8());
    X(0xE4, rX, areadz8(rread8()));
    X(0xEC, rX, aread8(rread16()));

    X(0xC0, rY, rread8());
    X(0xC4, rY, areadz8(rread8()));
    X(0xCC, rY, aread8(rread16()));
#undef X


    //what is N
    //EOR A xor M -> A  N/ Z/
#define X(cs, op) case cs: \
        statusRegister.Z = zbit( rA = rA ^ (op)); \
    break;

    X(0x49, rread8());
    X(0x45, areadz8(rread8()));
    X(0x55, areadz8(rread8() + rX));
    X(0x40, aread8(rread16()));
    X(0x5D, aread8(rread16() + rX));
    X(0x59, aread8(rread16() + rY));
    X(0x41, aread8(preii(rread8())));
    X(0x51, aread8(postii(rread8())));
#undef X

    //N register not working
    //DEC + INC + DEX + DEY + INY + INX M +- 1 -> M   N/ Z/
#define X(cs, op, mem, offset) case cs: \
        statusRegister.Z = zbit(op mem[offset]); \
    break;

    X(0xC6, --, memory->zeroPage, rread8());
    X(0xD6, --, memory->zeroPage, rread8() + rX);
    X(0xCE, --, memory->RAM, rread16());
    X(0xDE, --, memory->RAM, rread16() + rX);

    X(0xE6, ++, memory->zeroPage, rread8());
    X(0xF6, ++, memory->zeroPage, rread8() + rX);
    X(0xEE, ++, memory->RAM, rread16());
    X(0xFE, ++, memory->RAM, rread16() + rX);
#undef X

#define X(cs, op) case cs: \
        statusRegister.Z = zbit(op); \
    break;\

    X(0xCA, --rX) //DEX
    X(0x88, --rY) //DEY
    X(0xE8, ++rX) //INX
    X(0xC8, ++rY) //INY
#undef X


    //JMP
    case 0x4C: programCounter = rread16(); break;
    case 0x6C: programCounter = aread16(rread16()); break;

    //JSR
    case 0x20:
        stackPush16(programCounter + 2);
        programCounter = rread16();
    break;

    //LDA, LDX, LDY M -> A   N/ Z/
#define X(cs, reg, op) case cs: \
        statusRegister.Z = zbit(reg = (op)); \
    break;

    X(0xA9, rA, rread8());
    X(0xA5, rA, areadz8(rread8()));
    X(0xB5, rA, areadz8(rread8() + rX));
    X(0xAD, rA, aread8(rread16()));
    X(0xBD, rA, aread8(rread16() + rX));
    X(0xB9, rA, aread8(rread16() + rY));
    X(0xA1, rA, aread8(preii(rread8())));
    X(0xB1, rA, aread8(postii(rread8())));

    X(0xA2, rX, rread8());
    X(0xA6, rX, areadz8(rread8()));
    X(0xB6, rX, areadz8(rread8() + rY));
    X(0xAE, rX, aread8(rread16()));
    X(0xBE, rX, aread8(rread16() + rY));

    X(0xA0, rY, rread8());
    X(0xA4, rY, areadz8(rread8()));
    X(0xB4, rY, areadz8(rread8() + rX));
    X(0xAC, rY, aread8(rread16()));
    X(0xBC, rY, aread8(rread16() + rX));
#undef X

    //LSR
    case 0x4A:
        clearBit(statusRegister.N);
        statusRegister.C = rightBit(rA);
        rA >>= 1;
        statusRegister.Z = zbit(rA >>= 1);
    break;

    case 0x46: clearBit(statusRegister.N); t8  = rread8();             tt8 = areadz8(t8); memory->zeroPage[t8] = LSR_helper(tt8); break;
    case 0x56: clearBit(statusRegister.N); t8  = rX + rread8();        tt8 = areadz8(t8); memory->zeroPage[t8] = LSR_helper(tt8); break;
    case 0x4E: clearBit(statusRegister.N); t16 = rread16();            t8  = aread8(t16); memory->RAM[t16]     = LSR_helper(t8); break;
    case 0x5E: clearBit(statusRegister.N); t16 = (u16) rX + rread16(); t8  = aread8(t16); memory->RAM[t16]     = LSR_helper(t8); break;

    //NOOP
    case 0xEA: NOOP; break;

    //ORA A | M -> A  N/ Z/
#define X(cs, op) case cs: \
        statusRegister.Z = zbit( rA = rA | (op)); \
    break;

    X(0x09, rread8());
    X(0x05, areadz8(rread8()));
    X(0x15, areadz8(rread8() + rX));
    X(0x0D, aread8(rread16()));
    X(0x1D, aread8(rread16() + rX));
    X(0x19, aread8(rread16() + rY));
    X(0x01, aread8(preii(rread8())));
    X(0x11, aread8(postii(rread8())));
#undef X

    case 0x48: stackPush8(rA); break; //PHA
    case 0x08: stackPush8(statusRegister.byte); break; //PHP
    case 0x68: rA = stackPull8(); break; //PLA
    case 0x28: statusRegister.byte = stackPull8(); break; //PLP

    //TODO
    //ROR, ROL    N/ Z/ C/
    //

    //RTI + RTS
    case 0x4D:
        statusRegister.byte = stackPull8();
        programCounter = stackPull16();
    break;
    case 0x60:
        programCounter = stackPull16() + 1;
    break;

    //TODO
    //SBC
    //

    case 0x38: setBit(statusRegister.C); break; //SEC
    case 0xF8: setBit(statusRegister.D); break; //SED
    case 0x78: setBit(statusRegister.I); break; //SEI
    /*case 0x: setBit(statusRegister.V); break; //SEV*/

    //STA + STX + STY
#define X(cs, reg, location) case cs: \
        location = reg;
    break;

    X(0x85, rA, memory->zeroPage[rread8()]);
    X(0x95, rA, memory->zeroPage[rread8() + rX]);
    X(0x80, rA, memory->RAM[rread16()]);
    X(0x90, rA, memory->RAM[rread16() + rX]);
    X(0x99, rA, memory->RAM[rread16() + rY]);
    X(0x81, rA, memory->RAM[preii(rread8())]);
    X(0x91, rA, memory->RAM[postii(rread8())]);

    X(0x86, rX, memory->zeroPage[rread8()]);
    X(0x96, rX, memory->zeroPage[rread8() + rY]);
    X(0x8E, rX, memory->RAM[rread16()]);

    X(0x84, rY, memory->zeroPage[rread8()]);
    X(0x94, rY, memory->zeroPage[rread8() + rX]);
    X(0x8C, rY, memory->RAM[rread16()]);
#undef X

    //TAX + TAY + TSX + TXA + TXS + TYA
#define X(cs, from, to) case cs: \
        to = from; \
    break;

    X(0xAA, rA, rX);
    X(0xA8, rA, rY);
    X(0xBA, stackPointer, rX);
    X(0x8A, rX, rA);
    X(0x9A, rX, stackPointer);
    X(0x98, rY, rA);
#undef X

    default:
        printf("MEME\n");
    }
}
