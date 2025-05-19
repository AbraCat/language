#include <label.h>
#include <str.h>

#include <stdlib.h>
#include <string.h>

static const int max_label_len = 100, max_labels = 1000;

ErrEnum labelArrayCtor(LabelArray** label_array)
{
    LabelArray* la = *label_array = (LabelArray*)calloc(1, sizeof(LabelArray));
    if (la == NULL) return ERR_MEM;

    la->max_labels = max_labels;
    la->n_labels = 0;

    la->labels = (Label*)calloc(la->max_labels, sizeof(Label));
    if (la->labels == NULL) return ERR_MEM;
    la->name_buf = (char*)calloc(max_label_len * la->max_labels, sizeof(char));
    if (la->name_buf == NULL) return ERR_MEM;

    for (int i = 0; i < la->max_labels; ++i)
    {
        la->labels[i].adr = -1;
        la->labels[i].name = la->name_buf + i * max_label_len;
    }

    return ERR_OK;
}

void labelArrayDtor(LabelArray* la)
{
    free(la->labels);
    free(la->name_buf);
    free(la);
}

ErrEnum addLabel(LabelArray* la, int adr, const char* name, int work_with_names)
{
    if (la->n_labels >= la->max_labels) return ERR_TOO_MANY_NAMES;
    la->labels[la->n_labels].adr = adr;

    la->labels[la->n_labels].name = la->name_buf + la->n_labels * max_label_len;
    if (work_with_names) nameCpy(la->labels[la->n_labels].name, name);
    else strncpyToSpace(la->labels[la->n_labels].name, name, max_label_len);
    ++(la->n_labels);

    return ERR_OK;
}

void getLabelAdr(LabelArray* la, const char* name, int* adr, int work_with_names)
{
    for (int i = 0; i < la->max_labels; ++i)
    {
        if (work_with_names)
        {
            if (la->labels[i].name != NULL && nameCmp(la->labels[i].name, name, NULL) == 0)
            {
                *adr = la->labels[i].adr;
                return;
            }
        }
        else
        {
            if (la->labels[i].name != NULL && strcmpToSpace(la->labels[i].name, name) == 0)
            {
                *adr = la->labels[i].adr;
                return;
            }
        }
    }
    *adr = -1;
}

ErrEnum fixup(char* code, LabelArray* ft, LabelArray* la, int work_with_names)
{
    myAssert(code != NULL && ft != NULL && la != NULL);

    int adr = -1;
    for (int fixup_n = 0; fixup_n < ft->n_labels; ++fixup_n)
    {
        getLabelAdr(la, ft->labels[fixup_n].name, &adr, work_with_names);
        if (adr == -1)
        {
            printf("name: %s\n", ft->labels[fixup_n].name);
            return ERR_INVAL_LABEL;
        }
        if (work_with_names)
        {
            code[ft->labels[fixup_n].adr] = adr;
            // relative_adr            = function start adr - instuction end adr
            // adr                     = function start adr
            // ft->labels[fixup_n].adr = address in instruction start adr
            int relative_adr = adr - ft->labels[fixup_n].adr - 4;
            memcpy(code + ft->labels[fixup_n].adr, &relative_adr, sizeof(int));
        }
        else memcpy(code + sizeof(int) * ft->labels[fixup_n].adr, &adr, sizeof(int));
    }
    return ERR_OK;
}

void labelArrDump(LabelArray* la)
{
    printf("LabelArray %p\n", la);
    printf("size: %d capacity: %d\n", la->n_labels, la->max_labels);
    for (int i = 0; i < la->n_labels; ++i)
    {
        printf("%s %d\n", la->labels[i].name, la->labels[i].adr);
    }
}