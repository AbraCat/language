#include <stdio.h>

#include <processor.h>
#include <error.h>
#include <options.h>

const char *std_proc_fcode = "txt/code.txt";

int main(int argc, const char* argv[])
{
    const int n_opts = 3;
    Option opts[] = {{"-c", "--code"}, {"-i", "--input"}, {"-o", "--output"}};
    handleErr(parseOpts(argc, argv, opts, n_opts));

    const char *fcode_name = optByName(opts, n_opts, "-c")->str_arg, 
                 *fin_name = optByName(opts, n_opts, "-i")->str_arg, 
                *fout_name = optByName(opts, n_opts, "-o")->str_arg;

    if (fcode_name == NULL) fcode_name = std_proc_fcode;
    FILE *fin = NULL, *fout = NULL, *fcode = fopen(fcode_name, "rb");

    if (fin_name == NULL) fin = stdin;
    else fin = fopen(fin_name, "r");
    if (fout_name == NULL) fout = stdout; 
    else fout = fopen(fout_name, "w");

    if (fcode == NULL || fin == NULL || fout == NULL)
        handleErr(ERR_FILE);

    handleErr(runProcFile(fcode, fin, fout));

    fclose(fcode);
    fclose(fout);
    return 0;
}