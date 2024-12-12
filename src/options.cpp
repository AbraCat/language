#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <options.h>

typedef int (*voidcmp_f)(const void*, const void*);

int optcmp(struct Option* opt1, struct Option* opt2)
{
    return strcmp(opt1->sh_name, opt2->sh_name);
}

struct Option* optByName(struct Option* opts, int n_opts, const char* sh_name)
{
    int lft = -1, rgt = n_opts, mid = 0;

    while (rgt - lft > 1)
    {
        mid = (lft + rgt) / 2;
        if (strcmp(opts[mid].sh_name, sh_name) >= 0)
            rgt = mid;
        else
            lft = mid;
    }

    return (strcmp(opts[rgt].sh_name, sh_name) == 0) ? opts + rgt : NULL;
}

ErrEnum testOpts(struct Option* opts, int n_opts)
{
    struct Option *opt = NULL;
    for (int j = 0; j < n_opts; ++j)
    {
        opt = optByName(opts, n_opts, opts[j].sh_name);
        if (opt == NULL)
            return ERR_INVAL_OPT;
        printf("option %s trig %d str_arg %s int_arg %d\n", opt->sh_name, opt->trig, opt->str_arg, opt->int_arg);
    }
    return ERR_OK;
}

ErrEnum parseOpts(int argc, const char* argv[], struct Option* opts, int n_opts)
{
    qsort(opts, n_opts, sizeof(struct Option), (voidcmp_f)optcmp);

    for (int j = 0; j < n_opts; ++j)
        opts[j].l_name_len = strlen(opts[j].l_name);

    int trig = 0;

    for (int i = 1; i < argc; ++i)
    {
        trig = 0;
        for (int j = 0; j < n_opts; ++j)
        {
            if (strcmp(argv[i], opts[j].sh_name) == 0)
            {
                if (i == argc - 1 || argv[i + 1][0] == '-')
                {
                    trig = opts[j].trig = 1;
                    break;
                }

                trig = opts[j].trig = 1;
                opts[j].str_arg = argv[++i];
                opts[j].int_arg = strtol(opts[j].str_arg, NULL, 10);
                break;
            }

            if (strncmp(argv[i], opts[j].l_name, opts[j].l_name_len) != 0)
                continue;

            if (argv[i][opts[j].l_name_len] == '\0')
            {
                trig = opts[j].trig = 1;
                break;
            }

            if (argv[i][opts[j].l_name_len] != '=' || argv[i][opts[j].l_name_len + 1] == '\0')
                return ERR_OPT_ARG_FMT;

            trig = opts[j].trig = 1;
            opts[j].str_arg = argv[i] + opts[j].l_name_len + 1;
            opts[j].int_arg = strtol(opts[j].str_arg, NULL, 10);
            break;
        }
        if (!trig)
            return ERR_INVAL_OPT;
    }
    return ERR_OK;
}