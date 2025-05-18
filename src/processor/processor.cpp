#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include <processor.h>
#include <common.h>
#include <colors.h>
#include <error.h>
#include <utils.h>

static const int ram_size_sqrt = 20, max_rec_depth = 50;

ErrEnum procCtor(Proc* prc)
{
    stCtor(&prc->st, ram_size_sqrt * ram_size_sqrt);
    stCtor(&prc->ret, max_rec_depth);
    prc->n_cmds = prc->ip = prc->cmd = prc->argt = prc->arg1 = prc->arg2 = 0;
    prc->code = NULL;

    prc->ram = prc->st.data;
    // prc->ram = (int*)calloc(ram_size_sqrt * ram_size_sqrt * 2, sizeof(int));
    // if (prc->ram == NULL) return ERR_MEM;
    // initRam(prc);

    return ERR_OK;
}
void procDtor(Proc* prc)
{
    stDtor(&prc->st);
    stDtor(&prc->ret);
    free(prc->code);
    // free(prc->ram);
}

ErrEnum getPopDestination(Proc* prc, int** dest, int write)
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

    if ((prc->argt & (IMM_BIT | REG_BIT | MEM_BIT)) == 0 || 
        (prc->argt & (IMM_BIT | REG_BIT | MEM_BIT)) == MEM_BIT)
    {
        *dest = NULL;
        return ERR_INSTR_ARG_FMT;
    }

    if ((prc->argt & (IMM_BIT | REG_BIT | MEM_BIT)) == REG_BIT)
    {
        *dest = prc->reg + prc->code[prc->ip++];
        return ERR_OK;
    }

    int argv = 0;
    if (prc->argt & REG_BIT)
        argv += prc->reg[prc->code[prc->ip++]];
    if (prc->argt & IMM_BIT)
        argv += prc->code[prc->ip++];
    if (prc->argt & MEM_BIT)
    {
        *dest = prc->ram + argv;
        return ERR_OK;
    }

    if (write) return ERR_INSTR_ARG_FMT;

    prc->arg1 = argv;
    *dest = &prc->arg1;
    return ERR_OK;
}

ErrEnum runProcFile(FILE* fcode, FILE* fin, FILE* fout)
{
    myAssert(fout != NULL && fin != NULL && fout != NULL);

    struct Proc prc = {};
    returnErr(procCtor(&prc));
    returnErr(readCode(fcode, &prc.code, &prc.n_cmds));
    returnErr(runProc(&prc, fin, fout));
    procDtor(&prc);
    
    return ERR_OK;
}

ErrEnum runProc(Proc* prc, FILE* fin, FILE* fout)
{
    myAssert(prc != NULL);

    int* code = prc->code;

    while (1)
    {
        if (prc->ip < 0 || prc->ip >= prc->n_cmds)
            return ERR_IP_BOUND;

        prc->cmd = code[prc->ip] & MASK_CMD;
        prc->argt = code[prc->ip++] & MASK_ARGT;
        switch(prc->cmd)
        {
            case CMD_HLT:
                return ERR_OK;
            case CMD_IN:
                if (fscanf(fin, "%d", &prc->arg1) != 1) return ERR_INPUT;
                returnErr(stPush(&prc->st, prc->arg1));
                break;
            case CMD_OUT:
                returnErr(stPop(&prc->st, &prc->arg1));
                fprintf(fout, "%d\n", prc->arg1);
                break;

            #define CASE_ARITHM_OP(cmd, op)                              \
                case CMD_ ## cmd:                                        \
                    returnErr(stPop(&prc->st, &prc->arg1));              \
                    returnErr(stPop(&prc->st, &prc->arg2));              \
                    if (CMD_ ## cmd == CMD_DIV && prc->arg1 == 0)        \
                        {printf("%d %d\n", prc->arg1, prc->arg2); return ERR_DIV_NULL;}                             \
                    returnErr(stPush(&prc->st, prc->arg2 op prc->arg1)); \
                    break;

            CASE_ARITHM_OP(ADD, +)
            CASE_ARITHM_OP(SUB, -)
            CASE_ARITHM_OP(MUL, *)
            CASE_ARITHM_OP(DIV, /)
            CASE_ARITHM_OP(XOR, ^)

            case CMD_SQRT:                                     
                returnErr(stPop(&prc->st, &prc->arg1));     
                if (prc->arg1 < 0) return ERR_SQRT_NEGATIVE;            
                returnErr(stPush(&prc->st, (int)sqrt(prc->arg1)));
                break;
            case CMD_POW:
                returnErr(stPop(&prc->st, &prc->arg1));
                returnErr(stPop(&prc->st, &prc->arg2));
                if (prc->arg1 < 0) return ERR_POW_NEGATIVE;
                returnErr(stPush(&prc->st, intPow(prc->arg2, prc->arg1)));
                break;
            case CMD_SIZE:
                returnErr(stPush(&prc->st, prc->st.size));
                break;
            case CMD_DUMP:
                prcDump(fout, prc);
                break;
            case CMD_PUSH:
            {
                int *arg = NULL;
                returnErr(getPopDestination(prc, &arg, 0));
                returnErr(stPush(&prc->st, *arg));
                break;
            }
            case CMD_POP:
            {
                int *arg = NULL;
                returnErr(getPopDestination(prc, &arg, 1));
                returnErr(stPop(&prc->st, arg));
                break;
            }
            case CMD_DRAW:
                returnErr(drawRam(fout, prc));
                break;
            case CMD_JMP:
                prc->ip = code[prc->ip];
                break;

            #define CASE_JMP(jmp, op)                     \
                case CMD_ ## jmp:                         \
                    returnErr(stPop(&prc->st, &prc->arg2)); \
                    returnErr(stPop(&prc->st, &prc->arg1)); \
                    if (prc->arg1 op prc->arg2)             \
                        prc->ip = code[prc->ip];            \
                    else                                  \
                        ++prc->ip;                         \
                    break;

            CASE_JMP(JB, <)
            CASE_JMP(JBE, <=)
            CASE_JMP(JA, >)
            CASE_JMP(JAE, >=)
            CASE_JMP(JE, ==)
            CASE_JMP(JNE, !=)
            
            #undef CASE_JMP

            case CMD_CALL:
                returnErr(stPush(&prc->ret, prc->ip + 1));
                prc->ip = code[prc->ip];
                break;
            case CMD_RET:
                returnErr(stPop(&prc->ret, &prc->ip));
                break;
            default:
                fprintf(fout, "Invalid instruction: %d ip: %d\n", prc->code[prc->ip - 1], prc->ip - 1);
                return ERR_INVAL_INSTR;
        }
    }

    // this code should never be reached
    return ERR_OK;
}

void initRam(Proc* prc)
{
    myAssert(prc != NULL);

    int adr = 0;
    for (int i = 0; i < ram_size_sqrt; ++i)
    {
        for (int j = 0; j < ram_size_sqrt; ++j)
        {
            adr = (i * ram_size_sqrt + j);
            prc->ram[adr] = 0;
        }
    }
    return;
}

ErrEnum drawRam(FILE* fout, Proc* prc)
{
    myAssert(prc != NULL && fout != NULL);

    int adr = 0;
    for (int i = 0; i < ram_size_sqrt; ++i)
    {
        for (int j = 0; j < ram_size_sqrt; ++j)
        {
            adr = i * ram_size_sqrt + j;
            fprintf(fout, "%d ", prc->ram[adr]);
        }
        putc('\n', fout);
    }

    return ERR_OK;
}

void prcDump(FILE* fout, Proc* prc)
{
    myAssert(prc != NULL || fout != NULL);

    fprintf(fout, "Processor [0x%p]\n\n", prc);
    fprintf(fout, "ip: %d\n\n", prc->ip);
    for (int reg_num = 0; reg_num < n_regs; ++reg_num)
    {
        printRegName((RegEnum)reg_num, fout);
        fprintf(fout, ": %d\n", prc->reg[reg_num]);
    }
    putc('\n', fout);
    stDump(fout, &prc->st);
    fputs("RAM:\n\n", fout);
    drawRam(fout, prc);
}