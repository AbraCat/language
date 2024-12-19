#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <utils.h>
#include <logs.h>
#include <str.h>

int dump_cnt = 0;
extern const int name_buf_size = 30;
static const int buffer_size = 300;

#define OP_CODEGEN(name, n_operands, value, priority, text) {OP_ ## name, #name, text, sizeof text - 1, priority},
static OpInfo op_info_arr[] = {
    #include <operations.h>
};
#undef OP_CODEGEN

#define NODE_TYPE_CASE(type) {TYPE_ ## type, #type, sizeof #type - 1},
static NodeTypeInfo type_info_arr[] = {
    NODE_TYPE_CASE(OP)
    NODE_TYPE_CASE(VAR)
    NODE_TYPE_CASE(FUNC)
    NODE_TYPE_CASE(NUM)
};
#undef NODE_TYPE_CASE

extern const int n_ops = sizeof op_info_arr / sizeof(OpInfo), n_types = sizeof type_info_arr / sizeof(NodeTypeInfo);

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

bool treeEqual(Node* tree1, Node* tree2)
{
    if (tree1 == NULL && tree2 == NULL) return 1;
    if (tree1 == NULL || tree2 == NULL) return 0;
    if (tree1->type != tree2->type) return 0;
    if (tree1->type == TYPE_FUNC && nameCmp(tree1->val.func_name, tree2->val.func_name, NULL) != 0) return 0;
    if (tree1->type != TYPE_FUNC && tree1->val.num != tree2->val.num) return 0;
    return treeEqual(tree1->lft, tree2->lft) && treeEqual(tree1->rgt, tree2->rgt);
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
        if (strncmp(op_str, op_info_arr[ind].op_str, op_info_arr[ind].op_str_len) == 0)
        {
            *ans = op_info_arr + ind;
            return ERR_OK;
        }
    }
    *ans = NULL;
    return ERR_INVAL_OP_STR;
}

ErrEnum getTypeByCode(NodeType type, NodeTypeInfo** ans)
{
    myAssert(ans != NULL);
    int ind = (int)type;
    if (ind < 0 || ind >= n_types) return ERR_INVAL_NODE_TYPE;
    *ans = type_info_arr + ind;
    return ERR_OK;
}

ErrEnum getTypeByStr(const char* type_str, NodeTypeInfo** ans)
{
    myAssert(type_str != NULL && ans != NULL);
    for (int ind = 0; ind < n_ops; ++ind)
    {
        if (strncmp(type_str, type_info_arr[ind].type_str, type_info_arr[ind].type_str_len) == 0)
        {
            *ans = type_info_arr + ind;
            return ERR_OK;
        }
    }
    return ERR_INVAL_OP_STR;
}

void treeWrite(FILE* fout, Node* tree)
{
    nodeWrite(fout, tree, 0);
}

void nodeWrite(FILE* fout, Node* node, int depth)
{
    if (node == NULL)
    {
        fputManyChars(fout, ' ', 4 * depth);
        fputs("0\n", fout);
        return;
    }

    fputManyChars(fout, ' ', 4 * depth);
    fputs("{\n", fout);
    fputManyChars(fout, ' ', 4 * (depth + 1));

    NodeTypeInfo *type_info = NULL;
    getTypeByCode(node->type, &type_info);
    fprintf(fout, "%s\n", type_info->type_str);

    fputManyChars(fout, ' ', 4 * (depth + 1));
    switch (node->type)
    {
        case TYPE_NUM:
            fprintf(fout, "%d\n", node->val.num);
            break;
        case TYPE_VAR:
            fprintf(fout, "%d\n", node->val.var_id);
            break;
        case TYPE_FUNC:
            printName(fout, node->val.func_name);
            fputc('\n', fout);
            break;
        case TYPE_OP:
        {
            OpInfo *op_info = NULL;
            getOpByCode(node->val.op_code, &op_info);
            fprintf(fout, "%s\n", op_info->op_str);
            break;
        }
        default:
            fputs("INVAL_TYPE\n", fout);
            break;
    }

    nodeWrite(fout, node->lft, depth + 1);
    nodeWrite(fout, node->rgt, depth + 1);

    fputManyChars(fout, ' ', 4 * depth);
    fputs("}\n", fout);
}

ErrEnum nodeRead(char* buf, int* buf_pos, Node** node, int buf_size)
{
    #define cur_buf (buf + *buf_pos)

    #define scanfSpaceChar(chr)                   \
        pos_incr = -1;                            \
        sscanf(cur_buf, " " chr "%n", &pos_incr); \
        if (pos_incr == -1) return ERR_TREE_FMT;  \
        *buf_pos += pos_incr

    #define INCR_BUF_POS(num)     \
        *buf_pos += num;          \
        if (*buf_pos >= buf_size) \
            return ERR_BUF_BOUND;

    myAssert(node != NULL && *node == NULL);

    int pos_incr = -1;
    sscanf(cur_buf, " 0%n", &pos_incr);
    if (pos_incr != -1)
    {
        *node = NULL;
        INCR_BUF_POS(pos_incr);
        return ERR_OK;
    }

    returnErr(nodeCtor(node, TYPE_NUM, {.num = 0}, NULL, NULL, NULL));
    scanfSpaceChar("{");

    skipTrailSpace(buf, buf_pos, NULL);
    NodeTypeInfo *type_info = NULL;
    returnErr(getTypeByStr(cur_buf, &type_info));
    (*node)->type = type_info->type;
    INCR_BUF_POS(type_info->type_str_len);

    skipTrailSpace(buf, buf_pos, NULL);
    switch((*node)->type)
    {
        case TYPE_FUNC:
            (*node)->val.func_name = cur_buf;
            sscanf(cur_buf, "%*s%n", &pos_incr);
            break;
        case TYPE_OP:
        {
            OpInfo *op_info = NULL;
            returnErr(getOpByStr(cur_buf, &op_info));
            (*node)->val.op_code = op_info->op_code;
            pos_incr = op_info->op_str_len;
            break;
        }
        default:
            sscanf(cur_buf, "%d%n", &((*node)->val.num), &pos_incr);
            break;
    }
    INCR_BUF_POS(pos_incr);

    returnErr(nodeRead(buf, buf_pos, &((*node)->lft), buf_size));
    returnErr(nodeRead(buf, buf_pos, &((*node)->rgt), buf_size));
    if ((*node)->lft != NULL) (*node)->lft->parent = *node;
    if ((*node)->rgt != NULL) (*node)->rgt->parent = *node;

    scanfSpaceChar("}");
    #undef cur_buf
    #undef INCR_BUF_POS
    return ERR_OK;
}


ErrEnum treeRead(const char* file_name, Node** tree, const char** buffer)
{
    myAssert(file_name != NULL && tree != NULL && *tree == NULL);

    int buf_size = 0, buf_pos = 0;
    char *buf = NULL;
    readFile(file_name, (void**)(&buf), &buf_size);
    if (buffer != NULL) *buffer = buf;
    
    returnErr(nodeRead(buf, &buf_pos, tree, buf_size));
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
            fputs("FUNC | ", fout);
            printName(fout, node->val.func_name);
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