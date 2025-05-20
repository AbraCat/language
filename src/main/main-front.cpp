#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>

#include <frontend.h>
#include <antifrontend.h>

const char *std_prog_name = "./txt/prog.txt", *std_tree_name = "./txt/tree.txt";

ErrEnum checkTreeReadWrite(Node *tree);
ErrEnum checkAntiFrontend(Node* tree);

int main(int argc, const char* argv[])
{
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *prog_name = optByName(opts, n_opts, "-i")->str_arg, 
               *tree_name = optByName(opts, n_opts, "-o")->str_arg;
    if (prog_name == NULL) prog_name = std_prog_name;
    if (tree_name == NULL) tree_name = std_tree_name;

    Node *tree = NULL, *to_free = NULL;
    NameArr* name_arr = NULL;
    const char* prog_text = NULL;
    handleErr(runFrontend(prog_name, &tree, &to_free, &name_arr, &prog_text));
    
    FILE* tree_file = fopen(tree_name, "w");
    treeWrite(tree_file, tree);
    
    nameArrDtor(name_arr);
    free((void*)prog_text);
    free(to_free);
    return 0;
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