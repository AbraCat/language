#ifndef SPU_BACKEND_H
#define SPU_BACKEND_H

#include <my-error.h>
#include <tree.h>

ErrEnum runBackend(Node* tree, FILE* fout);

#endif // SPU_BACKEND_H