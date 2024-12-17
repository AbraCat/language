

#include <backend.h>
#include <str.h>

static ErrEnum compileCommaSeparated(FILE* fout, Node* node, ErrEnum (*compile)(FILE*, Node*));
static ErrEnum compileFuncDecl(FILE* fout, Node* node);
static ErrEnum compileParamList(FILE* fout, Node* node); // not used
static ErrEnum checkFuncParam(FILE* fout, Node* node);
static ErrEnum compileBody(FILE* fout, Node* node);
static ErrEnum compileS(FILE* fout, Node* node);

static ErrEnum compileIf(FILE* fout, Node* node);
static ErrEnum compileWhile(FILE* fout, Node* node);
static ErrEnum compileE(FILE* fout, Node* node);

const int st_size = 400;
int label_cnt = 0, n_vars = 0;
const char trash_reg[] = "AX", ret_val_reg[] = "BX", frame_adr_reg[] = "CX", st2_pos_reg[] = "DX";

ErrEnum runBackend(Node* tree, FILE* fout)
{
    myAssert(tree != NULL && fout != NULL);
    fprintf(fout, "PUSH 0\nPOP %s\nPUSH 0\nPOP %s\nCALL main:\nHLT\ninput:\nIN\nRET\noutput:\nOUT\nPUSH 0\nRET\nsqrt:\nSQRT\nRET\n", 
    st2_pos_reg, frame_adr_reg);

    return compileCommaSeparated(fout, tree, compileFuncDecl);
}

static ErrEnum compileCommaSeparated(FILE* fout, Node* node, ErrEnum (*compile)(FILE*, Node*))
{
    myAssert(fout != NULL && node != NULL);

    Node* cur_node = node;
    while (cur_node->type == TYPE_OP && cur_node->val.op_code == OP_COMMA) cur_node = cur_node->lft;
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

static ErrEnum compileFuncDecl(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->type == TYPE_OP && node->val.op_code == OP_FUNC);

    myAssert(node->lft != NULL && node->lft->type == TYPE_FUNC && node->lft->val.func_name != NULL);
    printName(fout, node->lft->val.func_name);
    fputs(":\n", fout);

    n_vars = 0;
    myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    if (node->rgt->lft != NULL) returnErr(compileCommaSeparated(fout, node->rgt->lft, checkFuncParam));
    fprintf(fout, "PUSH %s\nPUSH 1\nSUB\nPOP %s\n" "SIZE\nPUSH %d\nSUB\nPOP %s\nPUSH %s\nPOP [%s+%d]\n", 
    st2_pos_reg, st2_pos_reg, n_vars, frame_adr_reg, frame_adr_reg, st2_pos_reg, st_size);
    returnErr(compileBody(fout, node->rgt->rgt));

    for (int cnt = 0; cnt < n_vars; ++cnt)
        fprintf(fout, "POP %s\n", trash_reg);
    fprintf(fout, "PUSH 1\nPUSH %s\nADD\nPOP %s\nPUSH 0\nRET\n", st2_pos_reg, st2_pos_reg);
    return ERR_OK;
}

static ErrEnum checkFuncParam(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);
    myAssert(node->type == TYPE_VAR);
    myAssert(node->val.var_id == n_vars);
    ++n_vars;
    return ERR_OK;
}

static ErrEnum compileParamList(FILE* fout, Node* node)
{
    // not used
    myAssert(fout != NULL && node != NULL);

    Node* cur_node = node;
    while (cur_node->type == TYPE_OP && cur_node->val.op_code == OP_COMMA)
    {
        myAssert(cur_node->rgt != NULL && cur_node->rgt->type == TYPE_VAR);
        fprintf(fout, "POP [%d]\n", cur_node->rgt->val.var_id);
        cur_node = cur_node->lft;
    }
    myAssert(cur_node != NULL && cur_node->type == TYPE_VAR);
    fprintf(fout, "POP [%d]\n", cur_node->val.var_id);

    return ERR_OK;
}

static ErrEnum compileBody(FILE* fout, Node* node)
{
    myAssert(fout != NULL);
    if (node != NULL) returnErr(compileCommaSeparated(fout, node, compileS));
    return ERR_OK;
}

static ErrEnum compileS(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_OP) switch (node->val.op_code)
    {
        case OP_VAR:
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR);
            myAssert(node->lft->val.var_id == n_vars);
            ++n_vars;
            fprintf(fout, "PUSH 0 ; var %d declared\n", node->lft->val.var_id);
            return ERR_OK;
        case OP_IF:
            returnErr(compileIf(fout, node));
            return ERR_OK;
        case OP_WHILE:
            returnErr(compileWhile(fout, node));
            return ERR_OK;
        case OP_RET:
            myAssert(node->lft != NULL);
            returnErr(compileE(fout, node->lft));
            fprintf(fout, "POP %s\n", ret_val_reg);
            for (int ind = 0; ind < n_vars; ++ind)
                fprintf(fout, "POP %s\n", trash_reg);
            fprintf(fout, "PUSH %s\nPUSH 1\nADD\nPOP %s\n" "PUSH [%s+%d]\nPOP %s\n" "PUSH %s\nRET\n", 
            st2_pos_reg, st2_pos_reg, st2_pos_reg, st_size, frame_adr_reg, ret_val_reg);
            return ERR_OK;
        case OP_ASSIGN:
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR && node->rgt != NULL);
            returnErr(compileE(fout, node->rgt));
            fprintf(fout, "POP [%s+%d]\n", frame_adr_reg, node->lft->val.var_id);
            return ERR_OK;
        default:
            break;
    }
    returnErr(compileE(fout, node));
    fprintf(fout, "POP AX\n");
    return ERR_OK;
}

static ErrEnum compileIf(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;

    returnErr(compileE(fout, node->lft));
    fprintf(fout, "PUSH 0\nJE else_%d:\n", label_num);

    if (node->rgt->type != TYPE_OP || node->rgt->val.op_code != OP_OPEN_BRACKET)
    {
        returnErr(compileBody(fout, node->rgt));
        fprintf(fout, "else_%d:\n", label_num);
        return ERR_OK;
    }

    returnErr(compileBody(fout, node->rgt->lft));
    fprintf(fout, "JMP if_end_%d:\nelse_%d:\n", label_num, label_num);
    returnErr(compileBody(fout, node->rgt->rgt));
    fprintf(fout, "if_end_%d:\n", label_num);
    return ERR_OK;
}

static ErrEnum compileWhile(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;

    fprintf(fout, "while_%d:\n", label_num);
    returnErr(compileE(fout, node->lft));
    fprintf(fout, "PUSH 0\nJE while_end_%d:\n", label_num);
    returnErr(compileBody(fout, node->rgt));
    fprintf(fout, "JMP while_%d:\nwhile_end_%d:\n", label_num, label_num);

    return ERR_OK;
}

static ErrEnum compileE(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_NUM)
    {
        fprintf(fout, "PUSH %d\n", node->val.num);
        return ERR_OK;
    }
    if (node->type == TYPE_VAR)
    {
        fprintf(fout, "PUSH [%s+%d]\n", frame_adr_reg, node->val.var_id);
        return ERR_OK;
    }
    if (node->type == TYPE_FUNC)
    {
        if (node->lft != NULL) returnErr(compileCommaSeparated(fout, node->lft, compileE));
        fputs("CALL ", fout);
        printName(fout, node->val.func_name);
        fputs(":\n", fout);
        return ERR_OK;
    }
    myAssert(node->type == TYPE_OP);

    returnErr(compileE(fout, node->lft));
    returnErr(compileE(fout, node->rgt));
    
    #define OP_CODEGEN(name, n_operands, value, priority, text) \
    if (priority >= 2 && node->val.op_code == OP_ ## name)      \
    {                                                           \
        fputs(#name "\n", fout);                                \
        return ERR_OK;                                          \
    }
    #include <operations.h>
    #undef OP_CODEGEN

    int label_num = label_cnt++;

    #define OP_CODEGEN(name, n_operands, value, priority, text) \
    case OP_ ## name:                                           \
        myAssert(priority == 1);                                \
        fprintf(fout, "J" #name);                               \
        break;
    switch (node->val.op_code)
    {
        #include <operations.h>
        default: myAssert("Invalid op_code" && 0);
    }
    #undef OP_CODEGEN

    fprintf(fout, " cmp_success_%d:\nPUSH 0\nJMP cmp_end_%d:\ncmp_success_%d:\nPUSH 1\ncmp_end_%d:\n", 
    label_num, label_num, label_num, label_num);
    return ERR_OK;
}