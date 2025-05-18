#ifndef ERROR_H
#define ERROR_H

// #define MY_NDEBUG

#define handleErr(err_num) handleErrFn(err_num, __FILE__, __LINE__, __FUNCTION__)
    
#define returnErr(expr)     \
{                           \
    ErrEnum err_num = expr; \
    if (err_num)            \
        return err_num;     \
}

#ifdef MY_NDEBUG
#define myAssert(expr)
#else
#define myAssert(expr) myAssertFn(expr, #expr, __FILE__, __LINE__, __FUNCTION__)
#endif

extern const int n_errs;

typedef enum ErrEnum
{
    #define ERR_CODEGEN(code) ERR_ ## code,
    #include <error-codegen.h>
    #undef ERR_CODEGEN
} ErrEnum;

struct ErrDescr
{
    ErrEnum code;
    const char *str_code;
};

void myAssertFn(int expr, const char* str_expr, const char* file, int line, const char* function);
void getErrDescr(ErrEnum code, const char** descr);
void handleErrFn(ErrEnum code, const char* file, int line, const char* function);

#endif // ERROR_H