#include <ctype.h>

#include <str.h>

void skipTrailSpace(const char* str, int* str_pos, int* eof)
{
    while (isspace(str[*str_pos])) ++*str_pos;
    if (str[*str_pos] == '\0') *eof = 1;
    else *eof = 0;
}

void strlenToSpace(const char* str, int* len)
{
    *len = 0;
    while (!isspace(str[*len]) && str[*len] != '\0') ++*len;
}

void strncpyToSpace(char* dest, const char* src, int count)
{
    for (int i = 0; i < count && !isspace(src[i]) && src[i] != '\0'; ++i)
        dest[i] = src[i];
}

int strcmpToSpace(const char* lft, const char* rgt)
{
    while (!isspace(*lft) && *lft != '\0' && *lft == *rgt)
    {
        ++lft;
        ++rgt;
    }
    return *lft - *rgt;
}

int nameCmp(const char* lft, const char* rgt, int* lft_len)
{
    const char* init_lft = lft;
    while (isalnum(*lft) && *lft == *rgt)
    {
        ++lft;
        ++rgt;
    }
    if (lft_len != 0) *lft_len = init_lft - lft; // != NULL
    return (isalnum(*lft) ? *lft : '\0') - (isalnum(*rgt) ? *rgt : '\0');
}

void nameCpy(char* dest, const char* src)
{
    while (isalnum(*src))
    {
        *dest = *src;
        ++dest;
        ++src;
    }
}

int nameLen(const char* str)
{
    int len = 0;
    while (isalnum(str[len])) ++len;
    return len;
}