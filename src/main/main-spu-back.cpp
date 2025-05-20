#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>

#include <assembler.h>
#include <disassembler.h>
#include <spu-backend.h>

const char *std_tree_name = "./txt/tree.txt", 
           *std_asm_name = "./txt/asm.txt",   
           *std_code_name = "./txt/code.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 3;
    Option opts[] = {{"-i", "--input"}, {"-a", "--asm"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *tree_name = optByName(opts, n_opts, "-i")->str_arg, 
                *asm_name = optByName(opts, n_opts, "-a")->str_arg,
               *code_name = optByName(opts, n_opts, "-o")->str_arg;
    if (tree_name == NULL) tree_name = std_tree_name;
    if (asm_name == NULL) asm_name = std_asm_name;
    if (code_name == NULL) code_name = std_code_name;

    Node *tree = NULL;
    handleErr(treeRead(tree_name, &tree, NULL));

    FILE *asm_file = fopen(asm_name, "w");
    handleErr(runBackend(tree, asm_file));
    fclose(asm_file);

    asm_file = fopen(asm_name, "r");
    FILE *code_file = fopen(code_name, "wb");
    handleErr(runAsm(asm_file, code_file));
    
    fclose(asm_file);
    fclose(code_file);
}
