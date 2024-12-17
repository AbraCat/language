#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <assembler.h>
#include <disassembler.h>
#include <error.h>

#include <frontend.h>
#include <tree.h>
#include <backend.h>

const char *std_asm_fin = "txt/asm.txt", *std_asm_fout = "txt/code.txt",
           *std_dis_fin = "txt/code.txt", *std_dis_fout = "txt/asm.txt";

int mainAsm(const char* fin_name, const char* fout_name);
int mainDisasm(const char* fin_name, const char* fout_name);

int main(int argc, const char* argv[])
{
    // handleErr((ErrEnum)mainAsm(std_asm_fin, std_asm_fout)); return 0;

    Node *tree = NULL, *to_free = NULL;
    FILE *asm_file = fopen(std_asm_fin, "w");

    handleErr(runFrontend("./txt/prog.txt", &tree, &to_free));

    // FILE *tree_file = fopen("./txt/tree.txt", "w");
    // if (tree_file == NULL) handleErr(ERR_OPEN_FILE);
    // treeWrite(tree_file, tree);
    // fclose(tree_file);

    // Node *tree_copy = NULL;
    // returnErr(treeRead("./txt/tree.txt", &tree_copy));
    // returnErr(treeDump(tree_copy));
    // myAssert(treeCmp(tree, tree_copy) && !treeCmp(tree->lft, tree_copy));
    // return 0;

    handleErr(runBackend(tree, asm_file));
    free(to_free);
    fclose(asm_file);

    handleErr((ErrEnum)mainAsm(std_asm_fin, std_asm_fout));
    return 0;




    
    const int n_opts = 3;
    Option opts[] = {{"-d", "--decompile"}, {"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *fin_name = optByName(opts, n_opts, "-i")->str_arg, 
              *fout_name = optByName(opts, n_opts, "-o")->str_arg;

    if (optByName(opts, n_opts, "-d")->trig)
        return mainDisasm(fin_name, fout_name);
    return mainAsm(fin_name, fout_name);
}

int mainAsm(const char* fin_name, const char* fout_name)
{
    if (fin_name == NULL) fin_name = std_asm_fin;
    if (fout_name == NULL) fout_name = std_asm_fout;

    FILE *fin = fopen(fin_name, "r"), *fout = fopen(fout_name, "wb");
    if (fin == NULL || fout == NULL)
        return ERR_FILE;

    returnErr(runAsm(fin, fout));

    fclose(fin);
    fclose(fout);
    return 0;
}

int mainDisasm(const char* fin_name, const char* fout_name)
{
    if (fin_name == NULL) fin_name = std_dis_fin;
    if (fout_name == NULL) fout_name = std_dis_fout;

    FILE *fin = fopen(fin_name, "rb"), *fout = fopen(fout_name, "w");
    if (fin == NULL || fout == NULL)
        return ERR_FILE;

    runDisasm(fin, fout);

    fclose(fin);
    fclose(fout);
    return 0;
}