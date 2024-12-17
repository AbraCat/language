#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>

#include <common.h>
#include <error.h>

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

struct Label
{
    int adr;
    char* name;
};

struct LabelArray
{
    struct Label* labels;
    int n_labels, max_labels;
    char* name_buf;
};

struct Asm
{
    char *prog_text;
    int *code;
    int ip, prog_text_pos, arg1, arg2;
    LabelArray la, ft;
};

ErrEnum getCmdIndex(const char* cmd_name, int* index);

void clearComments(char* str);

void getRegNum(char* str_name, int* num);
ErrEnum getArg(Asm*);

ErrEnum asmCtor(Asm* ase);
void asmDtor(Asm* ase);
ErrEnum runAsm(FILE* fin, FILE* fout);


ErrEnum labelArrayCtor(LabelArray* la);
void labelArrayDtor(LabelArray* la);

void addLabel(LabelArray* la, int adr, char* name);
void getLabelAdr(LabelArray* la, char* name, int* adr);
ErrEnum fixup(int* code, LabelArray* ft, LabelArray* la);


#endif // ASSEMBLER_H