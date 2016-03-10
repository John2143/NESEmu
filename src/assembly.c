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
    if(diff & LEFTBIT){ //treat as negative
        programCounter -= diff ^ LEFTBIT;
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

static u16 pullStack16(void){
    u16 ret = memory->stack[--stackPointer];
    ret |= memory->stack[--stackPointer] << 8;
    return ret;
}

static u8 pullStack8(void){
    return memory->stack[--stackPointer];
}

static u8 ASL_helper(u8 t){
   statusRegister.C = leftBit(t);
   statusRegister.Z = zbit(t <<= 1);
   return t;
}

static u8 t8, tt8, ttt8; //temp
static u16 t16, tt16, ttt16; //temp
void execute(void){
    (void) (t8 + tt8 + ttt8);
    (void) (t16 + tt16 + ttt16);

    switch(memory->PGROM[programCounter++]){
    //AND section, A /\ M -> A     Z/ N/
    case 0x29: rA &= rread8(); statusRegister.Z = zbit(rA); break;

    case 0x25: rA &= areadz8(rread8()); statusRegister.Z = zbit(rA); break;
    case 0x35: rX &= areadz8(rread8()); statusRegister.Z = zbit(rX); break;

    case 0x2D: rA &= aread8(rread16()); statusRegister.Z = zbit(rA); break;
    case 0x3D: rX &= aread8(rread16()); statusRegister.Z = zbit(rX); break;
    case 0x39: rY &= aread8(rread16()); statusRegister.Z = zbit(rY); break;

    case 0x21: rX &= aread8(preii(rread8()));  statusRegister.Z = zbit(rX); break;
    case 0x31: rY &= aread8(postii(rread8())); statusRegister.Z = zbit(rY); break;

    //ASL section, C <- Q <- 0
    case 0x0A: statusRegister.C = leftBit(rA); rA <<= 1; statusRegister.Z = zbit(rA); break;

    case 0x06: t8  = rread8();             tt8 = areadz8(t8); memory->zeroPage[t8] = ASL_helper(tt8); break;
    case 0x16: t8  = rX + rread8();        tt8 = areadz8(t8); memory->zeroPage[t8] = ASL_helper(tt8); break;
    case 0x0E: t16 = rread16();            t8  = aread8(t16); memory->RAM[t16]     = ASL_helper(t8); break;
    case 0x1E: t16 = (u16) rX + rread16(); t8  = aread8(t16); memory->RAM[t16]     = ASL_helper(t8); break;

    //BCC, branch if C = 0
    case 0x90: if(!statusRegister.C) jmp(rread8()); break;
    //BCS, branch if C = 1
    case 0xB0: if( statusRegister.C) jmp(rread8()); break;
    //BNE, branch if Z = 0
    case 0xF0: if(!statusRegister.Z) jmp(rread8()); break;
    //BEQ, branch if Z = 1
    case 0x30: if( statusRegister.Z) jmp(rread8()); break;
    //BMI, branch if N = 0
    case 0xD0: if(!statusRegister.N) jmp(rread8()); break;
    //BPL, branch if N = 1
    case 0x10: if( statusRegister.N) jmp(rread8()); break;
    //BVC, branch if V = 0
    case 0x50: if(!statusRegister.V) jmp(rread8()); break;
    //BVS, branch if V = 1
    case 0x70: if( statusRegister.V) jmp(rread8()); break;

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
        stackPush8(statusRegister);
        setBit(statusRegister.I);
    break;

    //CLC
    case 0x18: clearBit(statusRegister.C); break;
    //CLD
    case 0xD8: clearBit(statusRegister.D); break;
    //CLI
    case 0x58: clearBit(statusRegister.I); break;
    //CLV
    case 0xB8: clearBit(statusRegister.V); break;


    }
}
