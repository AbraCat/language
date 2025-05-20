#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>
#include <middlend.h>

const char *std_tree_name = "./txt/tree.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 1;
    Option opts[] = {{"-i", "--input"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *tree_name = optByName(opts, n_opts, "-i")->str_arg;
    if (tree_name == NULL) tree_name = std_tree_name;

    Node* tree = NULL;
    handleErr(treeRead(std_tree_name, &tree, NULL));
    handleErr(runMiddleEnd(tree));

    FILE* tree_file = fopen(tree_name, "w");
    treeWrite(tree_file, tree);
    
    fclose(tree_file);
    nodeDtor(tree);
    return 0;
}