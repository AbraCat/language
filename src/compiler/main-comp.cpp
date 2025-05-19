#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <assembler.h>
#include <disassembler.h>
#include <error.h>

#include <frontend.h>
#include <tree.h>
#include <middlend.h>
#include <backend.h>
#include <antifrontend.h>

#include <asm-backend.h>
#include <bin-backend.h>

const char *std_prog_name = "./txt/prog.txt", *std_tree_name = "./txt/tree.txt", 
           *std_asm_name = "./txt/asm.txt",   *std_code_name = "./txt/code.txt",

           *std_nasm_name = "asm/prog.asm", *std_bin_name = "bin/prog.exe";

ErrEnum checkTreeReadWrite(Node *tree);
ErrEnum checkAntiFrontend(Node* tree);

int main(int argc, const char* argv[])
{
    const int n_opts = 5;
    Option opts[] = {{"-i", "--input"}, {"-a", "--asm"}, {"-o", "--output"}, 
    {"-n", "--nasm"}, {"-b", "--binary"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *prog_name = optByName(opts, n_opts, "-i")->str_arg, 
                *asm_name = optByName(opts, n_opts, "-a")->str_arg,
               *code_name = optByName(opts, n_opts, "-o")->str_arg;
    if (prog_name == NULL) prog_name = std_prog_name;
    if (asm_name == NULL) asm_name = std_asm_name;
    if (code_name == NULL) code_name = std_code_name;

    Node *tree = NULL, *to_free = NULL;
    NameArr* name_arr = NULL;
    const char* prog_text = NULL;
    handleErr(runFrontend(prog_name, &tree, &to_free, &name_arr, &prog_text));
    handleErr(runMiddleEnd(tree));

    if (optByName(opts, n_opts, "-n")->trig)
    {
        FILE* nasm_file = fopen(std_nasm_name, "w");
        handleErr(runAsmBackend(tree, name_arr, nasm_file));
        fclose(nasm_file);

        nameArrDtor(name_arr);
        free((void*)prog_text);
        free(to_free);
        return 0;
    }
    if (optByName(opts, n_opts, "-b")->trig)
    {
        FILE* bin_file = fopen(std_bin_name, "w");
        handleErr(runBinBackend(tree, name_arr, bin_file));
        fclose(bin_file);

        nameArrDtor(name_arr);
        free((void*)prog_text);
        free(to_free);
        return 0;
    }

    FILE *asm_file = fopen(asm_name, "w");
    handleErr(runBackend(tree, asm_file));
    fclose(asm_file);
    nameArrDtor(name_arr);
    free((void*)prog_text);
    free(to_free);

    asm_file = fopen(asm_name, "r");
    FILE *code_file = fopen(code_name, "wb");
    handleErr(runAsm(asm_file, code_file));
    fclose(asm_file);
    fclose(code_file);
}

ErrEnum checkTreeReadWrite(Node *tree)
{
    FILE *tree_file = fopen(std_tree_name, "w");
    treeWrite(tree_file, tree);
    fclose(tree_file);

    Node *tree_copy = NULL;
    const char* copy_buf = NULL;
    returnErr(treeDump(tree));
    returnErr(treeRead(std_tree_name, &tree_copy, &copy_buf));
    returnErr(treeDump(tree_copy));
    if (!treeEqual(tree, tree_copy)) return ERR_IO;

    free((void*)copy_buf);
    nodeDtor(tree_copy);
    return ERR_OK;
}

ErrEnum checkAntiFrontend(Node* tree)
{
    returnErr(runAntiFrontend(tree, std_prog_name));
    Node *tree_copy = NULL, *to_free_copy = NULL;
    NameArr* name_arr = NULL;
    const char *prog_text_copy = NULL;
    returnErr(runFrontend(std_prog_name, &tree_copy, &to_free_copy, &name_arr, &prog_text_copy));
    if (!treeEqual(tree, tree_copy)) return ERR_INVAL_TREE;
    free(to_free_copy);
    free((void*)prog_text_copy);
    nameArrDtor(name_arr);
    return ERR_OK;
}