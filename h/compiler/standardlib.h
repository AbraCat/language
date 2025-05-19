#ifndef STANDARTLIB_H
#define STANDARTLIB_H

#include <error.h>
#include <stdio.h>
#include <name.h>

void printStdLib(FILE* fout, const char* trash_reg, const char* ret_val_reg, const char* frame_adr_reg);
void asmPrintStdLib(FILE* file);
ErrEnum getBinStdLib(FILE* file, char* buf);

ErrEnum insertStdFunction(NameArr* name_arr, const char* name, int n_args, int offset);
ErrEnum insertStdNames(NameArr* name_arr);
void removeSectionHeader(char* buf);

#endif // STANDARTLIB_H