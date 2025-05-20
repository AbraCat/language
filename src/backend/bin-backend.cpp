#include <bin-backend.h>
#include <instructions.h>
#include <label.h>
#include <my-error.h>
#include <standardlib.h>
#include <str.h>
#include <tokenizer.h>
#include <backend.h>

#include <stdlib.h>

static void writeByte(unsigned char n);
static void writeBytes(unsigned n);
static void reserveAddress();
static void writeInt(unsigned n);

static void fixCodeSize(BinBackend* b);
static ErrEnum addNumberedLabel(const char* name, int label_num, int fixup);

static ErrEnum binBackendCtor(BinBackend** b, FILE* fout, Node* tree);
static void binBackendDtor(BinBackend* b);

static ErrEnum binCompileFuncDecl(FILE* fout, Node* node);
static ErrEnum binCompileBody(FILE* fout, Node* node);
static ErrEnum binCompileS(FILE* fout, Node* node);

static ErrEnum binCompileIf(FILE* fout, Node* node);
static ErrEnum binCompileWhile(FILE* fout, Node* node);
static ErrEnum binCompileE(FILE* fout, Node* node);
static ErrEnum binCompilePushE(FILE* fout, Node* node);

static BinBackend* b = NULL;
static const unsigned byte_size = 0x100;
static const int e_entry_adr = 0x18, p_filesz_adr = 0x98, p_memsz_adr = 0xa0, code_size_len = 2;
static const int cmp_op_priority = 1, buflen = 0x3000, name_buf_len = 100, code_offset = 0x1000;

#define DEFINE_LABEL(name) returnErr(addNumberedLabel(name, label_num, 0));
#define FIXUP_LABEL(name)\
{\
    returnErr(addNumberedLabel(name, label_num, 1));\
    reserveAddress();\
}
#define FIXUP_FUNCTION_LABEL(name)\
{\
    returnErr(addLabel(b->ft, b->pos, name, 1));\
    reserveAddress();\
}

static ErrEnum binBackendCtor(BinBackend** backend, FILE* fout, Node* tree)
{
    b = *backend = (BinBackend*)calloc(1, sizeof(BinBackend));
    b->buf = (char*)calloc(buflen, 1);
    b->name_buf = (char*)calloc(name_buf_len, 1);

    returnErr(getBinStdLib(fout, b->buf));
    b->pos = *(short*)(b->buf + e_entry_adr);
    b->label_cnt = 0;

    returnErr(labelArrayCtor(&b->la));
    returnErr(labelArrayCtor(&b->ft));
    returnErr(fillFuncArr(&b->name_arr, tree));

    for (int i = 0; i < 3; ++i)
        addLabel(b->la, b->name_arr->names[i].adr, b->name_arr->names[i].name_str, 1);

    return ERR_OK;
}

static void binBackendDtor(BinBackend* b)
{
    free(b->buf);
    free(b->name_buf);
}

ErrEnum runBinBackend(Node* tree, FILE* fout)
{
    myAssert(tree != NULL && fout != NULL);
    returnErr(binBackendCtor(&b, fout, tree));

    writeBytes(CALL); // call main
    FIXUP_FUNCTION_LABEL("main")
    writeBytes(PUSH_RAX);
    writeBytes(CALL); // call exit
    FIXUP_FUNCTION_LABEL("exit")

    returnErr(compileCommaSeparated(fout, tree, binCompileFuncDecl));
    fixCodeSize(b);
    returnErr(fixup(b->buf, b->ft, b->la, 1));
    fwrite(b->buf, 1, b->pos, fout);

    free(b->buf);
    free(b->name_buf);
    labelArrayDtor(b->la);
    labelArrayDtor(b->ft);
    return ERR_OK;
}

static void writeByte(unsigned char n) { b->buf[b->pos++] = n; }
static void reserveAddress() { writeInt(0); }

static void writeBytes(unsigned n)
{
    while (n != 0)
    {
        b->buf[b->pos++] = n % byte_size;
        n /= byte_size;
    }
}

static void writeInt(unsigned n)
{
    for (int i = 0; i < sizeof(int); ++i)
    {
        b->buf[b->pos++] = n % byte_size;
        n /= byte_size;
    }
}

static void fixCodeSize(BinBackend* b)
{
    *(short*)(b->buf + p_filesz_adr) = b->pos - code_offset;
    *(short*)(b->buf + p_memsz_adr) = b->pos - code_offset;
}

static ErrEnum addNumberedLabel(const char* name, int label_num, int fixup)
{
    sprintf(b->name_buf, "%s%d", name, label_num);
    return addLabel(fixup ? b->ft : b->la, b->pos, b->name_buf, 1);
}

static ErrEnum binCompileFuncDecl(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->type == TYPE_OP && node->val.op_code == OP_FUNC);
    myAssert(node->lft != NULL && node->lft->type == TYPE_FUNC && node->lft->val.func_name != NULL);

    Name* name_struct = findName(b->name_arr, node->lft->val.func_name);
    myAssert(name_struct != NULL && name_struct->type == NAME_FUNC);
    name_struct->adr = b->pos;
    addLabel(b->la, b->pos, name_struct->name_str, 1);
    
    n_vars = 0;
    myAssert(node->rgt != NULL && node->rgt->type == TYPE_OP && node->rgt->val.op_code == OP_OPEN_BRACKET);
    if (node->rgt->lft != NULL) returnErr(compileCommaSeparated(fout, node->rgt->lft, checkFuncParam));
    b->n_args = n_vars;

    writeBytes(PUSH_RBP);
    writeBytes(MOV_RBP_RSP);
    returnErr(binCompileBody(fout, node->rgt->rgt));

    if (n_vars != b->n_args)
    {
        writeBytes(ADD_RSP_NUM);
        writeByte(8 * (n_vars - b->n_args));
    }
    writeBytes(POP_RBP);
    writeBytes(XOR_RAX_RAX);
    writeBytes(RET);

    return ERR_OK;
}

static ErrEnum binCompileBody(FILE* fout, Node* node)
{
    myAssert(fout != NULL);
    if (node != NULL) returnErr(compileCommaSeparated(fout, node, binCompileS));
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

            writeBytes(PUSH_NUM);
            writeByte(node->lft->val.var_id);

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
            
            if (n_vars != b->n_args)
            {
                writeBytes(ADD_RSP_NUM);
                writeByte(8 * (n_vars - b->n_args));
            }
            writeBytes(POP_RBP);
            writeBytes(RET);
            return ERR_OK;
        case OP_ASSIGN:
        {
            myAssert(node->lft != NULL && node->lft->type == TYPE_VAR && node->rgt != NULL);
            returnErr(binCompileE(fout, node->rgt));

            int var_id = node->lft->val.var_id;
            writeBytes(MOV_RBP_PLUS_NUM_RAX);
            if (var_id < b->n_args) writeByte(8 * (b->n_args + 1 - var_id));
            else writeByte(0x100 - 8 * (var_id - b->n_args + 1));

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

    int label_num = b->label_cnt++;
    returnErr(binCompileE(fout, node->lft));

    writeBytes(TEST_RAX_RAX);
    writeBytes(JE);
    FIXUP_LABEL("else")

    if (node->rgt->type != TYPE_OP || node->rgt->val.op_code != OP_OPEN_BRACKET)
    {
        returnErr(binCompileBody(fout, node->rgt));
        DEFINE_LABEL("else")
        return ERR_OK;
    }

    returnErr(binCompileBody(fout, node->rgt->lft));
    writeBytes(JMP);
    FIXUP_LABEL("ifEnd")

    DEFINE_LABEL("else")
    returnErr(binCompileBody(fout, node->rgt->rgt));

    DEFINE_LABEL("ifEnd")
    return ERR_OK;
}

static ErrEnum binCompileWhile(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL && node->lft != NULL);

    int label_num = b->label_cnt++;
    DEFINE_LABEL("while")
    returnErr(binCompileE(fout, node->lft));

    writeBytes(TEST_RAX_RAX);
    writeBytes(JE);
    FIXUP_LABEL("whileEnd")

    returnErr(binCompileBody(fout, node->rgt));
    writeBytes(JMP);
    FIXUP_LABEL("while")

    DEFINE_LABEL("whileEnd")
    return ERR_OK;
}

static ErrEnum binCompileE(FILE* fout, Node* node)
{
    myAssert(fout != NULL && node != NULL);

    if (node->type == TYPE_NUM)
    {
        // mov rax, node->val.num
        if (node->val.num >= 0) writeBytes(MOV_RAX_NONNEGATIVE);
        else writeBytes(MOV_RAX_NEGATIVE);
        writeInt(node->val.num);
        return ERR_OK;
    }
    if (node->type == TYPE_VAR)
    {
        int var_id = node->val.var_id;
        writeBytes(MOV_RAX_RBP_PLUS_NUM);
        if (var_id < b->n_args) writeByte(8 * (b->n_args + 1 - var_id));
        else writeByte(0x100 - 8 * (var_id - b->n_args + 1));

        return ERR_OK;
    }
    if (node->type == TYPE_FUNC)
    {
        if (node->lft != NULL) returnErr(compileCommaSeparated(fout, node->lft, 
            binCompilePushE));

        writeBytes(CALL);
        FIXUP_FUNCTION_LABEL(node->val.func_name)

        Name* name_struct = findName(b->name_arr, node->val.func_name);
        myAssert(name_struct != NULL && name_struct->type == NAME_FUNC);
        
        writeBytes(ADD_RSP_NUM);
        writeByte(8 * name_struct->n_args);
        return ERR_OK;
    }
    myAssert(node->type == TYPE_OP);

    returnErr(binCompileE(fout, node->rgt));
    writeBytes(PUSH_RAX);
    returnErr(binCompileE(fout, node->lft));
    writeBytes(POP_RBX);

    // expr1 in rax, expr2 in rbx
    OpInfo *op_info = NULL;
    returnErr(getOpByCode(node->val.op_code, &op_info));
    if (op_info->priority > cmp_op_priority)
    {
        if (node->val.op_code == OP_DIV)
        {
            writeBytes(XOR_RDX_RDX);
            writeBytes(CMP_RAX_RDX);
            writeBytes(MOV_RCX_NUM);
            writeInt(-1);
            writeBytes(CMOVL_RAX_RDX);
            writeBytes(IDIV_RBX);
            return ERR_OK;
        }
        // op rax, rbx
        int instr_code = 0;
        switch (op_info->op_code)
        {
            case OP_ADD:
                instr_code = ADD_RAX_RBX;
                break;
            case OP_SUB:
                instr_code = SUB_RAX_RBX;
                break;
            case OP_MUL:
                instr_code = IMUL_RAX_RBX;
                break;
            case OP_XOR:
                instr_code = XOR_RAX_RBX;
                break;
            default:
                myAssert(0);
        }
        writeBytes(instr_code);
        return ERR_OK;
    }
    myAssert(op_info->priority == cmp_op_priority);
    
    int label_num = b->label_cnt++, instr_code = 0;
    writeBytes(CMP_RAX_RAX);
    switch (op_info->op_code)
    {
        case OP_B:
            instr_code = JL;
            break;
        case OP_BE:
            instr_code = JLE;
            break;
        case OP_A:
            instr_code = JG;
            break;
        case OP_AE:
            instr_code = JGE;
            break;
        case OP_E:
            instr_code = JE;
            break;
        case OP_NE:
            instr_code = JNE;
            break;
        default:
            myAssert(0);  
    }
    writeBytes(instr_code);
    FIXUP_LABEL("cmpSuccess")

    writeBytes(XOR_RAX_RAX);
    writeBytes(JMP);
    FIXUP_LABEL("cmpEnd")

    DEFINE_LABEL("cmpSuccess")
    writeBytes(MOV_RAX_NUM);
    writeInt(1);

    DEFINE_LABEL("cmpEnd")
    return ERR_OK;
}

static ErrEnum binCompilePushE(FILE* fout, Node* node)
{
    returnErr(binCompileE(fout, node));
    writeBytes(PUSH_RAX);
    return ERR_OK;
}
