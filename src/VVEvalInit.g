tree grammar VVEvalInit;

options {
	tokenVocab	    = VVExpr;
    language	    = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}

@header {
	#include "hashtable.h"
	#include "vvformula.h"
}

@members {
int contorDimItem = -1;	
int contorVariabile = -1;

hash_table_t *tableLocalVariable = NULL;

char rez[100];

void fillRez2(pANTLR3_BASE_TREE tt){
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	int c1 = (int)t->start;//  ANTLR3_MARKER este ANTLR3_INT32
	int c2 = (int)t->stop;
	strncpy(rez, c1 , c2 - c1 + 1);
	rez[c2 - c1 + 1] = '\0';
}

void registerLocalVariable(struct formula_struct *_formula, pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
	int* contor=  hash_table_lookup(tableLocalVariable, (void*)rez, strlen(rez));
    if (contor  != NULL){
        //printf("Key found \%s\n", rez);
		t->user1 = *contor;
		t->user2 = 2; // Variabila locala
    } else{
		t->user1 = ++contorVariabile;
		t->user2 = 2; // Variabila locala

        //printf("Key NOT found \%s\n", rez);
		hash_table_add(tableLocalVariable, (void*)rez, strlen(rez), (void *) &contorVariabile, sizeof(contorVariabile));
    }
}
void initVariable(struct formula_struct *_formula, pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
	int* contor=  hash_table_lookup(tableLocalVariable, (void*)rez, strlen(rez));
    if (contor  != NULL){
        //printf("Key found \%s\n", rez);
		t->user1 = *contor;
		t->user2 = 2; // Variabila locala
	} else{
	// Trebuie sa fie un dimension item.
		// -2 = dimensiunea curenta. -1 = Eroare.  hehe cam f urat.
		int idx = _formula->getDimItemIdx(_formula,-2, rez);
		if ( idx == -1 ) {
			printf("Eroare initalizare AST :nu gasesc dimItem cu codul \%s \n", rez);
			return;
		}
		t->user1 = idx;
		t->user2 = 1; // Variabila din cub.
	}
}
int initDimension(struct formula_struct *_formula, pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
	t->user1 = _formula->getDimIdx(_formula, rez);
	return t->user1;
}
int initDimensionItem(struct formula_struct *_formula, int dim,pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
	t->user1 = _formula->getDimItemIdx(_formula, dim, rez);
	return t->user1;
}

}

prog [struct formula_struct *_formula]
@init {
	tableLocalVariable = hash_table_new(MODE_COPY);
	int val = _formula->dummy(_formula);
	printf("From dummy i've got : \%d", val);
	int dim = _formula->getDimIdx(_formula,"measure_sales");
	printf("Idx for measure_sales is : \%d", dim);
}
:
		(assignment[_formula,1])*
		(if_statement[_formula])*
		expr[_formula] {
			hash_table_delete(tableLocalVariable);
		}
;

assignment[struct formula_struct *_formula,int cond] returns [ int value] :
	^('=' id=ID ex=expr[_formula]) 
	{
		registerLocalVariable(_formula,$ID);
	}
;
if_statement[struct formula_struct *_formula] returns [int value] :
		^(a=IF log=cond_logica[_formula] a1=assignment[_formula, log] (a2=assignment[_formula, !log])?)
		{
			;
		}
;

cond_logica[struct formula_struct *_formula] returns [int value]:
		 ^('==' a=expr[_formula] b=expr[_formula])			{$value = (a == b)? 1 : 0;}
		| ^('!=' a=expr[_formula] b=expr[_formula])			{$value = (a != b)? 1 : 0;}
		| ^('>' a=expr[_formula] b=expr[_formula])			{$value = (a > b)? 1 : 0;}
		| ^('<' a=expr[_formula] b=expr[_formula])			{$value = (a < b)? 1 : 0;}
		| ^('>=' a=expr[_formula] b=expr[_formula])			{$value = (a >= b)? 1 : 0;}
		| ^('<=' a=expr[_formula] b=expr[_formula])			{$value = (a <= b)? 1 : 0;}
		| ^('<>' a=expr[_formula] b=expr[_formula])			{$value = (a != b)? 1 : 0;}	 
		{
			
		}
;

expr[struct formula_struct *_formula] returns [double value]:
		^('+' a=expr[_formula] b=expr[_formula])  {$value = a + b;}
    |   ^('-' a=expr[_formula] b=expr[_formula])  {$value = a - b;}   
    |   ^('*' a=expr[_formula] b=expr[_formula])  {$value = a * b;}
    |   ^('/' a=expr[_formula] b=expr[_formula])  {$value = a / b;}
	|   ^('=>' (d=dim[_formula] dim_item[_formula, d])* )	
    |   ID 
        {
			initVariable(_formula,$ID);
		}
    |   FLOATING_POINT_LITERAL 
		{
			;
		}		
    |   INT                   
		{
			;
		}
    ;		
	
dim[struct formula_struct *_formula]	returns [int value]: ID 
	{
		$value = initDimension(_formula,$ID);
	}
	;
//Pun return-ul doar pentru uniformotate.	
dim_item[struct formula_struct *_formula, int dim] returns [int value]	: ID 
	{
		$value = initDimensionItem(_formula,dim,$ID);
	}
	;	
