#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <assembler.h>
#include <disassembler.h>
#include <error.h>

#include <frontend.h>
#include <tree.h>
#include <backend.h>

#include <antifrontend.h>

const char *std_prog_name = "./txt/prog.txt", *std_tree_name = "./txt/tree.txt", 
           *std_asm_name = "./txt/asm.txt",   *std_code_name = "./txt/code.txt";

ErrEnum checkTreeReadWrite(Node *tree)
{
    FILE *tree_file = fopen(std_tree_name, "w");
    treeWrite(tree_file, tree);
    fclose(tree_file);

    Node *tree_copy = NULL;
    const char* copy_buf = NULL;
    returnErr(treeRead(std_tree_name, &tree_copy, &copy_buf));

    returnErr(treeDump(tree));
    returnErr(treeDump(tree_copy));
    if (!treeEqual(tree, tree_copy)) return ERR_IO;

    free((void*)copy_buf);
    nodeDtor(tree_copy);
    return ERR_OK;
}

int main(int argc, const char* argv[])
{
    const int n_opts = 3;
    Option opts[] = {{"-i", "--input"}, {"-a", "--asm"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *prog_name = optByName(opts, n_opts, "-i")->str_arg, 
                *asm_name = optByName(opts, n_opts, "-a")->str_arg,
               *code_name = optByName(opts, n_opts, "-o")->str_arg;
    if (prog_name == NULL) prog_name = std_prog_name;
    if (asm_name == NULL) asm_name = std_asm_name;
    if (code_name == NULL) code_name = std_code_name;

    Node *tree = NULL, *to_free = NULL;
    const char* prog_text = NULL;
    handleErr(runFrontend(prog_name, &tree, &to_free, &prog_text));

    handleErr(runAntiFrontend(tree, "./txt/prog1.txt")); return 0;
    // handleErr(checkTreeReadWrite(tree));

    FILE *asm_file = fopen(asm_name, "w");
    handleErr(runBackend(tree, asm_file));
    fclose(asm_file);
    free((void*)prog_text);
    free(to_free);

    asm_file = fopen(asm_name, "r");
    FILE *code_file = fopen(code_name, "wb");
    handleErr(runAsm(asm_file, code_file));
    fclose(asm_file);
    fclose(code_file);
}