#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <utils.h>

ErrEnum callocErr(void **ptr, size_t count, size_t size)
{
    void* ptr1 = calloc(count, size);
    if (ptr == NULL) return ERR_MEM;
    *ptr = ptr1;
    return ERR_OK;
}

ErrEnum fileSize(FILE *file, long *siz)
{
    myAssert(file != NULL && siz != NULL);

    if (fseek(file, 0L, SEEK_END))
    {
        *siz = -1L;
        return ERR_FILE;
    }

    *siz = ftell(file);
    if (*siz == -1L) return ERR_FILE;

    if (fseek(file, 0L, SEEK_SET)) return ERR_FILE;

    return ERR_OK;
}

ErrEnum readFile(const char* file_name, void** array, int* size)
{
    myAssert(file_name != NULL && array != NULL && *array == NULL && size != NULL);

    FILE *fin = fopen(file_name, "r");
    if (fin == NULL) return ERR_OPEN_FILE;
    returnErr(fileSize(fin, (long*)size));
    ++*size;
    *array = calloc(1, *size);
    if (*array == NULL) return ERR_MEM;
    fread(*array, 1, *size - 1, fin);
    fclose(fin);
    (*((char**)array))[*size - 1] = 0;
    return ERR_OK;
}

int isZero(double num)
{
    const double EPS = 1e-6;
    return fabs(num) < EPS;
}

ErrEnum strToPosInt(const char* str, int* ans)
{
    const int base = 10;
    *ans = 0;
    int digit = 0;

    if (isspace(*str) || *str == '\0') return ERR_STR_TO_INT;
    for (; !isspace(*str) && *str != '\0'; ++str)
    {
        digit = *str - '0';
        if (digit < 0 || digit > 9) return ERR_STR_TO_INT;
        *ans = *ans * base + digit;
    }

    return ERR_OK;
}

void printDouble(FILE* fout, double num)
{
    myAssert(fout != NULL);

    const int max_prec = 3;
    if (num < 0)
    {
        num *= -1;
        fputc('-', fout);
    }
    fprintf(fout, "%d", (long long)num);
    num -= (long long)num;
    long long ten_pow = 1;
    double num10pow = num;
    if (!isZero(num - (long long)num)) fputc('.', fout);
    for (int digit = 1; digit <= max_prec && !isZero(num10pow - (long long)num10pow); ++digit)
    {
        ten_pow *= 10;
        num10pow = num * ten_pow;
        fprintf(fout, "%d", (long long)num10pow % 10);
    }
}

int intPow(int base, int power)
{
    myAssert(power >= 0);
    if (power == 0) return 1;
    int temp = intPow(base, power / 2);
    if (power % 2 == 0) return temp * temp;
    return temp * temp * base;
}

ErrEnum readDoubleArr(const char* file_path, double** arr, int n)
{
    FILE* file = fopen(file_path, "r");
    if (file == NULL) return ERR_OPEN_FILE;

    double *a = *arr = (double*)calloc(n, sizeof(double));
    for (int i = 0; i < n; ++i)
        fscanf(file, "%lf", a + i);
    
    fclose(file);
    return ERR_OK;
}

void dispersion(double* a, int n, double* disp, double* expect)
{
    double sum = 0, sum_of_sq = 0;
    for (int i = 0; i < n; ++i)
    {
        sum += a[i];
        sum_of_sq += a[i] * a[i];
    }
    double _expect = sum / n;
    if (expect != NULL) *expect = _expect;
    *disp = sum_of_sq / n - _expect * _expect;
}
