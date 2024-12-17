#include <stdlib.h>

#include <utils.h>
#include <colors.h>
#include <tree-dsl.h>

#include <frontend.h>
#include <tokenizer.h>

#define CUR_NODE (pars->s[pars->p])
#define INCR_P ++(pars->p)
#define START_RULE myAssert(pars != NULL && node != NULL && *node == NULL); int oldp = pars->p;
#define SYNT_ERR(exp) {pars->synt_err = {1, pars->p, exp, pars->s + pars->p}; pars->p = oldp; return ERR_OK;}
#define NO_SYNT_ERR_RET {pars->synt_err.err = 0; return ERR_OK;}
#define RET_IF_SYNT_ERR if (pars->synt_err.err) { pars->p = oldp; return ERR_OK; }

#define CALL_SYNT_ERR if (pars->synt_err.err) syntaxErr(&pars->synt_err)

typedef ErrEnum (*RuleFunc)(Parser*, Node**);

static void syntaxErr(SyntaxErr* synt_err);
static void undeclVars(NameArr* name_arr);

static ErrEnum getG(Parser* pars, Node** node);

static ErrEnum getFuncDecl(Parser* pars, Node** node);
static ErrEnum getFuncParam(Parser* pars, Node** node);
static ErrEnum getBody(Parser* pars, Node** node, bool allow_var_decl);
static ErrEnum getS(Parser* pars, Node** node);
static ErrEnum getSVarDecl(Parser* pars, Node** node);

static ErrEnum getIfLike(Parser* pars, Node** node, OpEnum op_code);

static ErrEnum getVarDecl(Parser* pars, Node** node);
static ErrEnum getIf(Parser* pars, Node** node);
static ErrEnum getWhile(Parser* pars, Node** node);
static ErrEnum getRet(Parser* pars, Node** node);
static ErrEnum getAssign(Parser* pars, Node** node);

static ErrEnum getE1(Parser* pars, Node** node);
static ErrEnum getE(Parser* pars, Node** node, int rule_num);
static ErrEnum getP(Parser* pars, Node** node);
static ErrEnum getVarInExpr(Parser* pars, Node** node); // unused

static ErrEnum getCommaSeparated(Parser* pars, Node** node,                   int* n_nodes, ErrEnum (*rule)(Parser*, Node**));
static ErrEnum getInBrackets    (Parser* pars, Node** node, bool allow_empty, int* n_nodes, ErrEnum (*rule)(Parser*, Node**));

ErrEnum runFrontend(const char* fin_name, Node** tree, Node** to_free, const char** prog_text)
{
    myAssert(tree != NULL && *tree == NULL && fin_name != NULL && to_free != NULL && *to_free == NULL);

    Parser pars = {};
    returnErr(nameArrCtor(&pars.name_arr));
    returnErr(tokenize(fin_name, &pars.s, &pars.n_nodes, &pars.name_arr, prog_text));

    // debug:
    // connectLinear(pars.s, pars.n_nodes);
    // returnErr(treeDump(pars.s));
    // return ERR_OK;

    returnErr(getG(&pars, &pars.root));
    if (pars.synt_err.err) syntaxErr(&pars.synt_err);

    // returnErr(nodeVerify(pars.root));
    // returnErr(treeDump(pars.root));

    *tree = pars.root;
    *to_free = pars.s;

    return ERR_OK;
}

void syntaxErr(SyntaxErr* synt_err)
{
    myAssert(synt_err != NULL && synt_err->err);

    printf("%sSyntax error at pos %d (val %d): %s%s\n", 
    RED_STR, synt_err->pos, synt_err->got->val.num, synt_err->exp, DEFAULT_STR);
    exit(0);
}

static void undeclVars(NameArr* name_arr)
{
    myAssert(name_arr != NULL);
    for (int ind = 0; ind < name_arr->n_names; ++ind)
        if (name_arr->names[ind].type == NAME_VAR)
            name_arr->names[ind].type = NAME_UNDECL;
}

ErrEnum getG(Parser* pars, Node** node)
{
    myAssert(pars != NULL && node != NULL && *node == NULL);
    START_RULE;

    returnErr(getCommaSeparated(pars, node, NULL, getFuncDecl));
    RET_IF_SYNT_ERR;
    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_END)
        SYNT_ERR("$");
    INCR_P;
    return ERR_OK;
}

static ErrEnum getCommaSeparated(Parser* pars, Node** node, int* n_nodes, ErrEnum (*rule)(Parser*, Node**))
{
    myAssert(pars != NULL && node != NULL && *node == NULL && rule != NULL);
    START_RULE;

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


static ErrEnum getInBrackets(Parser* pars, Node** node, bool allow_empty, int* n_nodes, ErrEnum (*rule)(Parser*, Node**))
{
    START_RULE;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_OPEN_BRACKET) SYNT_ERR("(");
    INCR_P;
    if (allow_empty && CUR_NODE.type == TYPE_OP && CUR_NODE.val.op_code == OP_CLOSE_BRACKET)
    {
        if (n_nodes != NULL) *n_nodes = 0;
        INCR_P;
        NO_SYNT_ERR_RET;
    }
    returnErr(getCommaSeparated(pars, node, n_nodes, rule));
    RET_IF_SYNT_ERR;
    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_CLOSE_BRACKET) SYNT_ERR(")");
    INCR_P;
    NO_SYNT_ERR_RET;
}

static ErrEnum getFuncDecl(Parser* pars, Node** node)
{
    START_RULE;
    pars->var_cnt = 0;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_FUNC) SYNT_ERR("func");
    Node *op_func_node = &CUR_NODE;
    INCR_P;

    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("FUNC NAME");
    Name* name_struct = pars->name_arr.names + CUR_NODE.val.var_id;
    if (name_struct->type == NAME_VAR) SYNT_ERR("VAR AND FUNC WITH SAME NAME"); // can only happen with gloabl vars which are not supported yet
    if (name_struct->type == NAME_UNDECL) name_struct->type = NAME_FUNC;
    CUR_NODE.type = TYPE_FUNC;
    CUR_NODE.val.func_name = name_struct->name_str;
    Node *func_name_node = &CUR_NODE;
    INCR_P;

    Node* bracket_node = &CUR_NODE;
    int n_args = -1;
    returnErr(getInBrackets(pars, &bracket_node->lft, 1, &n_args, getFuncParam));
    RET_IF_SYNT_ERR;
    if (name_struct->n_args >= 0 && n_args != name_struct->n_args) SYNT_ERR("SAME FUNC WITH DIFFERENT NUM OF ARGS");
    if (name_struct->n_args < 0) name_struct->n_args = n_args;

    returnErr(getBody(pars, &bracket_node->rgt, 1));
    RET_IF_SYNT_ERR;

    if (bracket_node->lft != NULL) bracket_node->lft->parent = bracket_node;
    if (bracket_node->rgt != NULL) bracket_node->rgt->parent = bracket_node;
    op_func_node->lft = func_name_node;
    op_func_node->rgt = bracket_node;
    func_name_node->parent = op_func_node;
    bracket_node->parent = op_func_node;
    *node = op_func_node;

    undeclVars(&pars->name_arr);
    NO_SYNT_ERR_RET;
}

static ErrEnum getFuncParam(Parser* pars, Node** node)
{
    START_RULE;
    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("VAR NAME");
    Name* name_struct = pars->name_arr.names + CUR_NODE.val.var_id;
    if (name_struct->type != NAME_UNDECL) SYNT_ERR("NAME ALREADY USED");
    name_struct->type = NAME_VAR;
    name_struct->var_id = CUR_NODE.val.var_id = pars->var_cnt++;
    *node = &CUR_NODE;
    INCR_P;
    NO_SYNT_ERR_RET;
}

static ErrEnum getBody(Parser* pars, Node** node, bool allow_var_decl)
{
    if (allow_var_decl) return getInBrackets(pars, node, 1, NULL, getSVarDecl);
    return getInBrackets(pars, node, 1, NULL, getS);
}

static ErrEnum getS(Parser* pars, Node** node)
{
    START_RULE;

    returnErr(getIf(pars, node));
    if (pars->synt_err.err == 0) return ERR_OK;
    returnErr(getWhile(pars, node));
    if (pars->synt_err.err == 0) return ERR_OK;
    returnErr(getRet(pars, node));
    if (pars->synt_err.err == 0) return ERR_OK;
    returnErr(getAssign(pars, node));
    if (pars->synt_err.err == 0) return ERR_OK;
    returnErr(getE1(pars, node));
    return ERR_OK;
}

static ErrEnum getSVarDecl(Parser* pars, Node** node)
{
    START_RULE;

    returnErr(getVarDecl(pars, node));
    if (pars->synt_err.err == 0) return ERR_OK;
    returnErr(getS(pars, node));
    return ERR_OK;
}

static ErrEnum getVarDecl(Parser* pars, Node** node)
{
    START_RULE;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_VAR) SYNT_ERR("var");
    Node* op_var_node = &CUR_NODE;
    INCR_P;

    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("VAR NAME");
    Name* name_struct = pars->name_arr.names + CUR_NODE.val.var_id;
    if (name_struct->type != NAME_UNDECL) SYNT_ERR("NAME ALREADY USED");
    name_struct->type = NAME_VAR;
    name_struct->var_id = CUR_NODE.val.var_id = pars->var_cnt++;
    op_var_node->lft = &CUR_NODE;
    INCR_P;

    *node = op_var_node;
    NO_SYNT_ERR_RET;
}

static ErrEnum getIfLike(Parser* pars, Node** node, OpEnum op_code)
{
    START_RULE;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != op_code) SYNT_ERR("operator");
    Node* if_node = &CUR_NODE;
    INCR_P;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_OPEN_BRACKET) SYNT_ERR("(");
    Node *open_bracket_node = &CUR_NODE;
    INCR_P;

    returnErr(getE1(pars, &if_node->lft));
    RET_IF_SYNT_ERR;
    if_node->lft->parent = if_node;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_CLOSE_BRACKET) SYNT_ERR(")");
    INCR_P;

    returnErr(getBody(pars, &if_node->rgt, 0));
    RET_IF_SYNT_ERR;
    if (if_node->rgt != NULL) if_node->rgt->parent = if_node;

    *node = if_node;
    NO_SYNT_ERR_RET;
}

static ErrEnum getIf(Parser* pars, Node** node)
{
    START_RULE;

    Node* if_node = NULL;
    returnErr(getIfLike(pars, &if_node, OP_IF));
    RET_IF_SYNT_ERR;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_OPEN_BRACKET)
    {
        *node = if_node;
        NO_SYNT_ERR_RET;
    }

    Node *else_body = NULL;
    returnErr(getBody(pars, &else_body, 0));
    RET_IF_SYNT_ERR;

    Node *open_bracket_node = if_node + 1, *if_body = if_node->rgt;
    if_node->rgt = open_bracket_node;
    open_bracket_node->parent = if_node;
    open_bracket_node->lft = if_body;
    if (if_body != NULL) if_body->parent = open_bracket_node;
    open_bracket_node->rgt = else_body;
    if (else_body != NULL) else_body->parent = open_bracket_node;
    
    *node = if_node;
    NO_SYNT_ERR_RET;
}

static ErrEnum getWhile(Parser* pars, Node** node)
{
    returnErr(getIfLike(pars, node, OP_WHILE));
}

static ErrEnum getRet(Parser* pars, Node** node)
{
    START_RULE;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_RET) SYNT_ERR("return");
    Node *ret_node = &CUR_NODE;
    INCR_P;

    returnErr(getE1(pars, &ret_node->lft));
    RET_IF_SYNT_ERR;
    ret_node->lft->parent = ret_node;

    *node = ret_node;
    NO_SYNT_ERR_RET;
}

static ErrEnum getAssign(Parser* pars, Node** node)
{
    START_RULE;

    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("VAR NAME");
    Name* name_struct = pars->name_arr.names + CUR_NODE.val.var_id;
    if (name_struct->type != NAME_VAR) SYNT_ERR("NO VAR WITH SUCH NAME");
    Node* var_node = &CUR_NODE;
    INCR_P;

    if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_ASSIGN) SYNT_ERR("=");
    Node* assign_node = &CUR_NODE;
    INCR_P;

    returnErr(getE1(pars, &assign_node->rgt));
    RET_IF_SYNT_ERR;
    var_node->val.var_id = name_struct->var_id;
    assign_node->lft = var_node;
    var_node->parent = assign_node;
    assign_node->rgt->parent = assign_node;

    *node = assign_node;
    NO_SYNT_ERR_RET;
}

ErrEnum getE1(Parser* pars, Node** node) { return getE(pars, node, 1); }

ErrEnum getE(Parser* pars, Node** node, int rule_num)
{
    const int max_rule_num = 4;
    myAssert(pars != NULL && node != NULL && *node == NULL);
    myAssert(0 <= rule_num || rule_num <= max_rule_num);
    START_RULE;

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
    }
    NO_SYNT_ERR_RET;
}

ErrEnum getP(Parser* pars, Node** node)
{
    myAssert(pars != NULL && node != NULL && *node == NULL);
    START_RULE;

    if (CUR_NODE.type == TYPE_OP && CUR_NODE.val.op_code == OP_OPEN_BRACKET)
    {
        INCR_P;
        returnErr(getE(pars, node, 1));
        RET_IF_SYNT_ERR;
        if (CUR_NODE.type != TYPE_OP || CUR_NODE.val.op_code != OP_CLOSE_BRACKET) SYNT_ERR(")");
        INCR_P;
        NO_SYNT_ERR_RET;
    }
    if (CUR_NODE.type == TYPE_NUM)
    {
        *node = &CUR_NODE;
        INCR_P;
        NO_SYNT_ERR_RET;
    }
    if (CUR_NODE.type == TYPE_OP && CUR_NODE.val.op_code == OP_SUB && pars->s[pars->p + 1].type == TYPE_NUM)
    {
        INCR_P;
        CUR_NODE.val.num *= -1;
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
        name_struct->type = NAME_FUNC;
        Node* func_node = &CUR_NODE;
        INCR_P;

        int n_args = -1;
        returnErr(getInBrackets(pars, &func_node->lft, 1, &n_args, getE1));
        RET_IF_SYNT_ERR;
        if (name_struct->n_args >= 0 && name_struct->n_args != n_args) SYNT_ERR("SAME FUNC WITH DIFFERENT NUM OF ARGS");
        if (name_struct->n_args < 0) name_struct->n_args = n_args;
        if (func_node->lft != NULL) func_node->lft->parent = func_node;
        *node = func_node;
        NO_SYNT_ERR_RET;
    }
    if (name_struct->type != NAME_VAR) {printf("aaa %s %d\n", name_struct->name_str, (int)(name_struct->type)); SYNT_ERR("NO VAR WITH SUCH NAME");}
    CUR_NODE.val.var_id = name_struct->var_id;
    *node = &CUR_NODE;
    INCR_P;
    NO_SYNT_ERR_RET;
}

static ErrEnum getVarInExpr(Parser* pars, Node** node)
{
    START_RULE;
    if (CUR_NODE.type != TYPE_VAR) SYNT_ERR("VAR NAME");

    return ERR_OK;
}