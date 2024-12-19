#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <assembler.h>
#include <common.h>
#include <error.h>
#include <utils.h>
#include <str.h>

const int n_cmds = 23, max_label_len = 50, buffer_size = 50;

#define CMD_ARRAY_CASE(name, type) {CMD_ ## name, CMDT_ ## type, #name, sizeof #name - 1},
Cmd cmd_array[n_cmds] = 
{
    CMD_ARRAY_CASE(HLT, NO_ARG)
    CMD_ARRAY_CASE(IN, NO_ARG)
    CMD_ARRAY_CASE(OUT, NO_ARG)
    CMD_ARRAY_CASE(PUSH, COMPLEX_ARG)
    CMD_ARRAY_CASE(POP, COMPLEX_ARG)
    CMD_ARRAY_CASE(ADD, NO_ARG)
    CMD_ARRAY_CASE(SUB, NO_ARG)
    CMD_ARRAY_CASE(MUL, NO_ARG)
    CMD_ARRAY_CASE(DIV, NO_ARG)
    CMD_ARRAY_CASE(DUMP, NO_ARG)
    CMD_ARRAY_CASE(JMP, LABEL_ARG)
    CMD_ARRAY_CASE(JB, LABEL_ARG)
    CMD_ARRAY_CASE(JBE, LABEL_ARG)
    CMD_ARRAY_CASE(JA, LABEL_ARG)
    CMD_ARRAY_CASE(JAE, LABEL_ARG)
    CMD_ARRAY_CASE(JE, LABEL_ARG)
    CMD_ARRAY_CASE(JNE, LABEL_ARG)
    CMD_ARRAY_CASE(CALL, LABEL_ARG)
    CMD_ARRAY_CASE(RET, NO_ARG)
    CMD_ARRAY_CASE(DRAW, NO_ARG)
    CMD_ARRAY_CASE(SQRT, NO_ARG)
    CMD_ARRAY_CASE(POW, NO_ARG)
    CMD_ARRAY_CASE(SIZE, NO_ARG)
};
#undef CMD_ARRAY_CASE

int cmdCmp(const void* cmd1, const void* cmd2)
{
    return strcmp(((const Cmd*)cmd1)->name, ((const Cmd*)cmd2)->name);
}

ErrEnum getCmdIndex(const char* cmd_name, int* index)
{
    int l = -1, r = n_cmds, m = 0;
    while (r - l > 1)
    {
        m = (l + r) / 2;
        if (strncmp(cmd_array[m].name, cmd_name, cmd_array[m].name_len) <= 0)
            l = m;
        else
            r = m;
    }
    if (l == -1 || strncmp(cmd_array[l].name, cmd_name, cmd_array[l].name_len) != 0)
        return ERR_INVAL_CMD;
    
    *index = l;
    return ERR_OK;
}

ErrEnum asmCtor(Asm* ase)
{
    qsort(cmd_array, n_cmds, sizeof(Cmd), cmdCmp);

    ase->ip = ase->arg1 = ase->arg2 = ase->prog_text_pos = 0;

    ase->code = (int*)calloc(max_cmds, sizeof(int));
    if (ase->code == NULL)
        return ERR_MEM;
    ase->prog_text = NULL;

    labelArrayCtor(&ase->la);
    labelArrayCtor(&ase->ft);

    return ERR_OK;
}
void asmDtor(Asm* ase)
{
    free(ase->prog_text);
    free(ase->code);
    labelArrayDtor(&ase->la);
    labelArrayDtor(&ase->ft);
}

void getRegNum(char* str_name, int* num)
{
    if ((!isspace(str_name[2]) && str_name[2] != '\0') || 
    str_name[1] != 'X' || str_name[0] - 'A' < 0 || str_name[0] - 'A' >= n_regs)
    {
        *num = INVAL_REG;
        return;
    }
    *num = str_name[0] - 'A';
}

ErrEnum getArg(Asm* ase)
{
    /*
    push:
    ok:    001 010 011 101 110 111
    wrong: 000 100

    pop:
    ok:    010 101 110 111
    wrong: 000 001 011 100
    */

    char *str_ptr1 = ase->prog_text + ase->prog_text_pos;
    while (isspace(*str_ptr1))
        ++str_ptr1;
    if (*str_ptr1 == '\0')
        return ERR_CMD_NO_ARG;

    char *str_ptr2 = str_ptr1;
    while (!isspace(*str_ptr2) && *str_ptr2 != '\0')
        ++str_ptr2;
    if (*str_ptr2 == '\0')
        ase->prog_text_pos = str_ptr2 - ase->prog_text;
    else
    {
        *str_ptr2 = '\0';
        ase->prog_text_pos = str_ptr2 - ase->prog_text + 1;
    }

    if (str_ptr2[-1] == ']')
    {
        // aaa]
        if (str_ptr1[0] != '[') return ERR_BRACKET;

        // [aaa]
        ase->code[ase->ip] |= MEM_BIT;
        ++str_ptr1;
        str_ptr2[-1] = ' ';
    }

    // aaa || [aaa
    char* chr1 = strchr(str_ptr1, '+');
    if (chr1 != NULL)
    {
        if (sscanf(chr1 + 1, "%d", ase->code + ase->ip + 2) != 1)
            return ERR_CMD_ARG_FMT;
        ase->code[ase->ip] |= REG_BIT | IMM_BIT;
        *(chr1) = ' ';
        getRegNum(str_ptr1, ase->code + ase->ip + 1);
        if (ase->code[ase->ip + 1] == INVAL_REG)
            return ERR_INVAL_REG_NAME;
        ase->ip += 3;
    }
    else if (sscanf(str_ptr1, "%d", ase->code + ase->ip + 1) == 1)
    {
        ase->code[ase->ip] |= IMM_BIT;
        ase->ip += 2;
    }

    else
    {
        ase->code[ase->ip] |= REG_BIT;
        getRegNum(str_ptr1, ase->code + ase->ip + 1);
        if (ase->code[ase->ip + 1] == INVAL_REG)
            return ERR_INVAL_REG_NAME;
        ase->ip += 2;
    }

    return ERR_OK;
}

ErrEnum runAsm(FILE* fin, FILE* fout)
{
    #define scanfProgText(fmt, ...)                           \
    (                                                         \
        scanf_res = sscanf(ase.prog_text + ase.prog_text_pos, \
        fmt "%n", __VA_ARGS__, &pos_incr),                    \
        ase.prog_text_pos += pos_incr,                        \
        scanf_res                                             \
    )

    #define scanfProgTextExp(exp_res, fmt, ...)         \
        if (scanfProgText(fmt, __VA_ARGS__) != exp_res) \
            return ERR_CMD_ARG_FMT;

    #define ASM_CMDT_CASE(type)                         \
        case CMDT_ ## type:                             \
            ASM_CASE_ ## type (cmd_array[cmd_ind].code)

    #define ASM_CASE_NO_ARG(command)  \
        ase.code[ase.ip++] = command; \
        continue;                             

    #define ASM_CASE_INT_ARG(command)         \
        ase.code[ase.ip++] = command;         \
        scanfProgTextExp(1, "%d", &ase.arg1); \
        ase.code[ase.ip++] = ase.arg1;        \
        continue;

    #define ASM_CASE_COMPLEX_ARG(command)   \
        ase.code[ase.ip] = command;         \
        returnErr(getArg(&ase));            \
        continue;

    #define ASM_CASE_LABEL_ARG(command)                          \
        ase.code[ase.ip++] = command;                            \
        skipTrailSpace(ase.prog_text, &ase.prog_text_pos, &eof); \
        if (eof) return ERR_CMD_ARG_FMT;                         \
        label = ase.prog_text + ase.prog_text_pos;               \
        strlenToSpace(ase.prog_text + ase.prog_text_pos, &len);  \
        if (label[len - 1] == label_end)                         \
        {                                                        \
            getLabelAdr(&ase.la, label, &ase.arg1);              \
            ase.code[ase.ip++] = ase.arg1;                       \
            if (ase.arg1 == -1)                                  \
                addLabel(&ase.ft, ase.ip - 1, label);            \
        }                                                        \
        else                                                     \
            returnErr(strToPosInt(label, ase.code + ase.ip++));  \
        ase.prog_text_pos += len;                                \
        continue;



    myAssert(fin != NULL && fout != NULL);

    int scanf_res = 0, pos_incr = 0, eof = 0, len = 0, cmd_ind = 0;
    char* label = NULL;
    const char label_end = ':';
    Asm ase = {};
    returnErr(asmCtor(&ase));

    long str_code_size = 0;
    returnErr(fileSize(fin, &str_code_size));
    
    ase.prog_text = (char*)calloc(str_code_size + 1, sizeof(char));
    if (ase.prog_text == NULL) return ERR_MEM;
    fread(ase.prog_text, sizeof(char), str_code_size, fin);

    clearComments(ase.prog_text, ';');
    while (1)
    {
        skipTrailSpace(ase.prog_text, &ase.prog_text_pos, &eof);
        if (eof) break;

        strlenToSpace(ase.prog_text + ase.prog_text_pos, &len);
        if (ase.prog_text[ase.prog_text_pos + len - 1] == label_end)
        {
            addLabel(&ase.la, ase.ip, ase.prog_text + ase.prog_text_pos);
            ase.prog_text_pos += len;
            continue;
        }

        returnErr(getCmdIndex(ase.prog_text + ase.prog_text_pos, &cmd_ind));
        ase.prog_text_pos += cmd_array[cmd_ind].name_len;
        switch (cmd_array[cmd_ind].type)
        {
            ASM_CMDT_CASE(NO_ARG)
            ASM_CMDT_CASE(INT_ARG)
            ASM_CMDT_CASE(COMPLEX_ARG)
            ASM_CMDT_CASE(LABEL_ARG)
            default:
                return ERR_UNKNOWN;
        }

        asmDtor(&ase);
        return ERR_INVAL_CMD;
    }

    returnErr(fixup(ase.code, &ase.ft, &ase.la));
    if (fwrite(ase.code, sizeof(int), ase.ip, fout) != ase.ip)
        return ERR_IO;
    asmDtor(&ase);

    #undef scanfProgText
    #undef scanfProgTextExp
    #undef ASM_CMDT_CASE
    #undef ASM_CASE_NO_ARG
    #undef ASM_CASE_INT_ARG
    #undef ASM_CASE_COMPLEX_ARG
    #undef ASM_CASE_LABEL_ARG

    return ERR_OK;
}

ErrEnum labelArrayCtor(LabelArray* la)
{
    la->max_labels = 50;
    la->n_labels = 0;

    la->labels = (Label*)calloc(la->max_labels, sizeof(Label));
    if (la->labels == NULL) return ERR_MEM;
    la->name_buf = (char*)calloc(max_label_len * la->max_labels, sizeof(char));
    if (la->name_buf == NULL) return ERR_MEM;

    for (int i = 0; i < la->max_labels; ++i)
    {
        la->labels[i].adr = -1;
        la->labels[i].name = la->name_buf + i * max_label_len;
    }

    return ERR_OK;
}

void labelArrayDtor(LabelArray* la)
{
    free(la->labels);
    free(la->name_buf);
}

void addLabel(LabelArray* la, int adr, char* name)
{
    la->labels[la->n_labels].adr = adr;

    la->labels[la->n_labels].name = la->name_buf + la->n_labels * max_label_len;
    strncpyToSpace(la->labels[la->n_labels].name, name, max_label_len);
    ++(la->n_labels);
}

void getLabelAdr(LabelArray* la, char* name, int* adr)
{
    for (int i = 0; i < la->max_labels; ++i)
    {
        if (la->labels[i].name != NULL && strcmpToSpace(la->labels[i].name, name) == 0)
        {
            *adr = la->labels[i].adr;
            return;
        }
    }
    *adr = -1;
}

ErrEnum fixup(int* code, LabelArray* ft, LabelArray* la)
{
    myAssert(code != NULL && ft != NULL && la != NULL);

    int adr = -1;
    for (int fixup_n = 0; fixup_n < ft->n_labels; ++fixup_n)
    {
        getLabelAdr(la, ft->labels[fixup_n].name, &adr);
        if (adr == -1)
            return ERR_INVAL_LABEL;
        code[ft->labels[fixup_n].adr] = adr;
    }
    return ERR_OK;
}