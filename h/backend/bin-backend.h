#ifndef BIN_BACKEND
#define BIN_BACKEND

#include <my-error.h>
#include <tree.h>
#include <stdlib.h>
#include <tokenizer.h>
#include <label.h>

struct BinBackend
{
    int pos, label_cnt, n_args;
    NameArr* name_arr;
    char *buf = NULL, *name_buf = NULL;
    LabelArray *la = NULL, *ft = NULL;
};

ErrEnum runBinBackend(Node* tree, FILE* fout);

#endif // BIN_BACKEND