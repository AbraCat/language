#ifndef TREE_DSL_H
#define TREE_DSL_H

#define _COPY(src) ({Node* noda = NULL; returnErr(nodeCopy(src, &noda)); noda;})

#define _EMPTY()                           ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_NUM, {.num = 0}, NULL, NULL, NULL)); noda;})
#define _NODE(type, val, parent, lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, type, val, parent, lft, rgt)); noda;})
#define _NUM(number)                       ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_NUM, {.num = number}, NULL, NULL, NULL)); noda;})
#define _VAR()                             ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_VAR, {.var_id = 0}, NULL, NULL, NULL)); noda;})

#define _ADD(lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_OP, {.op_code = OP_ADD}, NULL, lft, rgt)); noda;})
#define _SUB(lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_OP, {.op_code = OP_SUB}, NULL, lft, rgt)); noda;})
#define _MUL(lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_OP, {.op_code = OP_MUL}, NULL, lft, rgt)); noda;})
#define _DIV(lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_OP, {.op_code = OP_DIV}, NULL, lft, rgt)); noda;})
#define _POW(lft, rgt) ({Node* noda = NULL; returnErr(nodeCtor(&noda, TYPE_OP, {.op_code = OP_POW}, NULL, lft, rgt)); noda;})

#endif // TREE_DSL_H