#ifndef LABEL_H
#define LABEL_H

#include <error.h>

struct Label
{
    int adr;
    char* name;
};

struct LabelArray
{
    struct Label* labels;
    int n_labels, max_labels;
    char* name_buf;
};

ErrEnum labelArrayCtor(LabelArray** label_array);
void labelArrayDtor(LabelArray* la);

ErrEnum addLabel(LabelArray* la, int adr, const char* name, int work_with_names);
void getLabelAdr(LabelArray* la, const char* name, int* adr, int work_with_names);
ErrEnum fixup(char* code, LabelArray* ft, LabelArray* la, int work_with_names);

void labelArrDump(LabelArray* la);

#endif // LABEL_H