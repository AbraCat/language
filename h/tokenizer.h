#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <error.h>
#include <tree.h>

enum NameType
{
    NAME_FUNC,
    NAME_VAR,
    NAME_UNDECL,
};

struct Name
{
    char* name_str;
    int n_args; // for func
    NameType type;
};

struct NameArr
{
    Name* names;
    int n_names, max_names;
};

ErrEnum tokenize(const char* fin_name, Node** node_arr, int* n_nodes, NameArr* name_arr);
ErrEnum insertName(const char* name_str, NameArr* name_arr, int* name_id);

ErrEnum nameArrCtor(NameArr* name_arr);
void nameArrDtor(NameArr* name_arr);

#endif // TOKENIZER_H