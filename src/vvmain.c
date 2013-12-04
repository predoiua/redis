#include <stdio.h>
#include <stdlib.h>
#include "vvi.h"

char* cube_data;
char* dim_data;
char* di_data;
void print_cube(char* cd){
	cube _c;
	initCube(_c,cd);
	int nr_dim = getCubeNrDim(_c);
	printf("Nr dims:%d\n", nr_dim);
	int i,v;
	for(i=0; i< nr_dim; ++i){
		v = getCubeNrDi(_c,i);
		printf("-pos:%d val:%d\n", i, v);
	}
}
void init(){
	int nr_dim= 3;
	cube_data = malloc(sizeof(uint32_t)*(nr_dim + 1));
	cube _c;
	initCube(_c,cube_data);
	setCubeNrDims(_c,nr_dim);
	setCubeNrDi(_c,0,2);
	setCubeNrDi(_c,1,3);
	setCubeNrDi(_c,2,4);

}
int main(){
	init();
	print_cube(cube_data);
	printf("done.\n");
	return 0;
}
