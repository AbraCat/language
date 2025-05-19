#ifndef BIN_BACKEND
#define BIN_BACKEND

#include <error.h>
#include <tree.h>
#include <stdlib.h>
#include <tokenizer.h>

ErrEnum runBinBackend(Node* tree, NameArr* name_arr, FILE* fout);

#endif // BIN_BACKEND