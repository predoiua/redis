#include <string.h>

#include "vvformula.h"


static int         free_formula   (struct formula_struct *_formula);
static double      eval   (struct formula_struct *_formula,void* _cell);

static int         getDimIdx      (struct formula_struct *_formula,char* dim_code);
static int         getDimItemIdx  (struct formula_struct *_formula,int dim_idx, char* di_code);

static double      getValueByDimItemId (struct formula_struct *_formula,int di_idx);
static double      getValueByIds (struct formula_struct *_formula,int nr_dim, int* dim, int* dim_item);

formula* formulaNew(
        void *_cube, int _dim_idx, int _di_idx,
        const char* _program) {
    formula* res = malloc(sizeof(formula));
    //Init fuction
    res->free = free_formula;
    res->getDimIdx = getDimIdx;
    res->getDimItemIdx = getDimItemIdx;
    res->getValueByDimItemId = getValueByDimItemId;
    res->getValueByIds = getValueByIds;

    //Link with the rest of the application
    res->cube = _cube;
    res->dim_idx = _dim_idx;
    res->di_idx = _di_idx;


    //Init members
    res->program = NULL;
    res->input = NULL;
    res->lex = NULL;
    res->tokens = NULL;
    res->parser  = NULL;

    //exprAST;
    res->nodes = NULL ;
    res->treePsr = NULL;
    res->treePsrInit = NULL;

    // Build parser
    res->program = strdup(_program); // create a copy
    // pt 3.3 - 3.4
    res->input = antlr3StringStreamNew( (pANTLR3_UINT8)res->program,
                                   (ANTLR3_UINT32)ANTLR3_ENC_8BIT,
                                   (ANTLR3_UINT32) strlen(res->program),
                                   (pANTLR3_UINT8)res->program );
    // The input will be created successfully, providing that there is enough
    // memory and the file exists etc
    if ( res->input == NULL){
        //fprintf(stderr, "Failed to create stream for : %s\n", (char *)_program);
        return NULL;
    }
    res->lex    = VVExprLexerNew                (res->input);
    if ( res->lex == NULL ){
        //fprintf(stderr, "Unable to create the lexer due to malloc() failure1\n");
        return NULL;
    }
    res->tokens = antlr3CommonTokenStreamSourceNew  (ANTLR3_SIZE_HINT, TOKENSOURCE(res->lex));
    if ( res->tokens == NULL ){
        //fprintf(stderr, "Out of memory trying to allocate token stream\n");
        return NULL;
    }
    res->parser = VVExprParserNew              (res->tokens);
    if (res->parser == NULL){
        //fprintf(stderr, "Out of memory trying to allocate parser\n");
        return NULL;
    }

    res->exprAST = res->parser  ->prog(res->parser);
    if ( res->parser->pParser->rec->state->errorCount > 0){
        //fprintf(stderr, "The parser returned %d errors, tree walking aborted.\n", parser->pParser->rec->state->errorCount);
        return NULL;
    }

    res->nodes	= antlr3CommonTreeNodeStreamNewTree(res->exprAST.tree, ANTLR3_SIZE_HINT); // sIZE HINT WILL SOON BE DEPRECATED!!
    //Pentru Initializare
    res->treePsrInit	= VVEvalInitNew(res->nodes);

    //Completeaza valori precalculate in AST
    res->treePsrInit->prog(res->treePsrInit);
    //Pentru calcularea propriu zisa a expresiu
    res->treePsr	= VVEvalNew(res->nodes);

    return res;
}

static int			free_formula   (struct formula_struct *_formula ) {

    if( _formula->nodes != NULL)          _formula->nodes   ->free  (_formula->nodes);
    if( _formula->treePsr	!= NULL)        _formula->treePsr ->free  (_formula->treePsr);
    if( _formula->treePsrInit != NULL)    _formula->treePsrInit ->free  (_formula->treePsrInit);

    if(_formula->parser != NULL) _formula->parser ->free(_formula->parser);
    if(_formula->tokens != NULL) _formula->tokens ->free(_formula->tokens);
    if(_formula->lex    != NULL) _formula->lex    ->free(_formula->lex);
    if(_formula->input  != NULL) _formula->input  ->free (_formula->input);

    if( _formula->program != NULL ) free(_formula->program);
    return 0;
}

static int         getDimIdx      (struct formula_struct *_formula,char* dim_code){
    /*
    DDimension* dim = _cube->getChild(QString(cod_dim));
    if (dim == NULL) {
        qDebug() << "Eval context(formula): Nu gasesc dimensiunea  :" << QString(cod_dim) << "  in cubul " << _cube->getCode();
        return -1;
    }
    return dim->getPosition();
    */
    return 0;
}

static int         getDimItemIdx  (struct formula_struct *_formula,int dim_idx, char* di_code){
    /*
    DDimension* d;
    //Conventie: -2 = dimensiunea curenta
    if ( -2 == dim ) {
        d = _dim;
    } else {
        d = _cube->getChild(dim);
        if (d == NULL) {
            qDebug() << "Eval context(formula): Nu gasesc dimensiunea cu index-ul  :" << dim;
            return -1;
        }
    }

    DDimensionItem* di = d->getDimItem(QString(cod_dim_item));
    if (di == NULL) {
        qDebug() << "Eval context(formula): Nu gasesc item-ul  :" << QString(cod_dim_item) << "  in dimensiune " << _dim->getCode();
        return -1;
    }
    return di->getPosition();
    */
    return 0;
}

static double      getValueByDimItemId (struct formula_struct *_formula,int di_idx){
    /*
    DDimensionItem* di = _dim->getDimItem(dim_item);
    if (di == NULL) {
        qDebug() << "Eval context(formula): Invalid dimension position :" << dim_item << "  in dimension " << _dim->getCode();
        return 0.;
    }
    int pos = _dim->getPosition();
    _idx[pos] = di->getPosition();

    CellValue cv =  _storage->getValue(_idx);
    if ( ! cv.isDouble() ) {
        QString whatIs;
        if ( cv.isInt() )
            whatIs = "Dimension";
        if ( cv.isString() )
            whatIs += "String";

        qDebug() << "Eval context(formula): " << di->getCode() << " is not double, is "<< whatIs << "idx:" << _idx;
        return 0.;
    }
    return cv.getDouble();
    */
    return 0;
}

static double      getValueByIds (struct formula_struct *_formula,int nr_dim, int* dim, int* dim_item){
/*
    QList<int> idx(_idx);
    // !!! Atentie: este corect cu <=.
    for(int i=0; i<=nr_dim;++i) {
        idx[ dim[i] ] = dim_item[i];
    }

    CellValue cv =  _storage->getValue(idx);
    if ( ! cv.isDouble() ) {
        QString whatIs;
        if ( cv.isInt() )
            whatIs = "Dimension";
        if ( cv.isString() )
            whatIs += "String";

        qDebug() << "Eval context(formula): required cell is not double, is "<< whatIs << "idx:" << idx;
        return 0.;
    }
    //qDebug() << "acces complex:" << idx << " val:" <<  cv.getDouble();
    return cv.getDouble();
*/
    return 0;
}

static double      eval   (struct formula_struct *_formula, void* _cell){
    _formula->nodes   ->reset(_formula->nodes);
    _formula->treePsr ->reset(_formula->treePsr);
    double val;
    val = _formula->treePsr->prog(_formula->treePsr, 0);

    return val;
}

