#ifndef main_H
#define main_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <global.h>
#include <assembly.h>

void clearCPU(void);
void clearMem(void);
int memcheck(void);
void dumpMem(void);
void resetFlags(void);
int main(void);

#endif
