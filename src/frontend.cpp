#include <stdlib.h>

#include <utils.h>
#include <colors.h>
#include <tree-dsl.h>

#include <frontend.h>
#include <tokenizer.h>

#define CUR_NODE (pars->s[pars->p])
#define INCR_P ++(pars->p)
#define SYNT_ERR(exp) {pars->synt_err = {1, pars->p, exp, pars->s + pars->p}; return ERR_OK;}
#define NO_SYNT_ERR_RET {pars->synt_err.err = 0; return ERR_OK;}
#define RET_IF_SYNT_ERR if (pars->synt_err.err) return ERR_OK

#define CALL_SYNT_ERR if (pars->synt_err.err) syntaxErr(&pars->synt_err)

static void syntaxErr(SyntaxErr* synt_err);

static ErrEnum getG(Parser* pars, Node** node);
static ErrEnum getFdecl(Parser* pars, Node** node);
static ErrEnum getE1(Parser* pars, Node** node);
static ErrEnum getE(Parser* pars, Node** node, int rule_num);
static ErrEnum getP(Parser* pars, Node** node);

static ErrEnum getCommaSeparated(Parser* pars, Node** node, int* n_nodes, ErrEnum (*rule)(Parser*, Node**));

ErrEnum runFrontend(const char* fin_name, Node** tree)
{
    myAssert(tree != NULL && *tree == NULL && fin_name != NULL);

    Parser pars = {};
    returnErr(nameArrCtor(&pars.name_arr));
    returnErr(tokenize(fin_name, &pars.s, &pars.n_nodes, &pars.name_arr));

    // debug:
    // connectLinear(pars.s, pars.n_nodes);
    // returnErr(treeDump(pars.s));
    // return ERR_OK;

    returnErr(getG(&pars, &pars.root));
    if (pars.synt_err.err) syntaxErr(&pars.synt_err);

    returnErr(treeDump(pars.root));
    *tree = pars.root;

    // nodeDtor(pars.s);
    nodeDtor(pars.root);

    return ERR_OK;
}

void syntaxErr(SyntaxErr* synt_err)
{
    myAssert(synt_err != NULL && synt_err->err);

    printf("%sSyntax error at pos %d: expected %s, got %d%s\n", 
    RED_STR, synt_err->pos, synt_err->exp, synt_err->got->val.num, DEFAULT_STR);
    exit(0);
}

ErrEnum getG(Parser* pars, Node** node)
{
    myAssert(pars != NULL && node != NULL && *node == NULL);

    returnErr(getE(pars, node, 1));
    RET_IF_SYNT_ERR;
    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_END)
        SYNT_ERR("$");
    INCR_P;
    return ERR_OK;
}

static ErrEnum getCommaSeparated(Parser* pars, Node** node, int* n_nodes, ErrEnum (*rule)(Parser*, Node**))
{
    myAssert(pars != NULL && node != NULL && *node == NULL && rule != NULL);
    returnErr(rule(pars, node));
    RET_IF_SYNT_ERR;
    int node_cnt = 1;
    while (1)
    {
        if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_COMMA)
            break;
        Node *lft = *node, *op = &CUR_NODE, *rgt = NULL;
        INCR_P;
        returnErr(rule(pars, &rgt));
        RET_IF_SYNT_ERR;
        op->lft = lft;
        op->rgt = rgt;
        lft->parent = op;
        rgt->parent = op;
        *node = op;
        ++node_cnt;
    }
    if (n_nodes != NULL) *n_nodes = node_cnt;
    NO_SYNT_ERR_RET;
}

static ErrEnum getFdecl(Parser* pars, Node** node)
{
    return ERR_OK;
}

ErrEnum getE1(Parser* pars, Node** node) { return getE(pars, node, 1); }

ErrEnum getE(Parser* pars, Node** node, int rule_num)
{
    const int max_rule_num = 4;
    myAssert(pars != NULL && node != NULL && *node == NULL);
    myAssert(0 <= rule_num || rule_num <= max_rule_num);

    if (rule_num == max_rule_num) returnErr(getP(pars, node))
    else returnErr(getE(pars, node, rule_num + 1));
    RET_IF_SYNT_ERR;

    while (1)
    {
        OpEnum op_code = OP_ADD;
        #define OP_CODEGEN(name, n_operands, value, priority, text) \
            if (priority == rule_num && CUR_NODE.type == TYPE_OP && CUR_NODE.val.op_code == OP_ ## name) op_code = OP_ ## name; else
        #include <operations.h>
        break;
        #undef OP_CODEGEN

        Node *lft = *node, *op = &CUR_NODE, *rgt = NULL;
        INCR_P;
        if (rule_num == max_rule_num)
        {
            returnErr(getP(pars, &rgt));
            RET_IF_SYNT_ERR;
        }
        else returnErr(getE(pars, &rgt, rule_num + 1));
        RET_IF_SYNT_ERR;

        op->lft = lft;
        op->rgt = rgt;
        lft->parent = op;
        rgt->parent = op;
        *node = op;

        // (*node)->parent = _NODE(TYPE_OP, {.op_code = op_code}, NULL, *node, NULL);
        // *node = (*node)->parent;
        // INCR_P;

        // if (rule_num == max_rule_num)
        // {
        //     returnErr(getP(pars, &(*node)->rgt));
        //     RET_IF_SYNT_ERR;
        // }
        // else returnErr(getE(pars, &(*node)->rgt, rule_num + 1));
        // RET_IF_SYNT_ERR;

        // (*node)->rgt->parent = *node;
    }
    NO_SYNT_ERR_RET;
}

ErrEnum getP(Parser* pars, Node** node)
{
    myAssert(pars != NULL && node != NULL && *node == NULL);

    if (CUR_NODE.type == TYPE_OP && CUR_NODE.val.op_code == OP_OPEN_BRACKET)
    {
        INCR_P;
        returnErr(getE(pars, node, 1));
        RET_IF_SYNT_ERR;
        if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_CLOSE_BRACKET) SYNT_ERR(")");
        INCR_P;
        NO_SYNT_ERR_RET;
    }
    // (OP_SUB) (NUM) is a number
    if (CUR_NODE.type == TYPE_NUM)
    {
        *node = &CUR_NODE;
        INCR_P;
        NO_SYNT_ERR_RET;
    }
    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("VAR/FUNC NAME");

    Name* name_struct = pars->name_arr.names + CUR_NODE.val.var_id;
    if (pars->s[pars->p + 1].type == TYPE_OP && pars->s[pars->p + 1].val.op_code == OP_OPEN_BRACKET)
    {
        if (name_struct->type == NAME_VAR) SYNT_ERR("VAR AND FUNC WITH SAME NAME");
        CUR_NODE.type = TYPE_FUNC;
        CUR_NODE.val.func_name = name_struct->name_str;
        Node* func_node = &CUR_NODE;
        INCR_P;
        INCR_P;
        int n_args = 0;
        returnErr(getCommaSeparated(pars, &func_node->lft, &n_args, getE1));
        func_node->lft->parent = func_node;
        RET_IF_SYNT_ERR;
        if (name_struct->n_args >= 0 && name_struct->n_args != n_args) SYNT_ERR("SAME FUNC WITH DIFFERENT NUM OF ARGS");
        if (name_struct->n_args < 0) name_struct->n_args = n_args;

        if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_CLOSE_BRACKET) SYNT_ERR(")");
        INCR_P;
        name_struct->type = NAME_FUNC;
        *node = func_node;
        NO_SYNT_ERR_RET;
    }
    if (name_struct->type == NAME_FUNC) SYNT_ERR("NO VAR WITH SUCH NAME"); // if (name_struct->type != NAME_VAR)
    *node = &CUR_NODE;
    INCR_P;
    NO_SYNT_ERR_RET;



    // returnErr(getN(pars, node));
    // if (pars->synt_err.err == 0) return ERR_OK;
    // returnErr(getV(pars, node));
    // if (pars->synt_err.err == 0) return ERR_OK;
    // returnErr(getF(pars, node));
    // return ERR_OK;
}