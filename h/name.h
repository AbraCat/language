#ifndef NAME_H
#define NAME_H

#include <my-error.h>

enum NameType
{
    NAME_FUNC,
    NAME_VAR,
    NAME_UNDECL,
};

struct Name
{
    const char* name_str;
    int var_id; // for vars
    int n_args; // for func
    int adr;    // for func
    NameType type;
};

struct NameArr
{
    Name* names;
    int n_names, max_names;
};

ErrEnum nameArrCtor(NameArr** name_arr);
void nameArrDtor(NameArr* name_arr);

ErrEnum insertName(const char* name_str, NameArr* name_arr, int* name_id);
Name* findName(NameArr* name_arr, const char* name);

void nameArrDump(NameArr* arr);

#endif // NAME_H