#ifndef BACKEND_H
#define BACKEND_H

#include <error.h>
#include <tree.h>

ErrEnum runBackend(Node* tree, FILE* fout);

#endif // BACKEND_H