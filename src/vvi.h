#ifndef __VVI_H
#define __VVI_H

#include "vv.h"
// Based on initial work
// cube operation
int c_set(cube *_cube, cell *_cell, cell_val value);

/*
int c_break_back(dim _dim, cell _cell, cell_val value);

// basic operation
int compute_index(cube _cube,  cell _cell ); // set idx field in _cell
int di_is_simple(dim _dim, uint32_t dim, cell _cell );


int set_simple_cell_value_at_index_(void* ptr, size_t idx, double value);
int get_cube_value_at_index_(void  *ptr, size_t idx, cell_val* value);
*/
int set_simple_cell_value_at_index_(void *ptr, size_t idx, double value);
int get_cube_value_at_index_(void *ptr, size_t idx, cell_val* value);
int compute_index(cube *_cube,  cell *_cell );

#endif
