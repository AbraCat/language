#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <my-error.h>
#include <tree.h>
#include <name.h>

ErrEnum tokenize(const char* fin_name, Node** node_arr, int* n_nodes, NameArr* name_arr, const char** prog_text);

#endif // TOKENIZER_H