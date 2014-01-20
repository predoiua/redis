/*
 * vvcp.h
 *
 *	Cartesian Product.
 *  Created on: Jan 20, 2014
 *      Author: vagrant
 */

#ifndef VVCP_H_
#define VVCP_H_
#include "sds.h"


typedef struct {
	uint32_t 	nr_elem;
	uint32_t	me; // current position
	uint32_t    level;
	uint32_t*  	elem;
} cp_di;

typedef struct{
	uint32_t 	nr_elem;
	cp_di*   elem;
} cp_dim_list

typedef struct{
	uint32_t 	nr_elem;
	cell*   	cell;
} cell_list;

//cart_prod( QMap <int, QList<int> >& data_in, QList<QList<int> >& data_out)
void cp(cp_dim_list *data_is, cell_list *data_out ) {
	for(int i=0; i< data_is->nr_elem; ++i){

	}
	while(1){

	}
}

#endif /* VVCP_H_ */
