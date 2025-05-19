#include <backend.h>
#include <str.h>
#include <standartlib.h>
#include <tokenizer.h>
#include <label.h>

#include <stdlib.h>

static ErrEnum binCompileCommaSeparated(FILE* fout, Node* node, ErrEnum (*binCompile)(FILE*, Node*));
static ErrEnum binCompileFuncDecl(FILE* fout, Node* node);
static ErrEnum asmCheckFuncParam(FILE* fout, Node* node);
static ErrEnum binCompileBody(FILE* fout, Node* node);
static ErrEnum binCompileS(FILE* fout, Node* node);

static ErrEnum binCompileIf(FILE* fout, Node* node);
static ErrEnum binCompileWhile(FILE* fout, Node* node);
static ErrEnum binCompileE(FILE* fout, Node* node);
static ErrEnum binCompilePushE(FILE* fout, Node* node);

static const int cmp_op_priority = 1, buflen = 0x3000, name_buf_len = 100, code_start = 0x1000;
static int label_cnt = 0, n_vars = 0, n_args = 0;
static NameArr* namearr = NULL;

static int pos = 0;
static char *buf = NULL, *name_buf = NULL;

LabelArray *la = NULL, *ft = NULL;

#define BYTE(byte) buf[pos++] = 0x ## byte;
#define PUT(expr)  buf[pos++] = (expr);
#define ADR(integer) {*(int*)(buf + pos) = integer; pos += 4; }
#define ADD_LABEL(table, name)\
{\
    sprintf(name_buf, name "%d", label_num);\
    returnErr(addLabel(table, pos, name_buf, 1));\
}
#define ADD_LABEL_NO_NUM(table, name) returnErr(addLabel(table, pos, name, 1));

ErrEnum runBinBackend(Node* tree, NameArr* name_arr, FILE* fout)
{
    myAssert(tree != NULL && fout != NULL && name_arr != NULL);
    namearr = name_arr;

    returnErr(labelArrayCtor(&la));
    returnErr(labelArrayCtor(&ft));

    for (int i = 0; i < 3; ++i)
        addLabel(la, namearr->names[i].adr, namearr->names[i].name_str, 1);

    buf = (char*)calloc(buflen, 1);
    name_buf = (char*)calloc(name_buf_len, 1);
    returnErr(getBinStdLib(fout, buf));

    // 0x98 (2), 0xa0 (2) - size of code
    // 0x18 (8) - entrypoint
    pos = ((short*)buf)[12];
    // printf("pos: %x\n", pos);

    BYTE(e8) // call main
    addLabel(ft, pos, "main", 1);
    ADR(0)
    BYTE(50) // push rax
    BYTE(e8) // call exit
    addLabel(ft, pos, "exit", 1);
    ADR(0)

    returnErr(binCompileCommaSeparated(fout, tree, binCompileFuncDecl));

    ((short*)buf)[0x98 / 2] = ((short*)buf)[0xa0 / 2] = pos - code_start;

    // labelArrDump(la);
    // labelArrDump(ft);
    returnErr(fixup(buf, ft, la, 1));
    fwrite(buf, 1, pos, fout);

    free(buf);
    free(name_buf);
    labelArrayDtor(la);
    labelArrayDtor(ft);
    return ERR_OK;
}

static ErrEnum binCompileCommaSeparated(FILE* fout, Node* node, ErrEnum (*binCompile)(FILE*, Node*))
{
    myAssert(fout != NULL && node != NULL);

    Node* cur_node = node;
    while (cur_node != NULL && cur_node->type == TYPE_OP && cur_node->val.op_code == OP_COMMA) cur_node = cur_node->lft;
    returnErr(binCompile(fout, cur_node));
    if (cur_node == node) return ERR_OK;

    while (1)
    {
        cur_node = cur_node->parent;
        returnErr(binCompile(fout, cur_node->rgt));
        if (cur_node == node) break;
    }
    return ERR_OK;
}

static ErrEnum binCompileFuncDecl(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->type == TYPE_OP && node->val.op_code == OP_FUNC);

    myAssert(node->lft != NULL && node->lft->type == TYPE_FUNC && node->lft->val.func_name != NULL);
    // printName(fout, node->lft->val.func_name);
    // fputs(":\n", fout);

    // n_vars = 0;
    // myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    // if (node->rgt->lft != NULL) returnErr(binCompileCommaSeparated(fout, node->rgt->lft, asmCheckFuncParam));
    // n_args = n_vars;
    // fprintf(fout, "push rbp\nmov rbp, rsp\n");
    // returnErr(binCompileBody(fout, node->rgt->rgt));

    // fprintf(fout, "add rsp, %d ; deallocating local vars\n", 8 * (n_vars - n_args));
    // fprintf(fout, "pop rbp\nxor rax, rax\nret\n");
    // return ERR_OK;

    Name* name_struct = findName(namearr, node->lft->val.func_name);
    myAssert(name_struct != NULL && name_struct->type == NAME_FUNC);
    name_struct->adr = pos;
    // printf("func pos: %x\n", pos);
    addLabel(la, pos, name_struct->name_str, 1);
    
    n_vars = 0;
    myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    if (node->rgt->lft != NULL) returnErr(binCompileCommaSeparated(fout, node->rgt->lft, asmCheckFuncParam));
    n_args = n_vars;
    // fprintf(fout, "push rbp\nmov rbp, rsp\n");
    BYTE(55) // push rbp
    BYTE(48) // mov rbp, rsp
    BYTE(89)
    BYTE(e5)
    returnErr(binCompileBody(fout, node->rgt->rgt));

    // fprintf(fout, "add rsp, %d ; deallocating local vars\n", 8 * (n_vars - n_args));
    // fprintf(fout, "pop rbp\nxor rax, rax\nret\n");
    if (n_vars != n_args)
    {
        BYTE(48) // add rsp, 8 * (n_vars - n_args)
        BYTE(83)
        BYTE(c4)
        PUT(8 * (n_vars - n_args))
    }
    BYTE(5d) // pop rbp
    BYTE(48) // xor rax, rax
    BYTE(31)
    BYTE(c0)
    BYTE(c3) // ret
    return ERR_OK;
}

static ErrEnum asmCheckFuncParam(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);
    myAssert(node->type == TYPE_VAR);
    myAssert(node->val.var_id == n_vars);
    ++n_vars;
    return ERR_OK;
}

static ErrEnum binCompileBody(FILE* fout, Node* node)
{
    myAssert(fout != NULL);
    if (node != NULL) returnErr(binCompileCommaSeparated(fout, node, binCompileS));
    return ERR_OK;
}

static ErrEnum binCompileS(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_OP) switch (node->val.op_code)
    {
        case OP_VAR:
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR);
            myAssert(node->lft->val.var_id == n_vars);
            // fprintf(fout, "push 0 ; var %d declared\n", node->lft->val.var_id);
            BYTE(6a) // push node->lft->val.var_id
            PUT(node->lft->val.var_id)
            ++n_vars;
            return ERR_OK;
        case OP_IF:
            returnErr(binCompileIf(fout, node));
            return ERR_OK;
        case OP_WHILE:
            returnErr(binCompileWhile(fout, node));
            return ERR_OK;
        case OP_RET:
            myAssert(node->lft != NULL);
            returnErr(binCompileE(fout, node->lft));
            // fprintf(fout, "add rsp, %d ; deallocating local vars\n"
            //     "pop rbp\n"
            //     "ret\n", 
            //     8 * (n_vars - n_args));
            if (n_vars != n_args)
            {
                BYTE(48) // add rsp, 8 * (n_vars - n_args)
                BYTE(83)
                BYTE(c4)
                PUT(8 * (n_vars - n_args))
            }
            BYTE(5d) // pop rbp
            BYTE(c3) // ret
            return ERR_OK;
        case OP_ASSIGN:
        {
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR && node->rgt != NULL);
            returnErr(binCompileE(fout, node->rgt));
            int var_id = node->lft->val.var_id;

            BYTE(48) // mov [rbp + ??], rax
            BYTE(89)
            BYTE(45)
            if (var_id < n_args)
                // fprintf(fout, "mov [rbp + %d], rax\n", 8 * (n_args + 1 - var_id));
                PUT(8 * (n_args + 1 - var_id))
            else
                // fprintf(fout, "mov [rbp - %d], rax\n", 8 * (var_id - n_args + 1));
                PUT(0x100 - 8 * (var_id - n_args + 1))
            return ERR_OK;
        }
        default:
            break;
    }
    returnErr(binCompileE(fout, node));
    return ERR_OK;
}

static ErrEnum binCompileIf(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;
    // printf("if: %x\n", pos);

    returnErr(binCompileE(fout, node->lft));
    // printf("if: %x\n", pos);
    // fprintf(fout, "test rax, rax\nje else_%d\n", label_num);
    BYTE(48) // test rax, rax
    BYTE(85)
    BYTE(c0)
    BYTE(0f) // je else_%d
    BYTE(84)
    ADD_LABEL(ft, "else")
    ADR(0)

    if (node->rgt->type != TYPE_OP || node->rgt->val.op_code != OP_OPEN_BRACKET)
    {
        returnErr(binCompileBody(fout, node->rgt));
        // fprintf(fout, "else_%d:\n", label_num);
        ADD_LABEL(la, "else")
        return ERR_OK;
    }

    returnErr(binCompileBody(fout, node->rgt->lft));
    // fprintf(fout, "jmp if_end_%d\nelse_%d:\n", label_num, label_num);
    BYTE(e9) // jmp if_end_%d
    ADD_LABEL(ft, "ifEnd")
    ADR(0)
    ADD_LABEL(la, "else")
    returnErr(binCompileBody(fout, node->rgt->rgt));
    // fprintf(fout, "if_end_%d:\n", label_num);
    ADD_LABEL(la, "ifEnd")
    return ERR_OK;
}

static ErrEnum binCompileWhile(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);
    int label_num = label_cnt++;

    // fprintf(fout, "while_%d:\n", label_num);
    ADD_LABEL(la, "while")
    returnErr(binCompileE(fout, node->lft));
    // fprintf(fout, "test rax, rax\nje while_end_%d\n", label_num);
    BYTE(48) // test rax, rax
    BYTE(85)
    BYTE(c0)
    BYTE(0f) // je while_end_%d
    BYTE(84)
    ADD_LABEL(ft, "whileEnd")
    ADR(0)
    returnErr(binCompileBody(fout, node->rgt));
    // fprintf(fout, "jmp while_%d\nwhile_end_%d:\n", label_num, label_num);
    BYTE(e9) // jmp while_%d
    ADD_LABEL(ft, "while")
    ADR(0)
    ADD_LABEL(la, "whileEnd")
    return ERR_OK;
}

static ErrEnum binCompileE(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_NUM)
    {
        // fprintf(fout, "mov rax, %d\n", node->val.num);
        // mov rax, %d
        if (node->val.num >= 0)
        {
            BYTE(b8)
        }
        else
        {
            BYTE(48)
            BYTE(c7)
            BYTE(c0)
        }
        ADR(node->val.num)
        return ERR_OK;
    }
    if (node->type == TYPE_VAR)
    {
        int var_id = node->val.var_id;
        BYTE(48) // mov rax, [rbp + %d]
        BYTE(8b)
        BYTE(45)
        if (var_id < n_args)
            // fprintf(fout, "mov rax, [rbp + %d]\n", 8 * (n_args + 1 - var_id));
            PUT(8 * (n_args + 1 - var_id))
        else
            // fprintf(fout, "mov rax, [rbp - %d]\n", 8 * (var_id - n_args + 1));
            PUT(0x100 - 8 * (var_id - n_args + 1))
        return ERR_OK;
    }
    if (node->type == TYPE_FUNC)
    {
        if (node->lft != NULL) returnErr(binCompileCommaSeparated(fout, node->lft, binCompilePushE));
        // fputs("call ", fout);
        // printName(fout, node->val.func_name);
        BYTE(e8) // call %s
        ADD_LABEL_NO_NUM(ft, node->val.func_name)
        ADR(0)

        Name* name_struct = findName(namearr, node->val.func_name);
        myAssert(name_struct != NULL && name_struct->type == NAME_FUNC);
        // fprintf(fout, "\nadd rsp, %d ; deallocating function args\n", 8 * name_struct->n_args);
        BYTE(48) // add rsp, 8 * name_struct->n_args
        BYTE(83)
        BYTE(c4)
        PUT(8 * name_struct->n_args)
        return ERR_OK;
    }
    myAssert(node->type == TYPE_OP);

    returnErr(binCompileE(fout, node->rgt));
    // fprintf(fout, "push rax\n");
    BYTE(50) // push rax
    returnErr(binCompileE(fout, node->lft));
    // fprintf(fout, "pop rbx\n");
    BYTE(5b)

    // expr1 in rax, expr2 in rbx
    OpInfo *op_info = NULL;
    returnErr(getOpByCode(node->val.op_code, &op_info));
    if (op_info->priority > cmp_op_priority)
    {
        if (node->val.op_code == OP_DIV)
        {
            // fprintf(fout, "xor rdx, rdx\n"
            //     "cmp rax, rdx\n"
            //     "mov rcx, -1\n"
            //     "cmovl rdx, rcx\n"
            //     "idiv rbx\n");
            BYTE(48) // xor rdx, rdx
            BYTE(31)
            BYTE(d2)
            BYTE(48) // cmp rax, rdx
            BYTE(39)
            BYTE(d0)
            BYTE(48) // mov rcx, -1
            BYTE(c7)
            BYTE(c1)
            ADR(-1)
            BYTE(48) // cmovl rax, rdx
            BYTE(0f)
            BYTE(4c)
            BYTE(d1)
            BYTE(48) // idiv rbx
            BYTE(f7)
            BYTE(fb)
        }
        else
        {
            // fprintf(fout, "%s rax, rbx\n", op_info->asm_instr);
            // %s rax, rbx
            switch (op_info->op_code)
            {
                case OP_ADD:
                    BYTE(48)
                    BYTE(01)
                    BYTE(d8)
                    break;
                case OP_SUB:
                    BYTE(48)
                    BYTE(29)
                    BYTE(d8)
                    break;
                case OP_MUL:
                    BYTE(48)
                    BYTE(0f)
                    BYTE(af)
                    BYTE(c3)
                    break;
                case OP_XOR:
                    BYTE(48)
                    BYTE(31)
                    BYTE(d8)
                    break;
                default:
                    myAssert(0);
            }
        }
        return ERR_OK;
    }
    myAssert(op_info->priority == cmp_op_priority);
    // int label_num = label_cnt++;
    // fprintf(fout, "cmp rax, rbx\n"
    //     "%s cmp_success_%d\n"
    //     "xor rax, rax\n"
    //     "jmp cmp_end_%d\n"
    //     "cmp_success_%d:\n"
    //     "mov rax, 1\n"
    //     "cmp_end_%d:\n", 
    // op_info->asm_instr, label_num, label_num, label_num, label_num);
    int label_num = label_cnt++;
    BYTE(48) // cmp rax, rbx
    BYTE(39)
    BYTE(d8)
    
    // jxx cmp_success_%d
    switch (op_info->op_code)
    {
        case OP_B:
            BYTE(0f)
            BYTE(8c)
            break;
        case OP_BE:
            BYTE(0f)
            BYTE(8e)
            break;
        case OP_A:
            BYTE(0f)
            BYTE(8f)
            break;
        case OP_AE:
            BYTE(0f)
            BYTE(8d)
            break;
        case OP_E:
            BYTE(0f)
            BYTE(84)
            break;
        case OP_NE:
            BYTE(0f)
            BYTE(85)
            break;
        default:
            myAssert(0);  
    }
    ADD_LABEL(ft, "cmpSuccess")
    ADR(0)

    BYTE(48) // xor rax, rax
    BYTE(31)
    BYTE(c0)

    BYTE(e9) // jmp cmp_end_%d
    ADD_LABEL(ft, "cmpEnd")
    ADR(0)

    ADD_LABEL(la, "cmpSuccess")
    
    BYTE(b8) // mov rax, 1
    ADR(1)

    ADD_LABEL(la, "cmpEnd")
    return ERR_OK;
}

static ErrEnum binCompilePushE(FILE* fout, Node* node)
{
    returnErr(binCompileE(fout, node));
    // fprintf(fout, "push rax\n");
    BYTE(50) // push rax
    return ERR_OK;
}
