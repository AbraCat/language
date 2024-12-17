#ifndef ANTIFRONTEND_H
#define ANTIFRONTEND_H

#include <error.h>
#include <tree.h>

ErrEnum runAntiFrontend(Node* tree, const char* fout_name);

#endif // ANTIFRONTEND_H