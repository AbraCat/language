#include <utils.h>

#include <iostream>

int n_tests = 5;

int main()
{
    for (int i = 0; i < n_tests; ++i)
        system("command time -f %e exe/proc.exe 2>> txt/spu-result.txt");
    for (int i = 0; i < n_tests; ++i)
        system("command time -f %e bin/prog.exe 2>> txt/bin-result.txt");

    double *spu_result = NULL, *bin_result = NULL;
    handleErr(readDoubleArr("txt/spu-result.txt", &spu_result, n_tests));
    handleErr(readDoubleArr("txt/bin-result.txt", &bin_result, n_tests));

    double bin_exp = 0, bin_disp = 0, spu_exp = 0, spu_disp = 0;
    dispersion(spu_result, n_tests, &spu_disp, &spu_exp);
    dispersion(bin_result, n_tests, &bin_disp, &bin_exp);

    printf("SPU: %lf +- %lf\nBIN: %lf += %lf\n", spu_exp, spu_disp, bin_exp, bin_disp);
    return 0;
}