tree grammar VVEvalInit;

options {
	tokenVocab	    = VVExpr;
    language	    = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}

@header {
	#include "hashtable.h"
//	void setGetDimIdx(pGetDimIdx getDimIdx);
//	void setGetDimItemIdx(pGetDimItemIdx getDimItemIdx);
}

@members {
//	pGetDimIdx _getDimIdx=0;
//	void setGetDimIdx(pGetDimIdx getDimIdx) {
//		_getDimIdx = getDimIdx;
//	}
//	pGetDimItemIdx _getDimItemIdx=0;
//	void setGetDimItemIdx(pGetDimItemIdx getDimItemIdx) {
//		//_getDimItemIdx = getDimItemIdx;
//	}
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

void registerLocalVariable(pANTLR3_BASE_TREE tt) {
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
void initVariable(pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
	int* contor=  hash_table_lookup(tableLocalVariable, (void*)rez, strlen(rez));
    /*
    if (contor  != NULL){
        //printf("Key found \%s\n", rez);
		t->user1 = *contor;
		t->user2 = 2; // Variabila locala
	} else{
	// Trebuie sa fie un dimension item.
		// -2 = dimensiunea curenta. -1 = Eroare.  hehe cam f urat.
		int idx = _getDimItemIdx(-2, rez);
		if ( idx == -1 ) {
			printf("Eroare initalizare AST :nu gasesc dimItem cu codul \%s \n", rez);
			return;
		}
		t->user1 = idx;
		t->user2 = 1; // Variabila din cub.
	}
    */
}
int initDimension(pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
    /*
	if (  0 == _getDimIdx ){
		printf("Eroare initalizare AST :Accesorul pentru idx dimensiune nu a foast setat ( _getDimIdx) \n");
		return;
	}
	t->user1 = _getDimIdx(rez);
	return t->user1;
    */
}
int initDimensionItem(int dim,pANTLR3_BASE_TREE tt) {
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	fillRez2(tt);
    /*
	if (  0 == _getDimItemIdx ){
		printf("Eroare initalizare AST :Accesorul pentru idx dimension item nu a foast setat ( _getDimItemIdx) \n");
		return;
	}	
	t->user1 = _getDimItemIdx(dim,rez);
	return t->user1;
    */
}

}

prog 
@init {
	tableLocalVariable = hash_table_new(MODE_COPY);
}
:
		(assignment[1])*
		(if_statement)*
		expr {
			hash_table_delete(tableLocalVariable);
		}
;

assignment[int cond] returns [ int value] :
	^('=' id=ID ex=expr) 
	{
		registerLocalVariable($ID);
	}
;
if_statement returns [int value] :
		^(a=IF log=cond_logica a1=assignment[log] (a2=assignment[!log])?)
		{
			;
		}
;

cond_logica returns [int value]:
		 ^('==' a=expr b=expr)			{$value = (a == b)? 1 : 0;}
		| ^('!=' a=expr b=expr)			{$value = (a != b)? 1 : 0;}
		| ^('>' a=expr b=expr)			{$value = (a > b)? 1 : 0;}
		| ^('<' a=expr b=expr)			{$value = (a < b)? 1 : 0;}
		| ^('>=' a=expr b=expr)			{$value = (a >= b)? 1 : 0;}
		| ^('<=' a=expr b=expr)			{$value = (a <= b)? 1 : 0;}
		| ^('<>' a=expr b=expr)			{$value = (a != b)? 1 : 0;}	 
		{
			
		}
;

expr returns [double value]:
		^('+' a=expr b=expr)  {$value = a + b;}
    |   ^('-' a=expr b=expr)  {$value = a - b;}   
    |   ^('*' a=expr b=expr)  {$value = a * b;}
    |   ^('/' a=expr b=expr)  {$value = a / b;}
	|   ^('=>' (d=dim dim_item[d])* )	
    |   ID 
        {
			initVariable($ID);
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
	
dim	returns [int value]: ID 
	{
		$value = initDimension($ID);
	}
	;
//Pun return-ul doar pentru uniformotate.	
dim_item[int dim] returns [int value]	: ID 
	{
		$value = initDimensionItem(dim,$ID);
	}
	;	
