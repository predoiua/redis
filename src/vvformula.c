

#include "redis.h"
#include <string.h>
#include <stdio.h>

#include "vvformula.h"
#include "vvdb.h"
#include "vvfct.h"
#include "debug.h"

static int         free_formula   (struct formula_struct *_formula);
static double      eval   (struct formula_struct *_formula,void* _cell);

static int         getDimIdx      (struct formula_struct *_formula,char* dim_code);
static int         getDimItemIdx  (struct formula_struct *_formula,int dim_idx, char* di_code);

static double      getValueByDimItemId (struct formula_struct *_formula,int di_idx);
static double      getValueByIds (struct formula_struct *_formula,int nr_dim, int* dim, int* dim_item);


static int         dummy  (struct formula_struct *_formula);

formula* formulaNew(
		void *_db,
        void *_cube,
        void *_cell,
        int _dim_idx,
        const char* _program) {

	formula* res = (formula*)sdsnewlen(NULL,sizeof(formula));

    //formula* res = malloc(sizeof(formula));
    //Init fuction
    res->eval = eval;
    res->free = free_formula;
    res->getDimIdx = getDimIdx;
    res->getDimItemIdx = getDimItemIdx;
    res->getValueByDimItemId = getValueByDimItemId;
    res->getValueByIds = getValueByIds;
    res->dummy = dummy;
    //Link with the rest of the application
    res->dim_idx = _dim_idx;
    res->cell = _cell;


    //Init members
    res->vvdb = NULL;
    res->program = NULL;
    res->input = NULL;
    res->lex = NULL;
    res->tokens = NULL;
    res->parser  = NULL;

    //exprAST;
    res->nodes = NULL ;
    res->treePsr = NULL;
    res->treePsrInit = NULL;

    //INFO("Before vvdb build.");
    res->vvdb = vvdbNew( (redisDb*)_db, (cube*)_cube);
    if ( res->vvdb == NULL ) {
    	INFO("ERROR: Fail to build vvdb");
    	res->free(res);
    	return NULL;
    }
    // Build parser
    //res->program = strdup(_program); // create a copy
    res->program = sdsnew(_program);
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
    res->treePsrInit->prog(res->treePsrInit, res);
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

    if( _formula->vvdb != NULL ) {
    	((vvdb*)_formula->vvdb)->free((vvdb*)_formula->vvdb );
    	_formula->vvdb = NULL;
    }
    if( _formula->program != NULL ) {
    	//free(_formula->program);
    	sdsfree((sds)_formula->program );
    	_formula->program = 0;
    }
	sdsfree((sds)_formula );
	_formula = 0;
    return 0;
}

static int         getDimIdx      (struct formula_struct *_formula,char* dim_code){
	vvdb* _vvdb = (vvdb*)_formula->vvdb;
	return _vvdb->getDimIdx(_vvdb, dim_code);
    /*
    DDimension* dim = _cube->getChild(QString(cod_dim));
    if (dim == NULL) {
        qDebug() << "Eval context(formula): Nu gasesc dimensiunea  :" << QString(cod_dim) << "  in cubul " << _cube->getCode();
        return -1;
    }
    return dim->getPosition();
    */
}

static int         getDimItemIdx  (struct formula_struct *_formula,int dim_idx, char* di_code){
	int real_dim = dim_idx;
    if ( -2 == dim_idx ) {//By Convention: -2 = current dimension
    	real_dim = _formula->dim_idx;
    }
	vvdb* _vvdb = (vvdb*)_formula->vvdb;
	return _vvdb->getDimItemIdx(_vvdb,real_dim, di_code);
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
}

static double      getValueByDimItemId (struct formula_struct *_formula, int di_idx){
	vvdb* _vvdb = (vvdb*)_formula->vvdb;
	cell* _cell = cellBuildCell((cell*)_formula->cell); //_formula->cell;
	setCellIdx(_cell, _formula->dim_idx, di_idx );
	cell_val* cv = _vvdb->getCellValue(_vvdb, _cell);

//	// Print some details about what i'm doing
//	sds s = sdsempty();
//	for(int i=0; i < _cell->nr_dim; ++i ){
//		s = sdscatprintf(s,"%d ", (int)getCellDiIndex(_cell, i) );
//	}
//	redisLog(REDIS_WARNING, "Accessor: idx:%s, value:%f ", s, cv->val );
//	sdsfree(s);
	cellRelease(_cell);
	return cv->val;
}

static double      getValueByIds (struct formula_struct *_formula,int nr_dim, int* dim, int* dim_item){
	vvdb* _vvdb = (vvdb*)_formula->vvdb;
	cell* _cell = cellBuildCell(_formula->cell);
    for(int i=0; i<nr_dim;++i) {
    	setCellIdx(_cell, dim[i], dim_item[i]);
    }
	cell_val* cv = _vvdb->getCellValue(_vvdb, _cell);
	cellRelease(_cell);

	return cv->val;
}

static double      eval   (struct formula_struct *_formula, void* _cell){
    _formula->nodes   ->reset(_formula->nodes);
    _formula->treePsr ->reset(_formula->treePsr);
    double val;
    val = _formula->treePsr->prog(_formula->treePsr, _formula);

    return val;
}

static int         dummy  (struct formula_struct *_formula) {
	printf("Hello. I am a dummy function!");
	return 100;
}
