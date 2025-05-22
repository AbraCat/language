#include <stdio.h>
#include <stdlib.h>

#include <options.h>
#include <my-error.h>
#include <tree.h>
#include <middlend.h>

const char *std_input_name = "txt/tree.txt", *std_output_name = "txt/tree-optimized.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 2;
    Option opts[] = {{"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char  *input_name = optByName(opts, n_opts, "-i")->str_arg,
                *output_name = optByName(opts, n_opts, "-o")->str_arg;
    if (input_name == NULL) input_name = std_input_name;
    if (output_name == NULL) output_name = std_output_name;

    Node* tree = NULL;
    handleErr(treeRead(input_name, &tree, NULL));
    handleErr(runMiddleEnd(tree));
    // treeDump(tree);

    FILE* fout = fopen(output_name, "w");
    treeWrite(fout, tree);
    
    fclose(fout);
    nodeDtor(tree);
    return 0;
}