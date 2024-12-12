#ifndef DIFF_H
#define DIFF_H

#include <tree.h>

void evaluate(Node* node, int x, int* ans);
void nodeIsConst(Node* node, int* ans);
int simplifyCase(Node* node, NodeChild crit_child, double crit_val, int replace_with_num, int replacement);
void simplify(Node* node);

#endif // DIFF_H