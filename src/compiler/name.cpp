#include <name.h>
#include <str.h>

#include <stdlib.h>

static const int max_names = 20;

ErrEnum nameArrCtor(NameArr** name_arr)
{
    *name_arr = (NameArr*)calloc(1, sizeof(NameArr));
    NameArr* arr = *name_arr;
    if (arr == NULL) return ERR_MEM;

    arr->max_names = max_names;
    arr->n_names = 0;

    arr->names = (Name*)calloc(max_names, sizeof(Name));
    if (arr->names == NULL) return ERR_MEM;

    return ERR_OK;
}

void nameArrDtor(NameArr* name_arr)
{
    if (name_arr == NULL) return;
    free(name_arr->names);
}

ErrEnum insertName(const char* name_str, NameArr* name_arr, int* name_id)
{
    myAssert(name_str != NULL && name_arr != NULL);

    for (int ind = 0; ind < name_arr->n_names; ++ind)
        if (nameCmp(name_str, name_arr->names[ind].name_str, NULL) == 0)
        {
            if (name_id != NULL) *name_id = ind;
            return ERR_OK;
        }
    if (name_arr->n_names >= max_names) return ERR_TOO_MANY_NAMES;

    name_arr->names[name_arr->n_names].name_str = name_str;
    name_arr->names[name_arr->n_names].type = NAME_UNDECL;
    name_arr->names[name_arr->n_names].n_args = -1;

    if (name_id != NULL) *name_id = name_arr->n_names;
    ++name_arr->n_names;
    return ERR_OK;
}

Name* findName(NameArr* name_arr, const char* name)
{
    for (int i = 0; i < name_arr->n_names; ++i)
        if (nameCmp(name_arr->names[i].name_str, name, NULL) == 0)
            return name_arr->names + i;
    return NULL;
}

void nameArrDump(NameArr* arr)
{
    printf("NameArr %p\n", arr);
    if (arr == NULL) return;
    printf("%d / %d names\n", arr->n_names, arr->max_names);
    Name* n = arr->names;
    for (int i = 0; i < arr->n_names; ++i)
    {
        printf("name %d: %s. type %d n_args %d\n", i, n[i].name_str, n[i].type, n[i].n_args);
    }
    printf("NameArr dump end\n");
}