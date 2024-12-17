#ifndef STR_H
#define STR_H

#include <stdio.h>

void strlenToSpace(const char* str, int* len);
void strncpyToSpace(char* dest, const char* src, int count);
void skipTrailSpace(const char* str, int* str_pos, int* eof);

int strcmpToSpace(const char* lft, const char* rgt);

int nameCmp(const char* lft, const char* rgt, int* lft_len);
void nameCpy(char* dest, const char* src);
int nameLen(const char* str);
void printName(FILE* fout, const char* name);

#endif // STR_H