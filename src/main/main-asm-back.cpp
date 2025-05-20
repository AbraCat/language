#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>
#include <asm-backend.h>

const char *std_tree_name = "./txt/tree.txt", *std_nasm_name = "asm/prog.asm";

int main(int argc, const char* argv[])
{
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char  *tree_name = optByName(opts, n_opts, "-i")->str_arg, 
                *nasm_name = optByName(opts, n_opts, "-o")->str_arg;
    if (tree_name == NULL) tree_name = std_tree_name;
    if (nasm_name == NULL) nasm_name = std_nasm_name;

    Node *tree = NULL;
    handleErr(treeRead(tree_name, &tree, NULL));

    FILE* nasm_file = fopen(std_nasm_name, "w");
    handleErr(runAsmBackend(tree, nasm_file));

    fclose(nasm_file);
    nodeDtor(tree);
    return 0;
}
