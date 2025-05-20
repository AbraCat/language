#include <backend.h>
#include <standardlib.h>

static NameArr* namearr = NULL;
int n_vars = 0;

static ErrEnum addFunc(FILE* fout, Node* node);

ErrEnum fillFuncArr(NameArr** name_arr, Node* node)
{
    returnErr(nameArrCtor(name_arr));
    namearr = *name_arr;

    returnErr(insertStdNames(namearr));
    returnErr(compileCommaSeparated(NULL, node, addFunc));

    return ERR_OK;
}

static ErrEnum addFunc(FILE* fout, Node* node)
{
    myAssert(node != NULL && node->type == TYPE_OP && node->val.op_code == OP_FUNC);
    myAssert(node->lft != NULL && node->lft->type == TYPE_FUNC && node->lft->val.func_name != NULL);
    // printName(fout, node->lft->val.func_name);
    int name_id = -1;
    returnErr(insertName(node->lft->val.func_name, namearr, &name_id));
    namearr->names[name_id].type = NAME_FUNC;

    n_vars = 0;
    myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    if (node->rgt->lft != NULL) returnErr(compileCommaSeparated(fout, node->rgt->lft, checkFuncParam));
    namearr->names[name_id].n_args = n_vars;
    return ERR_OK;
}

ErrEnum checkFuncParam(FILE* fout, Node* node)
{
    myAssert(node != NULL);
    myAssert(node->type == TYPE_VAR);
    myAssert(node->val.var_id == n_vars);
    ++n_vars;
    return ERR_OK;
}

ErrEnum compileCommaSeparated(FILE* fout, Node* node, ErrEnum (*compile)(FILE*, Node*))
{
    myAssert(node != NULL);

    Node* cur_node = node;
    while (cur_node != NULL && cur_node->type == TYPE_OP && cur_node->val.op_code == OP_COMMA) cur_node = cur_node->lft;
    returnErr(compile(fout, cur_node));
    if (cur_node == node) return ERR_OK;

    while (1)
    {
        cur_node = cur_node->parent;
        returnErr(compile(fout, cur_node->rgt));
        if (cur_node == node) break;
    }
    return ERR_OK;
}