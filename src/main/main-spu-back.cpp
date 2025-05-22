#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>

#include <assembler.h>
#include <disassembler.h>
#include <spu-backend.h>

const char *std_tree_name = "txt/tree-optimized.txt", *std_asm_name = "./txt/asm.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char  *tree_name = optByName(opts, n_opts, "-i")->str_arg, 
                *asm_name = optByName(opts, n_opts, "-o")->str_arg;
    if (tree_name == NULL) tree_name = std_tree_name;
    if (asm_name == NULL) asm_name = std_asm_name;

    Node *tree = NULL;
    handleErr(treeRead(tree_name, &tree, NULL));

    FILE *asm_file = fopen(asm_name, "w");
    handleErr(runBackend(tree, asm_file));
    
    nodeDtor(tree);
    fclose(asm_file);
    return 0;
}
