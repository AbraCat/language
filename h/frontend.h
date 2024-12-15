#ifndef FRONTEND_H
#define FRONTEND_H

#include <error.h>
#include <tokenizer.h>

/*
grammar:

G = F_DECL{,F_DECL}*              +
F_DECL = func NAME(VAR{,VAR})BODY ++
BODY = (S{,S}*)                   ++
S = VAR_DECL|IF|WHILE|RET|A|E1
VAR_DECL = var VAR
A = VAR=E1
IF = if(E1)(BODY)(BODY)
WHILE = while(E1)BODY
RET = return E1

E1 = E2{[<]E2}*
E2 = E3{[+-]E3}*
E3 = E4{[*(slash)]E4}*
E4 = P{^P}*
P = (E0)|NUM|FUNC|VAR
FUNC = NAME(E1{,E1}*)             ++
NUM = {-}[0-9]+{.[0-9]+}
NAME = [a]{[a1]}*


errors with names:

func not declared                        - caught by assembler
func redeclared                          - not error, 1st definition used
func used with wrong number of arguments + check n_args 
var not declared                         + check type
var redeclared                           + check type
var and func with same name              + check type
*/

// NodeType: OP (if else while , '(' ')' < <= > >= == != + - * / ^ = func var $), VAR, FUNC, NUM

struct SyntaxErr
{
    int err, pos;
    const char *exp;
    Node* got;
};

struct Parser
{
    Node* s;
    int p, n_nodes;
    SyntaxErr synt_err;

    Node* root;
    NameArr name_arr;

    // ind in name_arr of function currently being defined. 
    // var decls store this id in Name and are undeclared at the end of definition
    // -1 when not in function definition
    int cur_func; 
};

ErrEnum runFrontend(const char* fin_name, Node** tree);

#endif // FRONTEND_H