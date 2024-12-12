#include <math.h>

#include <diff.h>
#include <logs.h>
#include <utils.h>
#include <tree-dsl.h>

static const int buffer_size = 300;

void evaluate(Node* node, int x, int* ans)
{
    myAssert(node != NULL);

    if (node->type == TYPE_NUM)
    {
        *ans = node->val.num;
        return;
    }
    if (node->type == TYPE_VAR)
    {
        *ans = x;
        return;
    }
    myAssert(node->type == TYPE_OP);

    #define OP_CODEGEN(name, n_operands, value, priority, text) \
    case OP_ ## name:                                           \
    {                                                           \
        int arg1 = 0, arg2 = 0;                              \
        evaluate(node->lft, x, &arg1);                          \
        if (n_operands == 2) evaluate(node->rgt, x, &arg2);     \
        *ans = value;                                           \
        return;                                                 \
    }

    switch (node->val.op_code)
    {
        #include <operations.h>
        default: myAssert(0);
    }

    return;
    #undef OP_CODEGEN
}

void nodeIsConst(Node* node, int* ans)
{
    myAssert(ans != NULL);
    if (node == NULL)
    {
        *ans = 1;
        return;
    }
    int ans_child = 0;
    nodeIsConst(node->lft, &ans_child);
    if (!ans_child)
    {
        *ans = 0;
        return;
    }
    nodeIsConst(node->rgt, &ans_child);
    if (!ans_child)
    {
        *ans = 0;
        return;
    }
    if (node->type == TYPE_VAR)
    {
        *ans = 0;
        return;
    }
    *ans = 1;
}

int simplifyCase(Node* node, NodeChild crit_child, int crit_val, int replace_with_num, int replacement)
{
    myAssert(node != NULL);

    Node *crit_node = NULL, *expr_node = NULL;
    if (crit_child == LFT_NODE)
    {
        crit_node = node->lft;
        expr_node = node->rgt;
    }
    else if (crit_child == RGT_NODE)
    {
        crit_node = node->rgt;
        expr_node = node->lft;
    }
    else myAssert(0);

    if (crit_node->type == TYPE_NUM && crit_node->val.num == crit_val)
    {
        if (replace_with_num)
        {
            node->type = TYPE_NUM;
            node->val.num = replacement;
            nodeDtor(node->lft);
            nodeDtor(node->rgt);
            node->lft = NULL;
            node->rgt = NULL;
            return 1;
        }
        if (!isZero(replacement - 1))
        {
            node->val.op_code = OP_MUL;
            crit_node->val.num = replacement;
            return 1;
        }
        node->type = expr_node->type;
        node->val = expr_node->val;
        node->lft = expr_node->lft;
        node->rgt = expr_node->rgt;

        expr_node->lft = expr_node->rgt = NULL;
        if (node->lft != NULL) node->lft->parent = node;
        if (node->rgt != NULL) node->rgt->parent = node;
        nodeDtor(expr_node);
        nodeDtor(crit_node);
        return 1;
    }
    return 0;
}

void simplify(Node* node)
{
    myAssert(node != NULL);

    if (node->type != TYPE_OP) return;
    simplify(node->lft);
    if (node->rgt != NULL) simplify(node->rgt);
    
    if (node->lft->type == TYPE_NUM && node->rgt != NULL && node->rgt->type == TYPE_NUM)
    {
        evaluate(node, 0, &(node->val.num));
        node->type = TYPE_NUM;
        nodeDtor(node->lft);
        nodeDtor(node->rgt);
        node->lft = NULL;
        node->rgt = NULL;
        return;
    }

    if (node->val.op_code == OP_ADD)
    {
        if (simplifyCase(node, LFT_NODE, 0, 0, 1)) return;
        if (simplifyCase(node, RGT_NODE, 0, 0, 1)) return;
    }
    else if (node->val.op_code == OP_SUB)
    {
        if (simplifyCase(node, LFT_NODE, 0, 0, -1)) return;
        if (simplifyCase(node, RGT_NODE, 0, 0, 1)) return;
    }
    else if (node->val.op_code == OP_MUL)
    {
        if (simplifyCase(node, LFT_NODE, 0, 1, 0)) return;
        if (simplifyCase(node, RGT_NODE, 0, 1, 0)) return;
        if (simplifyCase(node, LFT_NODE, 1, 0, 1)) return;
        if (simplifyCase(node, RGT_NODE, 1, 0, 1)) return;
    }
    else if (node->val.op_code == OP_DIV)
    {
        if (simplifyCase(node, RGT_NODE, 1, 0, 1)) return;
    }
    else if (node->val.op_code == OP_POW)
    {
        if (simplifyCase(node, RGT_NODE, 0, 1, 1)) return;
        if (simplifyCase(node, RGT_NODE, 1, 0, 1)) return;
        if (simplifyCase(node, LFT_NODE, 0, 1, 0)) return;
        if (simplifyCase(node, LFT_NODE, 1, 1, 1)) return;
    }
}