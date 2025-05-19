#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>

#include <common.h>
#include <error.h>
#include <label.h>

enum CmdType
{
    CMDT_NO_ARG,
    CMDT_INT_ARG,
    CMDT_COMPLEX_ARG,
    CMDT_LABEL_ARG,
};

struct Cmd
{
    CmdCode code;
    CmdType type;
    const char* name;
    int name_len;
};

struct Asm
{
    char *prog_text;
    int *code;
    int ip, prog_text_pos, arg1, arg2;
    LabelArray *la, *ft;
};

ErrEnum getCmdIndex(const char* cmd_name, int* index);

void getRegNum(char* str_name, int* num);
ErrEnum getArg(Asm*);

ErrEnum asmCtor(Asm* ase);
void asmDtor(Asm* ase);
ErrEnum runAsm(FILE* fin, FILE* fout);


#endif // ASSEMBLER_H