#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdio.h>

#include <common.h>
#include <error.h>

struct Disasm
{
    int *code;
    int ip;
};

ErrEnum disasmCtor(Disasm* dis);
void disasmDtor(Disasm* dis);

ErrEnum printComplexArg(Disasm* dis, FILE* fout);

ErrEnum runDisasm(FILE* fin, FILE* fout);

#endif // DISASSEMBLER_H