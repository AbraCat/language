#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>

#include <assembler.h>

const char *std_asm_name = "./txt/asm.txt", *std_code_name = "./txt/code.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *asm_name = optByName(opts, n_opts, "-i")->str_arg, 
               *code_name = optByName(opts, n_opts, "-o")->str_arg;
    if (asm_name == NULL) asm_name = std_asm_name;
    if (code_name == NULL) code_name = std_code_name;

    FILE *asm_file = fopen(asm_name, "r"), *code_file = fopen(code_name, "wb");
    handleErr(runAsm(asm_file, code_file));
    
    fclose(asm_file);
    fclose(code_file);
    return 0;
}
