#ifndef __VVI_H
#define __VVI_H

#include "vv.h"

// cube operation
int d_set(dim _dim, uint32_t dim, cell _cell, cell_val value );

// basic operation
int compute_index(cube _cube,  cell _cell, size_t* res );
int di_is_simple(dim _dim, uint32_t dim, cell _cell );


int set_simple_cell_value_at_index_(void* ptr, size_t idx, cell_val value);
int get_cube_value_at_index_(void  *ptr, size_t idx, cell_val* value);

#endif
