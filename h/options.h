#ifndef OPTIONS_H
#define OPTIONS_H

#include <my-error.h>

struct Option
{
    const char *sh_name, *l_name;

    const char *str_arg;
    int int_arg;
    int trig;
    
    int l_name_len;
};

int optcmp(struct Option* opt1, struct Option* opt2);
struct Option* optByName(struct Option* opts, int n_opts, const char* sh_name);
ErrEnum parseOpts(int argc, const char* argv[], struct Option* opts, int n_opts);
ErrEnum testOpts(struct Option* opts, int n_opts);

#endif //OPTIONS_H