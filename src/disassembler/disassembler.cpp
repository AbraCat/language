#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <disassembler.h>
#include <common.h>
#include <error.h>

ErrEnum disasmCtor(Disasm* dis)
{
    dis->ip = 0;
    dis->code = NULL;
    return ERR_OK;
}

void disasmDtor(Disasm* dis)
{
    free(dis->code);
}

ErrEnum printComplexArg(Disasm* dis, FILE* fout)
{
    /*
    push:
    ok:    001 010 011 101 110 111
    wrong: 000 100

    pop:
    ok:    010 101 110 111
    wrong: 000 001 011 100

    001 011     - arg1 (used for push)
    010         - reg
    101 110 111 - ram
    */

    int argt = dis->code[dis->ip - 1] & MASK_ARGT, mem = 0;

    if ((argt & (IMM_BIT | REG_BIT | MEM_BIT)) == 0 || 
        (argt & (IMM_BIT | REG_BIT | MEM_BIT)) == MEM_BIT)
        return ERR_INSTR_ARG_FMT;

    if (argt & MEM_BIT)
    {
        mem = 1;
        putc('[', fout);
    }
    if (argt & REG_BIT)
        returnErr(printRegName((RegEnum)(dis->code[dis->ip++]), fout));
    if ((argt & REG_BIT) && (argt & IMM_BIT))
        putc('+', fout);
    if (argt & IMM_BIT)
        fprintf(fout, "%d", dis->code[dis->ip++]);
    if (mem)
        putc(']', fout);

    return ERR_OK;
}

ErrEnum runDisasm(FILE* fin, FILE* fout)
{
    #define DISASM_CASE(command)        \
        case CMD_ ## command:           \
            fputs(#command "\n", fout); \
            break;

    #define DISASM_CASE_ARG(command)        \
        case CMD_ ## command:               \
            fprintf(fout, #command " %d\n", \
            dis.code[dis.ip++]);            \
            break;

    #define DISASM_CASE_COMPLEX_ARG(command)      \
        case CMD_ ## command:                     \
            fputs(#command " ", fout);            \
            printComplexArg(&dis, fout);          \
            putc('\n', fout);                     \
            break;

    myAssert(fin != NULL && fout != NULL);

    Disasm dis = {};
    returnErr(disasmCtor(&dis));
    
    int n_cmds = 0;
    returnErr(readCode(fin, &dis.code, &n_cmds));

    while (dis.ip < n_cmds)
    {
        switch (dis.code[dis.ip++] & MASK_CMD)
        {
            DISASM_CASE(HLT)
            DISASM_CASE(IN)
            DISASM_CASE(OUT)
            DISASM_CASE(ADD)
            DISASM_CASE(SUB)
            DISASM_CASE(MUL)
            DISASM_CASE(DIV)
            DISASM_CASE(DUMP)
            DISASM_CASE(RET)
            DISASM_CASE(DRAW)
            DISASM_CASE(SQRT)
            DISASM_CASE(POW)
            DISASM_CASE(XOR)
            DISASM_CASE(SIZE)

            DISASM_CASE_ARG(JMP)
            DISASM_CASE_ARG(JB)
            DISASM_CASE_ARG(JBE)
            DISASM_CASE_ARG(JA)
            DISASM_CASE_ARG(JAE)
            DISASM_CASE_ARG(JE)
            DISASM_CASE_ARG(JNE)
            DISASM_CASE_ARG(CALL)

            DISASM_CASE_COMPLEX_ARG(PUSH)
            DISASM_CASE_COMPLEX_ARG(POP)
            
            default:
                disasmDtor(&dis);
                return ERR_INVAL_INSTR;
        }
    }

    disasmDtor(&dis);
    return ERR_OK;

    #undef DISASM_CASE
    #undef DISASM_CASE_ARG
}