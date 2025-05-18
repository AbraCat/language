#ifndef TREE_H
#define TREE_H

#include <stdio.h>

#include <error.h>

extern const int n_ops, n_types, name_buf_size;
const int small_buf_size = 100;

enum NodeType
{
    TYPE_OP,
    TYPE_VAR,
    TYPE_FUNC,
    TYPE_NUM,
};

enum OpEnum
{
    #define OP_CODEGEN(name, n_operands, value, priority, text, asm_instr) OP_ ## name,
    #include <operations.h>
    #undef OP_CODEGEN
};

enum NodeChild
{
    LFT_NODE,
    RGT_NODE
};

struct NodeTypeInfo
{
    NodeType type;
    const char *type_str;
    const int type_str_len;
};

struct OpInfo
{
    OpEnum op_code;
    const char *op_code_str, *op_str, *asm_instr;
    const int op_str_len, priority;
};

union NodeVal
{
    int num;
    int var_id; // index in NameArr before syntax analysis, local var number after
    OpEnum op_code;
    const char* func_name;
};

struct Node
{
    NodeType type;
    NodeVal val;
    Node *parent, *lft, *rgt;

    // debug
    int visited;
};

ErrEnum nodeCtor(Node** node, NodeType type, NodeVal val, Node* parent, Node* lft, Node* rgt);
void nodeDtor(Node* node);

void nNodes(Node* node, int* ans);
ErrEnum nodeVerify(Node* node);

ErrEnum getOpByCode(OpEnum op_code, OpInfo** ans);
ErrEnum getOpByStr(const char* op_str, OpInfo** ans);
ErrEnum getTypeByCode(NodeType type, NodeTypeInfo** ans);
ErrEnum getTypeByStr(const char* type_str, NodeTypeInfo** ans);

ErrEnum printNodeDot(FILE* fout, Node* node);
ErrEnum treeMakeGraph(Node* tree);
ErrEnum treeDump(Node* tree);

void nodeWrite(FILE* fout, Node* node, int depth);
void treeWrite(FILE* fout, Node* tree);
ErrEnum nodeRead(char* buf, int* buf_pos, Node** node, int buf_size);
ErrEnum treeRead(const char* file_name, Node** tree, const char** buffer);

ErrEnum nodeCopy(Node* src, Node** dest);
ErrEnum connectLinear(Node* nodes, int n_nodes);
bool treeEqual(Node* tree1, Node* tree2);

#endif // TREE_H