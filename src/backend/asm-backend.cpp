#include <asm-backend.h>
#include <str.h>
#include <standardlib.h>
#include <backend.h>

static ErrEnum asmCompileFuncDecl(FILE* fout, Node* node);
static ErrEnum asmCompileBody(FILE* fout, Node* node);
static ErrEnum asmCompileS(FILE* fout, Node* node);

static ErrEnum asmCompileIf(FILE* fout, Node* node);
static ErrEnum asmCompileWhile(FILE* fout, Node* node);
static ErrEnum asmCompileE(FILE* fout, Node* node);
static ErrEnum asmCompilePushE(FILE* fout, Node* node);

static const int st_size = 400, cmp_op_priority = 1;
static int label_cnt = 0, n_args = 0;
static NameArr* namearr = NULL;

ErrEnum runAsmBackend(Node* tree, FILE* fout)
{
    myAssert(tree != NULL && fout != NULL);
    returnErr(fillFuncArr(&namearr, tree));

    fprintf(fout,   "global _start\n"
                    "section .text\n\n"
                    "_start:\n"
                    "call main\n"
                    "push rax\n"
                    "call exit\n");
    asmPrintStdLib(fout);
    return compileCommaSeparated(fout, tree, asmCompileFuncDecl);
}

static ErrEnum asmCompileFuncDecl(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->type == TYPE_OP && node->val.op_code == OP_FUNC);

    myAssert(node->lft != NULL && node->lft->type == TYPE_FUNC && node->lft->val.func_name != NULL);
    printName(fout, node->lft->val.func_name);
    fputs(":\n", fout);

    n_vars = 0;
    myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    if (node->rgt->lft != NULL) returnErr(compileCommaSeparated(fout, node->rgt->lft, checkFuncParam));
    n_args = n_vars;
    fprintf(fout, "push rbp\nmov rbp, rsp\n");
    returnErr(asmCompileBody(fout, node->rgt->rgt));

    if (n_vars != n_args)
        fprintf(fout, "add rsp, %d ; deallocating local vars\n", 8 * (n_vars - n_args));
    fprintf(fout, "pop rbp\nxor rax, rax\nret\n");
    return ERR_OK;
}

static ErrEnum asmCompileBody(FILE* fout, Node* node)
{
    myAssert(fout != NULL);
    if (node != NULL) returnErr(compileCommaSeparated(fout, node, asmCompileS));
    return ERR_OK;
}

static ErrEnum asmCompileS(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_OP) switch (node->val.op_code)
    {
        case OP_VAR:
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR);
            myAssert(node->lft->val.var_id == n_vars);
            fprintf(fout, "push 0 ; var %d declared\n", node->lft->val.var_id);
            ++n_vars;
            return ERR_OK;
        case OP_IF:
            returnErr(asmCompileIf(fout, node));
            return ERR_OK;
        case OP_WHILE:
            returnErr(asmCompileWhile(fout, node));
            return ERR_OK;
        case OP_RET:
            myAssert(node->lft != NULL);
            returnErr(asmCompileE(fout, node->lft));
            if (n_vars != n_args)
                fprintf(fout, "add rsp, %d ; deallocating local vars\n", 8 * (n_vars - n_args));
            fprintf(fout, "pop rbp\nret\n");
            return ERR_OK;
        case OP_ASSIGN:
        {
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR && node->rgt != NULL);
            returnErr(asmCompileE(fout, node->rgt));
            int var_id = node->lft->val.var_id;
            if (var_id < n_args)
                fprintf(fout, "mov [rbp + %d], rax\n", 8 * (n_args + 1 - var_id));
            else
                fprintf(fout, "mov [rbp - %d], rax\n", 8 * (var_id - n_args + 1));
            return ERR_OK;
        }
        default:
            break;
    }
    returnErr(asmCompileE(fout, node));
    return ERR_OK;
}

static ErrEnum asmCompileIf(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;

    returnErr(asmCompileE(fout, node->lft));
    fprintf(fout, "test rax, rax\nje else_%d\n", label_num);

    if (node->rgt->type != TYPE_OP || node->rgt->val.op_code != OP_OPEN_BRACKET)
    {
        returnErr(asmCompileBody(fout, node->rgt));
        fprintf(fout, "else_%d:\n", label_num);
        return ERR_OK;
    }

    returnErr(asmCompileBody(fout, node->rgt->lft));
    fprintf(fout, "jmp if_end_%d\nelse_%d:\n", label_num, label_num);
    returnErr(asmCompileBody(fout, node->rgt->rgt));
    fprintf(fout, "if_end_%d:\n", label_num);
    return ERR_OK;
}

static ErrEnum asmCompileWhile(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;

    fprintf(fout, "while_%d:\n", label_num);
    returnErr(asmCompileE(fout, node->lft));
    fprintf(fout, "test rax, rax\nje while_end_%d\n", label_num);
    returnErr(asmCompileBody(fout, node->rgt));
    fprintf(fout, "jmp while_%d\nwhile_end_%d:\n", label_num, label_num);

    return ERR_OK;
}

static ErrEnum asmCompileE(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_NUM)
    {
        fprintf(fout, "mov rax, %d\n", node->val.num);
        return ERR_OK;
    }
    if (node->type == TYPE_VAR)
    {
        int var_id = node->val.var_id;
        if (var_id < n_args)
                fprintf(fout, "mov rax, [rbp + %d]\n", 8 * (n_args + 1 - var_id));
            else
                fprintf(fout, "mov rax, [rbp - %d]\n", 8 * (var_id - n_args + 1));
        return ERR_OK;
    }
    if (node->type == TYPE_FUNC)
    {
        if (node->lft != NULL) returnErr(compileCommaSeparated(fout, node->lft, asmCompilePushE));
        fputs("call ", fout);
        printName(fout, node->val.func_name);

        Name* name_struct = findName(namearr, node->val.func_name);
        myAssert(name_struct != NULL && name_struct->type == NAME_FUNC);
        fprintf(fout, "\nadd rsp, %d ; deallocating function args\n", 8 * name_struct->n_args);
        return ERR_OK;
    }
    myAssert(node->type == TYPE_OP);

    returnErr(asmCompileE(fout, node->rgt));
    fprintf(fout, "push rax\n");
    returnErr(asmCompileE(fout, node->lft));
    fprintf(fout, "pop rbx\n");

    // expr1 in rax, expr2 in rbx
    OpInfo *op_info = NULL;
    returnErr(getOpByCode(node->val.op_code, &op_info));
    if (op_info->priority > cmp_op_priority)
    {
        if (node->val.op_code == OP_DIV)
            fprintf(fout, "xor rdx, rdx\n"
                "cmp rax, rdx\n"
                "mov rcx, -1\n"
                "cmovl rdx, rcx\n"
                "idiv rbx\n");
        else
            fprintf(fout, "%s rax, rbx\n", op_info->asm_instr);
        return ERR_OK;
    }
    myAssert(op_info->priority == cmp_op_priority);
    int label_num = label_cnt++;
    fprintf(fout, "cmp rax, rbx\n"
        "%s cmp_success_%d\n"
        "xor rax, rax\n"
        "jmp cmp_end_%d\n"
        "cmp_success_%d:\n"
        "mov rax, 1\n"
        "cmp_end_%d:\n", 
    op_info->asm_instr, label_num, label_num, label_num, label_num);
    return ERR_OK;
}

static ErrEnum asmCompilePushE(FILE* fout, Node* node)
{
    returnErr(asmCompileE(fout, node));
    fprintf(fout, "push rax\n");
    return ERR_OK;
}
