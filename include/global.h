#ifndef global_H
#define global_H

#include <stdint.h>

typedef uint_least8_t  u8;
typedef unsigned char  byte; //byte = octet, whatever
typedef uint_least16_t u16;
typedef uint_least32_t u32;
typedef int_least8_t   s8;

//binary calloc because I cant ever remeber the order
#define bcalloc(size) calloc(size, 1)

#define setBit(addr) (addr = 1)
#define clearBit(addr) (addr = 0)
#define bit(addr) ((addr) ? 1 : 0)
#define zbit(addr) ((addr) ? 0 : 1)

//http://nesdev.com/6502.txt
union statusRegister{
    u8 byte;
    struct{
        u8 C: 1; //Carry
        u8 Z: 1; //Zero flag
        u8 I: 1; //Interrupts allowed
        u8 D: 1; //Decimal mode
        u8 B: 1; //Software Interrupt
        u8 _: 1; //Unused
        u8 V: 1; //Overflow
        u8 N: 1; //Sign flag
    };
};

//https://en.wikibooks.org/wiki/NES_Programming
union memory{
    byte raw[0x10000];        //Emulating 64kb
    struct{
        union{
            byte RAM[0x800];
            struct{
                byte zeroPage[0x100];       //0000-00FF
                byte stack[0x100];          //0100-01FF
                byte GRAM[0x600];           //0200-07FF
            } __attribute__ ((__packed__));
        };
        byte RAM1[0x800];           //General purpose ram mirroring 0000-07FF
        byte RAM2[0x800];           //"
        byte RAM3[0x800];           //"

        byte IO[0x2000];            //PPU registers 2000-2007, PPU RAM to 3FFF

        byte APUIO[0x18];           //4000-4017
        byte EROM[0x1FE8];          //4020-5FFF
        byte SWRAM[0x2000];         //6000-7FFF
        byte PGROM[0x8000];         //8000-FFFF
    } __attribute__ ((__packed__));
};

//All the parts to make a NES
extern u8 rA;
extern u8 rX;
extern u8 rY;
//I assume this is a u16 as the memory is 64kb
//  but I never read anything that confirmed this
extern u16 programCounter;
extern u8 stackPointer;
extern union statusRegister statusRegister;
extern union memory *memory;

#endif
