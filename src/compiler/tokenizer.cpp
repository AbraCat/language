#include <stdlib.h>
#include <string.h>

#include <tokenizer.h>
#include <utils.h>
#include <str.h>

const int max_names = 50;

ErrEnum tokenize(const char* fin_name, Node** node_arr, int* n_nodes, NameArr* name_arr, const char** prog_text)
{
    #define CUR_NODE ((*node_arr)[cur_node])
    myAssert(fin_name != NULL && node_arr != NULL && *node_arr == NULL && n_nodes != NULL && name_arr != NULL);

    char* buf = NULL;
    int buf_size = 0;
    returnErr(readFile(fin_name, (void**)&buf, &buf_size));
    if (prog_text != NULL) *prog_text = buf;

    int buf_pos = 0, eof = 0, pos_incr = 0, cur_node = 0;
    *node_arr = (Node*)calloc(buf_size, sizeof(Node));

    returnErr(insertName("input", name_arr, NULL));
    name_arr->names[0].type = NAME_FUNC;
    name_arr->names[0].n_args = 0;
    returnErr(insertName("output", name_arr, NULL));
    name_arr->names[1].type = NAME_FUNC;
    name_arr->names[1].n_args = 1;

    for (; cur_node < buf_size; ++cur_node)
    {
        skipTrailSpace(buf, &buf_pos, &eof);
        if (buf[buf_pos] == '\0') break;

        OpInfo *op_info = NULL;
        if (getOpByStr(buf + buf_pos, &op_info) == ERR_OK)
        {
            CUR_NODE.type = TYPE_OP;                                
            CUR_NODE.val.op_code = op_info->op_code;                     
            buf_pos += op_info->op_str_len;                             
            continue;  
        }

        if (sscanf(buf + buf_pos, "%d%n", &(CUR_NODE.val.num), &pos_incr) == 1)
        {
            buf_pos += pos_incr;
            CUR_NODE.type = TYPE_NUM;
            continue;
        }

        CUR_NODE.type = TYPE_VAR;
        returnErr(insertName(buf + buf_pos, name_arr, &(CUR_NODE.val.var_id)));
        int name_len = nameLen(buf + buf_pos);
        if (name_len == 0) return ERR_INVAL_TOKEN;
        buf_pos += name_len;
    }
    CUR_NODE.type = TYPE_OP;
    CUR_NODE.val.op_code = OP_END;
    ++cur_node;
    
    *n_nodes = cur_node;
    return ERR_OK;
}

ErrEnum nameArrCtor(NameArr* name_arr)
{
    myAssert(name_arr != NULL);
    name_arr->max_names = max_names;
    name_arr->n_names = 0;

    name_arr->names = (Name*)calloc(max_names, sizeof(Name));
    if (name_arr->names == NULL) return ERR_MEM;

    return ERR_OK;
}

void nameArrDtor(NameArr* name_arr)
{
    if (name_arr == NULL) return;
    free(name_arr->names);
}

ErrEnum insertName(const char* name_str, NameArr* name_arr, int* name_id)
{
    myAssert(name_str != NULL && name_arr != NULL);

    for (int ind = 0; ind < name_arr->n_names; ++ind)
        if (nameCmp(name_str, name_arr->names[ind].name_str, NULL) == 0)
        {
            if (name_id != NULL) *name_id = ind;
            return ERR_OK;
        }
    if (name_arr->n_names >= max_names) return ERR_TOO_MANY_NAMES;

    name_arr->names[name_arr->n_names].name_str = name_str;
    name_arr->names[name_arr->n_names].type = NAME_UNDECL;
    name_arr->names[name_arr->n_names].n_args = -1;

    if (name_id != NULL) *name_id = name_arr->n_names;
    ++name_arr->n_names;
    return ERR_OK;
}