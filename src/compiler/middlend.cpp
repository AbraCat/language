#include <middlend.h>
#include <utils.h>

ErrEnum runMiddleEnd(Node *node)
{
    simplify(node);
    return ERR_OK;
}

void simplify(Node* node)
{
    if (node == NULL) return;
    simplify(node->lft);
    simplify(node->rgt);
    if (node->type != TYPE_OP) return;

    OpInfo *op_info = NULL;
    getOpByCode(node->val.op_code, &op_info);
    if (op_info->priority == 0) return;
    
    if (node->lft != NULL && node->lft->type == TYPE_NUM && node->rgt != NULL && node->rgt->type == TYPE_NUM)
    {
        int val = 0;
        evaluate(node, &val);
        node->type = TYPE_NUM;
        node->val.num = val;
        // nodeDtor(node->lft);
        // nodeDtor(node->rgt);
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
    // else if (node->val.op_code == OP_POW)
    // {
    //     if (simplifyCase(node, RGT_NODE, 0, 1, 1)) return;
    //     if (simplifyCase(node, RGT_NODE, 1, 0, 1)) return;
    //     if (simplifyCase(node, LFT_NODE, 0, 1, 0)) return;
    //     if (simplifyCase(node, LFT_NODE, 1, 1, 1)) return;
    // }
}

int simplifyCase(Node* node, NodeChild crit_child, int crit_val, bool replace_with_num, int replacement)
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

void evaluate(Node* node, int* ans)
{
    myAssert(node != NULL);

    if (node->type == TYPE_NUM)
    {
        *ans = node->val.num;
        return;
    }
    if (node->type == TYPE_VAR)
    {
        myAssert("evaluate() on non-const node" && 0);
        return;
    }
    myAssert(node->type == TYPE_OP);

    #define OP_CODEGEN(name, n_operands, value, priority, text, asm_instr) \
    case OP_ ## name:                                           \
    {                                                           \
        int arg1 = 0, arg2 = 0;                              \
        evaluate(node->lft, &arg1);                             \
        if (n_operands == 2) evaluate(node->rgt, &arg2);     \
        *ans = value;                                           \
        return;                                                 \
    }

    switch (node->val.op_code)
    {
        #include <operations.h>
        default: myAssert(0);
    }
    #undef OP_CODEGEN

    return;
}