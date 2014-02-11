grammar VVExpr;
options {
	output	    = AST;
	language    = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}
/*
Ex de programe:
1. ap1 + 101 
2. pi = 3.14; if (pi == 3.14) then { rez = 3.14; } else { rez = 3; }; rez;
3. 
*/
prog
options{
	backtrack=true;
	}:
	asignment*
	ifStmt*
	expr
	
	;
	
asignment:
	ID '=' expr ';' -> ^('=' ID expr)	
	;	
// Ex: [time=>ap01;version=>budget;]	
accesor:
//	'[' ( dim=ID '=>' dim_item=ID ';')+ ']' -> ^('=>' $dim+ $dim_item+ ) 	
	'[' ( ID ('=>'|';') )+ ']' -> ^('=>' ID+ ) 	

	;
		
expr:
	   multExpr (('+'^|'-'^) multExpr)*
    ; 

multExpr:
    atomComplex ( ('*'^|'/'^) atomComplex)*
    ; 
atomComplex
	:	atom	-> atom
    |   '(' expr ')' ->expr	;
atom:   
		INT 
    |   FLOATING_POINT_LITERAL
    |   ID
	|   accesor
    ;
  
logicalExpr
	: equalityExpr
	;

equalityExpr
	:	comparisonExpr (('=='|'!='|'<>')^ comparisonExpr)*
;
	
comparisonExpr
	:	additiveExpr (('>'|'<'|'<='|'>=')^ additiveExpr)*
;
additiveExpr
	:	atom
;


ifStmt:	
	IF '(' l=logicalExpr ')' '{' c1=asignment '}' ('else' '{' c2=asignment '}')? -> ^(IF $l $c1 $c2)
	//| IF '(' l=logicalExpr ')' '{' c1=expr '}' -> ^(IF $l $c1 'NOP')
	;

NOT	:	'!'|'not'
	;
	

IF	:	'if'
	;
	
ID  
	:   ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'_'|'%'|('0'..'9'))* 
	;

INT
    :   ('1'..'9')('0'..'9')*
	;

FLOATING_POINT_LITERAL
    :   ('0'..'9')+ '.' ('0'..'9')* Exponent? FloatTypeSuffix?
    |   '.' ('0'..'9')+ Exponent? FloatTypeSuffix?
    |   ('0'..'9')+ ( Exponent FloatTypeSuffix? | FloatTypeSuffix)
    ;

fragment
Exponent : ('e'|'E') ('+'|'-')? ('0'..'9')+ ;

fragment
FloatTypeSuffix : ('f'|'F'|'d'|'D') ;


WS  :  (' '|'\r'|'\t'|'\u000C'|'\n') {$channel=HIDDEN;}
    ;
	
