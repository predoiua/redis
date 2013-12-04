#ifndef __VV_H
#define __VV_H

#include <stdint.h> // uint32_t
#include <stddef.h> // size_t

#define CUBE_DIM_END "_dim"
#define CUBE_DATA_END "_data"

// Number of bytes per cube cell
#define CELL_BYTES 4
typedef double cell_val;

typedef struct {
	uint32_t *nr_dim;// size = 1
	uint32_t *nr_di; // size = nr_dim. nr of di in each dim
} cube;
#define initCube(_cube,_ptr) do { \
    _cube.nr_dim = (uint32_t *)_ptr; \
    _cube.nr_di = (uint32_t *)((uint32_t *)_ptr + 1); \
} while(0);
#define setCubeNrDims(_cube,_nr_dims) do { \
		*(_cube.nr_dim ) = (uint32_t)_nr_dims; \
} while(0);
#define setCubeNrDi(_cube,_nr_dim,_nr_di) do { \
		*(_cube.nr_di + _nr_dim )= (uint32_t)_nr_di; \
} while(0);
#define getCubeNrDi(_cube,_nr_dim)  (*(_cube.nr_di + _nr_dim ))
#define getCubeNrDim(_cube)  (*(_cube.nr_dim ))

typedef struct {
	uint32_t *nr_di;
	uint32_t *di_order; // level. 0 = lowest level
} dim;

typedef struct {
	uint32_t *nr_child;
	uint32_t *child;
	uint32_t *nr_parent;
	uint32_t *parent;
	uint32_t *nr_char_formula;
	char* forumula;
} di;

#endif
