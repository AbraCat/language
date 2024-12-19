#ifndef STR_H
#define STR_H

#include <stdio.h>

void strlenToSpace(const char* str, int* len);
void strncpyToSpace(char* dest, const char* src, int count);
int strcmpToSpace(const char* lft, const char* rgt);
int strcmpToBracket(const char* lft, const char* rgt);

int nameCmp(const char* lft, const char* rgt, int* lft_len);
void nameCpy(char* dest, const char* src);
int nameLen(const char* str);
void printName(FILE* fout, const char* name);

void skipTrailSpace(const char* str, int* str_pos, int* eof);
void fputManyChars(FILE* fout, char chr, int cnt);
void clearComments(char* str, const char comment_start);

#endif // STR_H