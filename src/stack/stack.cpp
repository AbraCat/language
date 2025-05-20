#include <stdio.h>
#include <stdlib.h>

#include <stack.h>
#include <utils.h>
#include <colors.h>

#define stUpdateHash(st) ST_ON_HASH(stUpdateHashFn(st);)

static const StackElem poison_val = 1, canary_val = 2;
static const int capacity_increase = 2, capacity_decrease = 2, capacity_decrease_condition = 4;

static int hashFn(char* arr, int size);
static void stUpdateHashFn(Stack* st);

ErrEnum stCtorNDebug(Stack* st, int capacity)
{
    if (st == NULL)
        return ERR_NULL_STACK;

    st->size = 0;
    st->capacity = capacity;
    st->data = NULL;

    ST_ON_CANARY
    (
        st->left_st_canary = st->right_st_canary = canary_val;
    )

    returnErr(resize(st, capacity));

    stUpdateHash(st);

    returnErr(stErr(st));
    return ERR_OK;
}

ErrEnum stCtorDebug(Stack* st, int capacity, const char* file_born, int line_born, const char* func_born)
{
    if (st == NULL)
        return ERR_NULL_STACK;

    ST_ON_DEBUG
    (
        st->debug.file_born = file_born;
        st->debug.line_born = line_born;
        st->debug.func_born = func_born;
    )

    ST_ON_CANARY
    (
        st->left_st_canary = st->right_st_canary = canary_val;
    )

    st->size = 0;
    st->capacity = capacity;
    st->data = NULL;

    returnErr(resize(st, capacity));

    stUpdateHash(st);

    returnErr(stErr(st));
    return ERR_OK;
}

void stDtor(Stack* st)
{
    if (st == NULL)
        return;

    int fill_val = 0;
    ST_ON_DEBUG(fill_val = poison_val);

    if (st->data != NULL)
    {
        ST_ON_CANARY
        (
            for (int i = -1; i <= st->capacity; ++i)
            {
                st->data[i] = fill_val;
            }

            free(st->data - 1);
        )

        ST_ON_NO_CANARY
        (
            for (int i = 0; i < st->capacity; ++i)
            {
                st->data[i] = fill_val;
            }

            free(st->data);
        )

        st->data = NULL;
    }

    st->size = st->capacity = fill_val;

    ST_ON_CANARY
    (
        st->left_st_canary = st->right_st_canary = fill_val;
    )
    ST_ON_HASH
    (
        st->st_hash = st->data_hash = fill_val;
    )
    ST_ON_DEBUG
    (
        st->debug.file_born = st->debug.func_born = NULL;
        st->debug.line_born = fill_val;
    )
}

static int hashFn(char* arr, int size)
{
    int hash = 5381;

    if (arr == NULL)
        return hash;

    for (int i = 0; i < size; ++i)
    {
        hash = (hash << 5) + hash + arr[i];
    }

    return hash;
}

static void stUpdateHashFn(Stack* st)
{
    ST_ON_HASH
    (
        stAssert(st != NULL);

        st->st_hash = st->data_hash = 0;

        ST_ON_NO_CANARY
        (
        st->st_hash = hashFn((char*)st, sizeof(Stack));
        )

        ST_ON_CANARY
        (
        st->st_hash = hashFn((char*)st + sizeof(StackElem), sizeof(Stack) - 2 * sizeof(StackElem));
        )

        st->data_hash = hashFn((char*)(st->data), st->capacity * sizeof(StackElem));
    )
}

ErrEnum resize(Stack* st, int new_capacity)
{
    stAssert(st != NULL);

    st->capacity = new_capacity;
    if (new_capacity == 0)
    {
        st->data = NULL;
        return ERR_OK;
    }

    StackElem* new_data = NULL;
    ST_ON_NO_CANARY
    (
        new_data = (StackElem*)realloc(st->data, new_capacity * sizeof(StackElem));
    )
    ST_ON_CANARY
    (
        new_data = (StackElem*)realloc(st->data == NULL ? NULL : st->data - 1, (new_capacity + 2) * sizeof(StackElem)) + 1;
    )

    if (new_data == NULL)
    {
        return ERR_MEM;
    }

    st->data = new_data;

    ST_ON_CANARY
    (
        st->data[-1] = st->data[new_capacity] = canary_val;
    )
    ST_ON_DEBUG
    (
        for (int i = st->size; i < new_capacity; ++i)
        {
            st->data[i] = poison_val;
        }
    )

    return ERR_OK;
}

ErrEnum stPush(Stack* st, StackElem elem)
{
    returnErr(stErr(st));

    if (st->size == st->capacity)
    {
        return ERR_STACK_OVERFLOW;
        returnErr(resize(st, st->capacity == 0 ? 1 : st->capacity * capacity_increase));
    }

    st->data[st->size++] = elem;

    stUpdateHash(st);
    returnErr(stErr(st));
    return ERR_OK;
}

ErrEnum stPop(Stack* st, StackElem* elem)
{
    returnErr(stErr(st));

    if (st->size == 0)
        return ERR_STACK_UNDERFLOW;

    *elem = st->data[--(st->size)];
    
    ST_ON_DEBUG
    (
        st->data[st->size] = poison_val;
    )

    // if (st->size <= st->capacity / capacity_decrease_condition)
    // {
    //     returnErr(resize(st, st->capacity / capacity_decrease));
    // }

    stUpdateHash(st);
    returnErr(stErr(st));
    return ERR_OK;
}

ErrEnum stErr(Stack* st)
{
    #ifdef ST_NDEBUG
    return ERR_OK;
    #endif // ST_NDEBUG
    
    if (st == NULL)
    {
        return ERR_NULL_STACK;
    }

    if (st->data == NULL && st->capacity != 0)
    {
        return ERR_NULL_STACK;
    }

    ST_ON_DEBUG
    (
        if (st->debug.file_born == NULL || st->debug.func_born == NULL)
        {
            return ERR_NULL_STACK;
        }
    )

    if (0 > st->size || st->size > st->capacity)
        return ERR_BAD_SIZE;

    ST_ON_DEBUG
    (
        for (int i = st->size; i < st->capacity; ++i)
        {
            if (st->data[i] != poison_val)
            {
                return ERR_POISON;
            }
        }
    )

    ST_ON_CANARY
    (
        if (st->left_st_canary != canary_val || st->right_st_canary != canary_val)
        {
            return ERR_BAD_CANARY;
        }

        if (st->data != NULL && (st->data[-1] != canary_val || st->data[st->capacity] != canary_val))
        {
            return ERR_BAD_CANARY;
        }
    )



    ST_ON_HASH
    (
        const int st_hash_saved = st->st_hash, data_hash_saved = st->data_hash;

        stUpdateHash(st);

        int good_hash = (st->st_hash == st_hash_saved && st->data_hash == data_hash_saved);

        st->st_hash = st_hash_saved;
        st->data_hash = data_hash_saved;

        if (!good_hash)
            return ERR_BAD_HASH;
    )

    return ERR_OK;
}

void stDumpFn(FILE* file, Stack* st, const char* file_name, int line, const char* func_name)
{
    #define PRINT_ERR(err)                                                    \
    {                                                                         \
        const char* descr = NULL;                                                   \
        getErrDescr(ERR_ ## err, &descr);                                     \
        fprintf(file, "%sstDump(): %s%s\n", MAGENTA_STR, descr, DEFAULT_STR); \
    }
    static const int max_n_elem = 20;

    if (file == NULL || file_name == NULL || func_name == NULL)
    {
        PRINT_ERR(NULL_STACK);
        return;
    }

    if (st == NULL)
    {
        fprintf(file, "Stack [NULL] at %s:%d (function %s)\n\n", file_name, line, func_name);
        PRINT_ERR(NULL_STACK);
        return;
    }

    ST_ON_RELEASE
    (
        fprintf(file, "Stack [0x%p] at %s:%d (function %s)\n\n", 
        st, file_name, line, func_name);
    )
    ST_ON_DEBUG
    (
        fprintf(file, "Stack [0x%p] at %s:%d (function %s) born at %s:%d (function %s)\n\n", 
        st, file_name, line, func_name, 
        st->debug.file_born == NULL ? "???" : st->debug.file_born, st->debug.line_born, 
        st->debug.func_born == NULL ? "???" : st->debug.func_born);
    )

    fprintf(file, "size = %d\ncapacity = %d\n\n", st->size, st->capacity);

    int too_big = 0;
    if (0 > st->size || st->size > st->capacity)
    {
        PRINT_ERR(BAD_SIZE);
    }
    else if (st->capacity > max_n_elem)
    {
        too_big = 1;
        if (st->size > max_n_elem)
        {
            fprintf(file, "%s%s%s", MAGENTA_STR, "Warning: size is too big\n\n", DEFAULT_STR);
        }
        else
        {
            fprintf(file, "%s%s%s", MAGENTA_STR, "Warning: capacity is too big\n\n", DEFAULT_STR);
        }
    }

    ST_ON_CANARY
    (
        fprintf(file, "left_st_canary = 0x%X\nright_st_canary = 0x%X\n", 
        st->left_st_canary, st->right_st_canary);
        if (st->left_st_canary != canary_val || st->right_st_canary != canary_val)
        {
            PRINT_ERR(BAD_CANARY);
        }

        if (st->data != NULL)
        {
            fprintf(file, "left_data_canary = 0x%X\nright_data_canary = 0x%X\n", 
            st->data[-1], st->data[st->capacity]);
            if (st->data[-1] != canary_val || st->data[st->capacity] != canary_val)
            {
                PRINT_ERR(BAD_CANARY);
            }
        }

        fputc('\n', file);

    )

    ST_ON_HASH
    (
        fprintf(file, "st_hash = %d\ndata_hash = %d\n\n", st->st_hash, st->data_hash);
    )



    if (st->data == NULL)
    {
        fprintf(file, "data [NULL]:\n\n");
        if (st->capacity != 0)
            PRINT_ERR(NULL_STACK);
        return;
    }

    fprintf(file, "data [0x%p]:\n\n", st->data);


    int rgt1 = myMin(max_n_elem, myMin(st->size, st->capacity));
    int rgt2 = myMin(max_n_elem, st->capacity);

    for (int i = 0; i < rgt1; ++i)
    {
        fprintf(file, "* [%d]: %d\n", i, st->data[i]);
    }
    for (int i = myMax(st->size, 0); i < rgt2; ++i)
    {
        fprintf(file, "  [%d]: %d\n", i, st->data[i]);
    }
    if (too_big)
    {
        fprintf(file, "...\n");
    }

    putc('\n', file);

    #undef PRINT_ERR
}