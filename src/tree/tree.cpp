#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <utils.h>
#include <logs.h>

int dump_cnt = 0;
static const int buffer_size = 300;

#define OP_CODEGEN(name, n_operands, value, priority, text) {OP_ ## name, text, sizeof text - 1, priority},
static OpInfo op_info_arr[] = {
    #include <operations.h>
};
#undef OP_CODEGEN

extern const int n_ops = sizeof op_info_arr / sizeof(OpInfo);

ErrEnum nodeCtor(Node** node, NodeType type, NodeVal val, Node* parent, Node* lft, Node* rgt)
{
    myAssert(node != NULL);
    if (*node == NULL) 
    {
        *node = (Node*)calloc(1, sizeof(Node));
        if (*node == NULL) return ERR_MEM;
    }

    (*node)->type = type;
    (*node)->val = val;
    (*node)->parent = parent;
    (*node)->lft = lft;
    (*node)->rgt = rgt;

    if (lft != NULL)
    {
        if (lft->parent != NULL && lft->parent != *node) return ERR_PARENT_DISCARDED;
        lft->parent = *node;
    }
    if (rgt != NULL)
    {
        if (rgt->parent != NULL && rgt->parent != *node) return ERR_PARENT_DISCARDED;
        rgt->parent = *node;
    }

    (*node)->visited = 0;

    return ERR_OK;
}

void nodeDtor(Node* node)
{
    if (node == NULL) return;
    nodeDtor(node->lft);
    nodeDtor(node->rgt);
    free(node);
}

void nNodes(Node* node, int* ans)
{
    myAssert(ans != NULL);

    if (node == NULL)
    {
        *ans = 0;
        return;
    }
    int ans_lft = 0, ans_rgt = 0;
    nNodes(node->lft, &ans_lft);
    nNodes(node->rgt, &ans_rgt);
    *ans = ans_lft + ans_rgt + 1;
}

ErrEnum nodeVerify(Node* node)
{
    if (node == NULL) return ERR_OK;
    
    if (node->visited == 1) return ERR_TREE_CYCLE;
    node->visited = 1;

    if (node->lft != NULL && node->lft->parent != node) return ERR_INVAL_CONNECT;
    if (node->rgt != NULL && node->rgt->parent != node) return ERR_INVAL_CONNECT;

    returnErr(nodeVerify(node->lft));
    returnErr(nodeVerify(node->lft));

    node->visited = 0;
    return ERR_OK;
}

ErrEnum getOpByCode(OpEnum op_code, OpInfo** ans)
{
    myAssert(ans != NULL);

    int ind = (int)op_code;
    if (ind < 0 || ind >= n_ops)
    {
         *ans = NULL;
        return ERR_INVAL_OP_CODE;
    }
    *ans = op_info_arr + ind;
    return ERR_OK;
}

ErrEnum getOpByStr(const char* op_str, OpInfo** ans)
{
    myAssert(op_str != NULL && ans != NULL);

    for (int ind = 0; ind < n_ops; ++ind)
    {
        if (strcmpToBracket(op_str, op_info_arr[ind].op_str) == 0)
        {
            *ans = op_info_arr + ind;
            return ERR_OK;
        }
    }
    *ans = NULL;
    return ERR_INVAL_OP_STR;
}

void nodeWrite(FILE* fout, Node* node)
{
    if (node == NULL) return;
    fputc('(', fout);
    nodeWrite(fout, node->lft);
    
    switch (node->type)
    {
        case TYPE_NUM:
            printDouble(fout, node->val.num);
            break;
        case TYPE_VAR:
            fputc('x', fout);
            break;
        case TYPE_OP:
        {
            OpInfo* op_descr = NULL;
            fputs(getOpByCode(node->val.op_code, &op_descr) == ERR_OK ? op_descr->op_str : "BAD_OP", fout);
            break;
        }
        default:
            fputs("BAD_TYPE", fout);
            break;
    }

    nodeWrite(fout, node->rgt);
    fputc(')', fout);
}

ErrEnum treeWrite(Node* node, const char* expr_brackets_path)
{
    FILE* fout = fopen(expr_brackets_path, "w");
    if (fout == NULL) return ERR_OPEN_FILE;

    nodeWrite(fout, node);

    fclose(fout);
    return ERR_OK;
}

ErrEnum nodeRead(char* buf, int* buf_pos, Node* node, int buf_size)
{
    #define cur_buf (buf + *buf_pos)

    #define INCR_BUF_POS(incr)    \
        *buf_pos += incr;         \
        if (*buf_pos >= buf_size) \
            return ERR_BUF_BOUND;

    int pos_incr = 0;

    if (cur_buf[0] != '(') return ERR_TREE_FMT;
    INCR_BUF_POS(1);
    if (cur_buf[0] == '(')
    {
        returnErr(nodeCtor(&node->lft, TYPE_NUM, {.num = 0}, node, NULL, NULL));
        returnErr(nodeRead(buf, buf_pos, node->lft, buf_size));
    }
    if (sscanf(cur_buf, "%lf%n", &node->val.num, &pos_incr) == 1)
    {
        INCR_BUF_POS(pos_incr);
        node->type = TYPE_NUM;
    }
    else if (cur_buf[0] == 'x')
    {
        INCR_BUF_POS(1);
        node->type = TYPE_VAR;
        node->val.var_id = 0;
    }
    else
    {
        OpInfo* op_info = 0;
        returnErr(getOpByStr(cur_buf, &op_info));
        node->type = TYPE_OP;
        node->val.op_code = op_info->op_code;
        INCR_BUF_POS(op_info->op_str_len);
    }

    if (cur_buf[0] == '(')
    {
        returnErr(nodeCtor(&node->rgt, TYPE_NUM, {.num = 0}, node, NULL, NULL));
        returnErr(nodeRead(buf, buf_pos, node->rgt, buf_size));
    }
    if (*cur_buf != ')') return ERR_TREE_FMT;
    INCR_BUF_POS(1);

    return ERR_OK;
}

ErrEnum treeRead(Node** tree, const char* expr_brackets_path)
{
    myAssert(tree != NULL && *tree == NULL);

    int buf_size = 0, buf_pos = 0;
    char* buf = NULL;
    returnErr(readFile(expr_brackets_path, (void**)(&buf), &buf_size));

    returnErr(nodeCtor(tree, TYPE_NUM, {.num = 0}, NULL, NULL, NULL));
    returnErr(nodeRead(buf, &buf_pos, *tree, buf_size));

    free(buf);
    return ERR_OK;
}

ErrEnum printNodeDot(FILE* fout, Node* node)
{
    // node123 [shape = Mrecord, label = "{type | val | parent | { lft | rgt }}"]
    
    fprintf(fout, "node%p [shape = Mrecord , label = \"{node %p|", node, node);
    switch (node->type)
    {
        case TYPE_OP:
        {
            OpInfo* op_info = NULL;
            returnErr(getOpByCode(node->val.op_code, &op_info));
            fprintf(fout, "OP | %s", op_info->op_str);
            break;
        }
        case TYPE_VAR:
            fprintf(fout, "VAR | %d", node->val.var_id);
            break;
        case TYPE_FUNC:
            fprintf(fout, "FUNC | %s", node->val.func_name);
            break;
        case TYPE_NUM:
            fprintf(fout, "NUM | %d", node->val.num);
            break;
        default:
            fprintf(fout, "BAD_TYPE (%d)", (int)(node->type));
            break;
    }
    fprintf(fout, " | <parent>parent %p | {<lft>lft %p | <rgt>rgt %p}}\"]\n", node->parent, node->lft, node->rgt);

    if (node->lft != NULL)
    {
        printNodeDot(fout, node->lft);
        fprintf(fout, "node%p:<lft> -> node%p[color = red]\n", node, node->lft);
    }
    if (node->rgt != NULL)
    {
        printNodeDot(fout, node->rgt);
        fprintf(fout, "node%p:<rgt> -> node%p[color = green]\n", node, node->rgt);
    }

    return ERR_OK;
}

ErrEnum treeMakeGraph(Node* tree)
{
    myAssert(tree != NULL);

    char buf[buffer_size] = "";

    sprintf(buf, "%s/dot-src/dot-src.txt", log_path);
    FILE *fout = fopen(buf, "w");
    if (fout == NULL) return ERR_OPEN_FILE;

    fputs("digraph Tree\n{\nrankdir = TB\n", fout);
    if (tree == NULL) fputs("NULL [shape = Mrecord]\n", fout);
    else returnErr(printNodeDot(fout, tree));
    fputs("}\n", fout);

    fclose(fout);

    sprintf(buf, "dot %s/dot-src/dot-src.txt -Tpng -o%s/dot-img/dot-img-%d.png", 
    log_path, log_path, dump_cnt);
    system(buf);

    return ERR_OK;
}

ErrEnum treeDump(Node* tree)
{
    myAssert(tree != NULL);

    char buf[buffer_size] = "";
    openDumpFile();

    // dump text

    sprintf(buf, "../dot-img/dot-img-%d.png", dump_cnt);
    fprintf(fdump, "\n\n<img src=\"%s\" alt=\"graph image\"/>\n\n", buf);

    treeMakeGraph(tree);
    ++dump_cnt;
    return ERR_OK;
}

ErrEnum nodeCopy(Node* src, Node** dest)
{
    myAssert(src != NULL && dest != NULL && *dest == NULL);

    Node *lft_copy = NULL, *rgt_copy = NULL;
    if (src->lft != NULL) 
    {
        returnErr(nodeCopy(src->lft, &lft_copy));
    }
    if (src->rgt != NULL) 
    {
        returnErr(nodeCopy(src->rgt, &rgt_copy));
    }
    returnErr(nodeCtor(dest, src->type, src->val, NULL, lft_copy, rgt_copy));
    if (lft_copy != NULL) lft_copy->parent = *dest;
    if (rgt_copy != NULL) rgt_copy->parent = *dest;

    return ERR_OK;
}

ErrEnum connectLinear(Node* nodes, int n_nodes)
{
    myAssert(nodes != NULL && n_nodes >= 0);
    for (int ind = 0; ind < n_nodes - 1; ++ind)
    {
        nodes[ind].lft = nodes + ind + 1;
    }
    return ERR_OK;
}