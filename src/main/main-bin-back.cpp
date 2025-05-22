#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>

#include <standardlib.h>
#include <bin-backend.h>

const char *std_tree_name = "txt/tree-optimized.txt", *std_bin_name = "bin/prog.exe";

int main(int argc, const char* argv[])
{   
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *tree_name = optByName(opts, n_opts, "-i")->str_arg, 
               *bin_name = optByName(opts, n_opts, "-o")->str_arg;
    if (tree_name == NULL) tree_name = std_tree_name;
    if (bin_name == NULL) bin_name = std_bin_name;

    Node *tree = NULL;
    handleErr(treeRead(tree_name, &tree, NULL));

    FILE* bin_file = fopen(bin_name, "w");
    handleErr(runBinBackend(tree, bin_file));

    fclose(bin_file);
    nodeDtor(tree);
    return 0;
}
