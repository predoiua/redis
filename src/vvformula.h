#ifndef _VVFORMULA_H
#define _VVFORMULA_H

typedef struct formula;
#include "VVExprLexer.h"
#include "VVExprParser.h"
#include "VVEval.h"
#include "VVEvalInit.h"

typedef struct formula_struct{
    void                            *cube;
    int                             dim_idx;
    int                             di_idx;
    char                            *program;

    pANTLR3_INPUT_STREAM                input;
    pVVExprLexer                        lex;
    pANTLR3_COMMON_TOKEN_STREAM         tokens;
    pVVExprParser                       parser;

    VVExprParser_prog_return            exprAST;
    pANTLR3_COMMON_TREE_NODE_STREAM     nodes;
    pVVEval                             treePsr;
    pVVEvalInit                         treePsrInit;

    int         (*getDimIdx)      (struct formula_struct *_formula, char* dim_code);
    int         (*getDimItemIdx)  (struct formula_struct *_formula, int dim_idx, char* di_code);

    double      (*getValueByDimItemId) (struct formula_struct *_formula, int di_idx);
    double      (*getValueByIds)(struct formula_struct *_formula,int nr_dim, int* dim, int* dim_item);

    int         (*free)   (struct formula_struct *_formula);
    double      (*eval)   (struct formula_struct *_formula, void* _cell);

} formula;

formula* formulaNew(
        void *_cube, int _dim_idx, int _di_idx,
        const char* _program);
#endif
