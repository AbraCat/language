#include <stdio.h>
#include <stdlib.h>

#include <my-error.h>
#include <colors.h>

static ErrDescr err_arr[] = {
    #define ERR_CODEGEN(code) {ERR_ ## code, #code},
    #include <error-codegen.h>
    #undef ERR_CODEGEN
};

extern const int n_errs = (sizeof err_arr) / sizeof(ErrDescr);

void myAssertFn(int expr, const char* str_expr, const char* file, int line, const char* function)
{
    if (expr)
        return;

    printf("%sAssertion failed: %s at %s:%d (%s)%s\n", RED_STR, str_expr, file, line, function, DEFAULT_STR);
    exit(expr);
}

void getErrDescr(ErrEnum code, const char** descr)
{
    myAssert(descr != NULL);

    int ind = (int)code;
    if (ind < 0 || ind >= n_errs)
    {
        *descr = NULL;
        return;
    }
    *descr = err_arr[ind].str_code;
}

void handleErrFn(ErrEnum code, const char* file, int line, const char* function)
{
    if (code == ERR_OK) return;

    const char* descr = NULL;
    getErrDescr(code, &descr);

    if (descr == NULL) printf("%sUnknown error (%d) at %s:%d (%s)%s\n", RED_STR, (int)code, file, line, function, DEFAULT_STR);
    else printf("%sError: %s at %s:%d (%s)%s\n", RED_STR, descr, file, line, function, DEFAULT_STR);
    
    exit(code);
}