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
    clearComments(buf, '#');

    int buf_pos = 0, eof = 0, pos_incr = 0, cur_node = 0;
    *node_arr = (Node*)calloc(buf_size, sizeof(Node));

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
