#include <main.h>

u8 rA;
u8 rX;
u8 rY;
u16 programCounter;
u8 stackPointer;
union statusRegister statusRegister;
union memory *memory;

void clearCPU(void){
    rA = 0;
    rX = 0;
    rY = 0;
    programCounter = 0;
    stackPointer = 0;
    resetFlags();
    memset(memory, 0, 0x800);
}

void clearMem(void){
    memset(memory, 0, 0x8000);
}

void resetFlags(void){
    statusRegister.byte = 0;
    statusRegister._ = 1;
}

int memcheck(void){
    //Make sure the compiler did not pad the memory or mangle the structs

    printf("Checking status of some register bits\n");
    statusRegister.byte = 0x1;
    if(!statusRegister.C) return 0;
    statusRegister.byte = 0x4;
    if(!statusRegister.I) return 0;
    statusRegister.byte = 0x80;
    if(!statusRegister.N) return 0;

    resetFlags();

    printf("Checking status of some register raw\n");
    statusRegister.C = 1;
    statusRegister.I = 1;
    statusRegister.B = 1;
    statusRegister.N = 1;
    if(statusRegister.byte != 0xb5) return 0;

    printf("Checking RAM offsets\n");

    if(memory->APUIO - memory->raw != 0x4000) return 0;
    if(memory->PGROM - memory->raw != 0x8000) return 0;

    //This one isnt very important
    if(sizeof(union memory) != 0x10000) return 0;

    printf("Passed all compile tests\n");
    resetFlags();

    return 1;
}

void dumpMem(void){
    FILE *f = fopen("mem.bin", "wb");
    fwrite(memory, 1, sizeof(union memory), f);
    fclose(f);
}

int main(void){
    memory = bcalloc(sizeof(union memory));
    memcheck();
    execute();
    return 0;
}
