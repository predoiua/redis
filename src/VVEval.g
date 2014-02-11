tree grammar VVEval;

options {
	tokenVocab	    = VVExpr;
    language	    = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}

@header {
//	#include "planning_formula/ParserConnection.h"
	
}

@members {

//Informatia ncesara pentru accesarea inf din cub.	
int nrDim;	// Cate dimensiuni specificate ( in urmatoarele liste pe cate pozitii am valori )
int dim_idx[10];
int dim_item_idx[10];

double variables[10];

char rez[100];
//Asta e o chestie interesanta:
// Tot stoca in token valori. Adica sa fac o singura data decodificarea char in int
// si apoi sa folosesc rez precalculate.
// printf("User1:d:\n",t->user1);
// t->user1 = 101;


void fillRez(pANTLR3_BASE_TREE tt){
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	int c1 = (int)t->start;//  ANTLR3_MARKER este ANTLR3_INT32
	int c2 = (int)t->stop;
	strncpy(rez, c1 , c2 - c1 + 1);
	rez[c2 - c1 + 1] = '\0';
}
//In acest caz este un dim item.
double getValoareDimItem(pANTLR3_BASE_TREE tt){
    /*
	if ( 0 == _getFCubeValueByDimItemId ) {
		printf("Eroare. Accesorul din parser catre aplicatie nu a fost setat.");
		return 0.;
	}
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	//printf("Tip token:\%d\n",t->user2);
	//Variabila din cub.
	if ( 1 == t->user2 ) {
		return _getFCubeValueByDimItemId( t->user1 );
	} else if ( 2 == t->user2 ) {
		return variables[ t->user1 ];
	} else {
		printf("Eroare: Nu cunosc tipull de variabila \%n in AST.\n", t->user2);
	}
	*/
	return 0.;
}
//FLOATING_POINT_LITERAL
double getValoareFloat(pANTLR3_BASE_TREE tt){
	fillRez(tt);
	double d;
	sscanf(rez, "\%lf", &d);
	return d;
}
//INT
double getValoareInt(pANTLR3_BASE_TREE tt){
	fillRez(tt);
	int d;
	sscanf(rez, "\%d", &d);
//	printf("Int \%d \n",d);
	return (double)d;
}
//ASSIGN
void slyAssign(pANTLR3_BASE_TREE tt, double val){
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	variables[ t->user1 ] = val;
	//fillRez(id);
//	printf("\%s = \%f \n", rez, val);

}

//ACCESOR
void		setDimension(pANTLR3_BASE_TREE id) {
	pANTLR3_COMMON_TOKEN    t = id->getToken(id);
	++nrDim;
	dim_idx[nrDim] = t->user1; 
}
void		setDimensionItem(pANTLR3_BASE_TREE id) {
	pANTLR3_COMMON_TOKEN    t = id->getToken(id);
	dim_item_idx[nrDim] = t->user1; 
}
double getValoareCell() {
    /*
	if ( 0 == _getFCubeValueByIds ) {
		printf("Eroare initalizare AST :Accesorul _getFCubeValueByIds nu a foast setat \n");
		return 0.;	
	}
	return _getFCubeValueByIds(nrDim,dim_idx,dim_item_idx);
    */
    return 0;
}

}

prog	[void* arg] returns [double value]:   
		(assignment[1])*
		(if_statement)*
		e=expr
        {
			$value = e; 
//			printf("Expreesie: \%f \n", e);
		}


    ;
// Pentru valoarea conditei logice = 1 = true
assignment[int cond] returns [ int value] :
	^('=' id=ID ex=expr) 
	{
			if (cond) {
				slyAssign(id, ex);
			}
	}
;
if_statement returns [int value] :
		^(a=IF log=cond_logica a1=assignment[log] (a2=assignment[log-1])?)
		{
			;//printf("cond logica este: \%d \n",log);
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
	|   ^(acc (dim dim_item)* )	
		{
			//printf("Evaluez un acces complex.\n");
			$value = getValoareCell();
		}
    |   ID 
        {
			$value = getValoareDimItem($ID);
		}
    |   FLOATING_POINT_LITERAL 
		{
			$value = getValoareFloat($FLOATING_POINT_LITERAL);
		}		
    |   INT                   
		{
			$value = getValoareInt($INT);
		}
    ;
acc : '=>' 
		{
			nrDim = -1;
		}
	;
dim	: ID 
	{
		setDimension($ID);
	}
	;
dim_item	: ID 
	{
		setDimensionItem($ID);
	}
	;
	
