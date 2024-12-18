#ifndef MIDDLEND_H
#define MIDDLEND_H

#include <error.h>
#include <tree.h>

ErrEnum runMiddleEnd(Node *node);

int simplifyCase(Node* node, NodeChild crit_child, int crit_val, bool replace_with_num, int replacement);
void simplify(Node* node);
void evaluate(Node* node, int* ans);

#endif // MIDDLEND_H