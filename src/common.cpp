#include <stdlib.h>

#include <common.h>
#include <utils.h>

ErrEnum printRegName(RegEnum reg_num, FILE* fout)
{
    myAssert(fout != NULL);
    if (reg_num < 0 || reg_num >= n_regs)
        return ERR_INVAL_REG_NUM;

    fprintf(fout, "%cX", 'A' + reg_num);
    return ERR_OK;
}

ErrEnum readCode(FILE* fcode, int** code, int* n_cmds)
{
    myAssert(fcode != NULL && code != NULL);

    long fcode_size = 0;
    returnErr(fileSize(fcode, &fcode_size));
    *n_cmds = fcode_size / sizeof(int);

    *code = (int*)calloc(*n_cmds, sizeof(int));
    if (code == NULL) return ERR_MEM;
    fread(*code, sizeof(int), *n_cmds, fcode);

    return ERR_OK;
}