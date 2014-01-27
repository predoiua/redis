#include <stdio.h>
#include <stdlib.h>
#include "vvi.h"

char* cube_d;
char* cube_m;
char* dim_m_struct;
char* dim_m_measure;
char* dim_m_time;

char* di_data;
void print_cube(char* cd){
	cube _c;
	int i,v;
	initCube(_c,cd);
	int nr_dim = getCubeNrDim(_c);
	printf("Nr dims:%d\n", nr_dim);
	for(i=0; i< nr_dim; ++i){
		v = getCubeNrDi(_c,i);
		printf("-pos:%d val:%d\n", i, v);
	}
	printf("===========\n");
}
void print_dim(char* cd){
//	dim _d;
//	int i,v, nr_di;
//	initDim(_d,cd);
//	nr_di = getDimNrDi(_d);
//	printf("Nr di in dims:%d\n", nr_di);
//	for(i=0; i< nr_di; ++i){
//		v = getDimDiOrder(_d,i);
//		printf("-pos:%d order:%d\n", i, v);
//	}

}
void init(){

	int nr_dim= 3;
	cube_m = malloc( cubeStructSize(nr_dim) );
	cube _c;
	initCube(_c,cube_m);
	setCubeNrDims(_c,nr_dim);
	setCubeNrDi(_c,0,4); // Measure = 4
	setCubeNrDi(_c,1,7); // structure = 7
	setCubeNrDi(_c,2,3); // time = 3
/*
	int nr_dim_cur;
	dim _d;
	nr_dim_cur = 4;
	dim_m_struct = malloc( dimStructSize(nr_dim_cur));
	initDim(_d,dim_m_struct);
	setDimNrDi(_d,nr_dim_cur);
	setDimDiOrder(_d,0,0); // qty
	setDimDiOrder(_d,1,0); // price
	setDimDiOrder(_d,2,1); // ns = qty * price
	setDimDiOrder(_d,3,2); // avg_price = ns/qty
*/
	printf("Done init.\n");
}
int main(){
	init();
	print_cube(cube_m);
	print_dim(dim_m_struct);
	printf("done.\n");
	return 0;
}
