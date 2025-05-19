#ifndef STANDARTLIB_H
#define STANDARTLIB_H

#include <error.h>
#include <stdio.h>
#include <name.h>

void insertStdNames(NameArr* name_arr);

ErrEnum getBinStdLib(FILE* file, char* buf);
void printStdLib(FILE* fout, const char* trash_reg, const char* ret_val_reg, const char* frame_adr_reg);
void asmPrintStdLib(FILE* file);

#endif // STANDARTLIB_H