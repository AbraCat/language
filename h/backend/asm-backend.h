#ifndef ASM_BACKEND
#define ASM_BACKEND

#include <my-error.h>
#include <tree.h>
#include <stdlib.h>
#include <tokenizer.h>

ErrEnum runAsmBackend(Node* tree, FILE* fout);

#endif // ASM_BACKEND