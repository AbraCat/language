#ifndef BACKEND_H
#define BACKEND_H

#include <my-error.h>
#include <tree.h>
#include <name.h>

#include <stdio.h>

extern int n_vars;

ErrEnum fillFuncArr(NameArr** name_arr, Node* node);
ErrEnum compileCommaSeparated(FILE* fout, Node* node, ErrEnum (*compile)(FILE*, Node*));
ErrEnum checkFuncParam(FILE* fout, Node* node);

#endif // BACKEND_H