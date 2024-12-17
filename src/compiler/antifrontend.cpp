

#include <antifrontend.h>
#include <str.h>

static ErrEnum decompG(FILE* fout, Node* node);
static ErrEnum decompInBrackets(FILE* fout, Node* node, ErrEnum (*rule)(FILE*, Node*));
static ErrEnum decompInBracketsNewline(FILE* fout, Node* node, ErrEnum (*rule)(FILE*, Node*));
static ErrEnum decompCommaSeparated(FILE* fout, Node* node, const char sep, ErrEnum (*rule)(FILE*, Node*));
static ErrEnum decompFuncDecl(FILE* fout, Node* node);

static ErrEnum decompS(FILE* fout, Node* node);
static ErrEnum decompE1(FILE* fout, Node* node);
static ErrEnum decompE(FILE* fout, Node* node, int rule_num);
static ErrEnum decompVar(FILE* fout, Node* node);

ErrEnum runAntiFrontend(Node* tree, const char* fout_name)
{
    myAssert(fout_name != NULL);

    FILE* fout = fopen(fout_name, "w");
    returnErr(decompG(fout, tree));
    fclose(fout);
    return ERR_OK;
}

static ErrEnum decompG(FILE* fout, Node* node)
{
    return decompCommaSeparated(fout, node, '\n', decompFuncDecl);
}

static ErrEnum decompCommaSeparated(FILE* fout, Node* node, const char sep, ErrEnum (*rule)(FILE*, Node*))
{
    Node *cur_node = node;
    while (cur_node != NULL && cur_node->type == TYPE_OP && cur_node->val.op_code == OP_COMMA) cur_node = cur_node->lft;
    returnErr(rule(fout, cur_node));
    if (cur_node == node) return ERR_OK;
    while (1)
    {
        fprintf(fout, ",%c", sep);
        cur_node = cur_node->parent;
        returnErr(rule(fout, cur_node->rgt));
        if (cur_node == node) break;
    }
    return ERR_OK;
}

static ErrEnum decompInBrackets(FILE* fout, Node* node, ErrEnum (*rule)(FILE*, Node*))
{
    fputc('(', fout);
    if (node != NULL) returnErr(decompCommaSeparated(fout, node, ' ', rule));
    fputc(')', fout);
    return ERR_OK;
}

static ErrEnum decompInBracketsNewline(FILE* fout, Node* node, ErrEnum (*rule)(FILE*, Node*))
{
    fputs("\n(\n", fout);
    if (node != NULL) returnErr(decompCommaSeparated(fout, node, '\n', rule));
    fputs("\n)", fout);
    return ERR_OK;
}

static ErrEnum decompFuncDecl(FILE* fout, Node* node)
{
    if (node == NULL || node->type != TYPE_OP && node->val.op_code != OP_FUNC ||
    node->lft == NULL || node->lft->type != TYPE_FUNC || 
    node->rgt == NULL || node->rgt->type != TYPE_OP || node->rgt->val.op_code != OP_OPEN_BRACKET) return ERR_INVAL_TREE;

    fputs("func ", fout);
    printName(fout, node->lft->val.func_name);
    returnErr(decompInBrackets(fout, node->rgt->lft, decompVar));
    returnErr(decompInBracketsNewline(fout, node->rgt->rgt, decompS));
    return ERR_OK;
}

static ErrEnum decompS(FILE* fout, Node* node)
{
    if (node == NULL) return ERR_INVAL_TREE;
    if (node->type == TYPE_OP) switch (node->val.op_code)
    {
        case OP_VAR:
            if (node->lft == NULL || node->lft->type != TYPE_VAR) return ERR_INVAL_TREE;
            fprintf(fout, "var v%d", node->lft->val.var_id);
            return ERR_OK;
        case OP_IF:
            if (node->lft == NULL || node->rgt == NULL) return ERR_INVAL_TREE;
            fputs("if (", fout);
            returnErr(decompE1(fout, node->lft));
            fputc(')', fout);
            if (node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET)
            {
                returnErr(decompInBracketsNewline(fout, node->rgt->lft, decompS));
                returnErr(decompInBracketsNewline(fout, node->rgt->rgt, decompS));
            }
            else returnErr(decompInBracketsNewline(fout, node->rgt, decompS));
            return ERR_OK;
        case OP_WHILE:
            if (node->lft == NULL || node->rgt == NULL) return ERR_INVAL_TREE;
            fputs("while (", fout);
            returnErr(decompE1(fout, node->lft));
            fputc(')', fout);
            returnErr(decompInBracketsNewline(fout, node->rgt, decompS));
            return ERR_OK;
        case OP_RET:
            fputs("return ", fout);
            returnErr(decompE1(fout, node->lft));
            return ERR_OK;
        case OP_ASSIGN:
            if (node->lft == NULL || node->lft->type != TYPE_VAR) return ERR_INVAL_TREE;
            fprintf(fout, "v%d = ", node->lft->val.var_id);
            returnErr(decompE1(fout, node->rgt));
            return ERR_OK;
        default: break;
    }
    return decompE1(fout, node);
}

static ErrEnum decompVar(FILE* fout, Node* node)
{
    if (node == NULL || node->type != TYPE_VAR) return ERR_INVAL_TREE;
    fprintf(fout, "v%d", node->val.var_id);
    return ERR_OK;
}

static ErrEnum decompE1(FILE* fout, Node* node)
{
    return decompE(fout, node, 1);
}

static ErrEnum decompE(FILE* fout, Node* node, int parent_priority)
{
    myAssert(parent_priority >= 1 && parent_priority <= 4);
    if (node == NULL) return ERR_INVAL_TREE;

    switch (node->type)
    {
        case TYPE_NUM:
            fprintf(fout, "%d", node->val.num);
            return ERR_OK;
        case TYPE_VAR:
            fprintf(fout, "v%d", node->val.var_id);
            return ERR_OK;
        case TYPE_FUNC:
            printName(fout, node->val.func_name);
            returnErr(decompInBrackets(fout, node->lft, decompE1));
            return ERR_OK;
        default: break;
    }

    OpInfo* op_info = NULL;
    returnErr(getOpByCode(node->val.op_code, &op_info));
    int priority = op_info->priority;
    if (priority == 0) return ERR_INVAL_TREE;
    int brackets = parent_priority > op_info->priority;

    if (brackets) fputc('(', fout);
    returnErr(decompE(fout, node->lft, priority));
    fprintf(fout, " %s ", op_info->op_str);
    if (node->val.op_code == OP_SUB || node->val.op_code == OP_DIV) ++priority;
    returnErr(decompE(fout, node->rgt, priority));
    if (brackets) fputc(')', fout);

    return ERR_OK;
}