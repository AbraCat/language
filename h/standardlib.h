#ifndef STANDARTLIB_H
#define STANDARTLIB_H

#include <my-error.h>
#include <stdio.h>
#include <name.h>

ErrEnum stripStdlib();

void writeHeader(char* buf, int entrypoint);
void writeProgramHeader(char* buf, int flags, int offset);
ErrEnum getHeaderAndStdlib(char* buf);
void patchCodeSize(char* buf, int size);

void printStdLib(FILE* fout, const char* trash_reg, const char* ret_val_reg, const char* frame_adr_reg);
void asmPrintStdLib(FILE* file);

ErrEnum insertStdFunction(NameArr* name_arr, const char* name, int n_args, int offset);
ErrEnum insertStdNames(NameArr* name_arr);

#endif // STANDARTLIB_H