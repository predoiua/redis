#ifndef __VV_H
#define __VV_H

#include <stdint.h> // uint32_t
#include <stddef.h> // size_t

/*
 * Basic structure related with cube operation.
 * Can be included in any file.
 */
#define CUBE_DATA_END "_data"

// Number of bytes per cube cell
#define CELL_BYTES sizeof(cell_val)
typedef struct {
	uint8_t	flags;
	double  val; // 4 bites
} cell_val;

typedef struct{
	uint32_t	nr_dim; // same as nr of dim in cube
	size_t		idx;    // cell index in a flat structure
	size_t		*idxs;   // an array of di index in each dimension
} cell;
#define cellStructSize(nr_dim) ( nr_dim * sizeof(size_t) )
#define initCell(_cell,_ptr) do { \
    _cell->idxs  = (size_t *)_ptr; \
} while(0);
#define setCellNrDims(_cell,_nr_dims) do { \
		_cell->nr_dim = _nr_dims; \
} while(0);
#define setCellIdx(_cell,_dim, _idx) do { \
		*(_cell->idxs + _dim ) = _idx; \
} while(0);

#define getCellDiIndex(_cell, _dim) (*(_cell->idxs + _dim ))

typedef struct {
	void *ptr;
} cube_data;


typedef struct {
	uint32_t *nr_dim;// size = 1
	uint32_t *nr_di; // size = nr_dim. nr of di in each dim
} cube;

#define initCube(_cube,_ptr) do { \
    _cube->nr_dim = (uint32_t *)_ptr; \
    _cube->nr_di = (uint32_t *)((uint32_t *)_ptr + 1); \
} while(0);

#define setCubeNrDims(_cube,_nr_dims) do { \
		*(_cube->nr_dim ) = (uint32_t)_nr_dims; \
} while(0);
#define setCubeNrDi(_cube,_dim_idx,_nr_di) do { \
		*(_cube->nr_di + _dim_idx )= (uint32_t)_nr_di; \
} while(0);
#define getCubeNrDi(_cube,_dim_idx)  (*(_cube->nr_di + _dim_idx ))



#endif
