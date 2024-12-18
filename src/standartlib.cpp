#include <standartlib.h>

void printStdLib(FILE* fout, const char* trash_reg, const char* ret_val_reg, const char* frame_adr_reg)
{
    fprintf(fout, "PUSH 0\nCALL main:\nHLT\n"
    "input:\nPOP %s\nIN\nRET\n"
    "output:\nOUT\nPOP %s\nPUSH 0\nRET\n"
    "sqrt:\nSQRT\nPOP %s\nPOP %s\nPUSH %s\nRET\n",
    frame_adr_reg, frame_adr_reg, ret_val_reg, frame_adr_reg, ret_val_reg);
}