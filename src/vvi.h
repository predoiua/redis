#ifndef __VVI_H
#define __VVI_H

#include "vv.h"

// cube operation
int d_set(uint32_t dim, uint32_t* cell, cell_val value );

// basic operation
int compute_index(int nr_dim, uint32_t* dis, uint32_t* dims, size_t* res );
int di_is_simple( uint32_t dim_in_cube, dim* dim, uint32_t* cell );

int set_simple_cell_value_at_index_(void* ptr, size_t idx, cell_val value);
int get_cube_value_at_index_(void  *ptr, size_t idx, cell_val* value);

#endif
