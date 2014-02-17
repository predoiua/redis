tree grammar VVEval;

options {
	tokenVocab	    = VVExpr;
    language	    = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}

@header {
#include "vvformula.h"	
}

@members {

//Informatia ncesara pentru accesarea inf din cub.	
int nrDim;	// Cate dimensiuni specificate ( in urmatoarele liste pe cate pozitii am valori )
int dim_idx[10];
int dim_item_idx[10];

double variables[10];

char rez[100];
//Asta e o chestie interesanta:
// Pot stoca in token valori. Adica sa fac o singura data decodificarea char in int
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
double getValoareDimItem(struct formula_struct *_formula,pANTLR3_BASE_TREE tt){
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	//printf("Tip token:\%d\n",t->user2);
	//Variabila din cub.
	if ( 1 == t->user2 ) {
		double d = _formula->getValueByDimItemId(_formula, t->user1);// _getFCubeValueByDimItemId( t->user1 );
		//printf("Cube cube accessor: Dim Item:\%d Get value:\%f\n",t->user1, d);
		return d;
	} else if ( 2 == t->user2 ) {
		return variables[ t->user1 ];
	} else {
		printf("Eroare: Nu cunosc tipull de variabila \%n in AST.\n", t->user2);
	}
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
void slyAssign(struct formula_struct *_formula, pANTLR3_BASE_TREE tt, double val){
	pANTLR3_COMMON_TOKEN    t = tt->getToken(tt);
	variables[ t->user1 ] = val;
	//fillRez(id);
//	printf("\%s = \%f \n", rez, val);

}

//ACCESOR
void		setDimension(struct formula_struct *_formula, pANTLR3_BASE_TREE id) {
	pANTLR3_COMMON_TOKEN    t = id->getToken(id);
	++nrDim;
	dim_idx[nrDim] = t->user1; 
}
void		setDimensionItem(struct formula_struct *_formula, pANTLR3_BASE_TREE id) {
	pANTLR3_COMMON_TOKEN    t = id->getToken(id);
	dim_item_idx[nrDim] = t->user1; 
}
double getValoareCell(struct formula_struct *_formula) {
	return _formula->getValueByIds(_formula, nrDim + 1, dim_idx, dim_item_idx);
}

}

prog	[struct formula_struct *_formula] returns [double value]:   
		(assignment[_formula,1])*
		(if_statement[_formula])*
		e=expr[_formula]
        {
			$value = e; 
			//printf("Result: \%f \n", e);
		}


    ;
// Pentru valoarea conditei logice = 1 = true
assignment[struct formula_struct *_formula,int cond] returns [ int value] :
	^('=' id=ID ex=expr[_formula]) 
	{
			if (cond) {
				slyAssign(_formula, id, ex);
			}
	}
;
if_statement[struct formula_struct *_formula] returns [int value] :
		^(a=IF log=cond_logica[_formula] a1=assignment[_formula, log] (a2=assignment[_formula, log-1])?)
		{
			;//printf("cond logica este: \%d \n",log);
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
	|   ^(acc[_formula] (dim[_formula] dim_item[_formula])* )	
		{
			//printf("Evaluez un acces complex.\n");
			$value = getValoareCell(_formula);
		}
    |   ID 
        {
			$value = getValoareDimItem(_formula, $ID);
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
acc[struct formula_struct *_formula] : '=>' 
		{
			nrDim = -1;
		}
	;
dim[struct formula_struct *_formula]	: ID 
	{
		setDimension(_formula, $ID);
	}
	;
dim_item[struct formula_struct *_formula]	: ID 
	{
		setDimensionItem(_formula, $ID);
	}
	;
	
