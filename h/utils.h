#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include <my-error.h>

#define CALLOC_BUF(name, size) returnErr(callocErr((void**)(&name), 1, size))

#define OPEN_FILE(var, name, mode)         \
    FILE* var = fopen(name, mode);         \
    if (var == NULL) return ERR_OPEN_FILE;

static int myMin(int a, int b) { return a < b ? a : b; }
static int myMax(int a, int b) { return a > b ? a : b; }

ErrEnum fileSize(FILE *file, long *siz);
ErrEnum callocErr(void **ptr, size_t count, size_t size);
ErrEnum readFile(const char* file_name, void** array, int* size);
ErrEnum readDoubleArr(const char* file_path, double** arr, int n);

int isZero(double num);
ErrEnum strToPosInt(const char* str, int* ans);
void printDouble(FILE* fout, double num);
int intPow(int base, int power);

void dispersion(double* a, int n, double* disp, double* expect);

#endif // UTILS_H