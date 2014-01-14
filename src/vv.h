#ifndef __VV_H
#define __VV_H

#include <stdint.h> // uint32_t
#include <stddef.h> // size_t

#define CUBE_DATA_END "_data"

// Number of bytes per cube cell
#define CELL_BYTES sizeof(cell_val)
typedef struct {
	uint8_t	flags;
	double  val; // 4 bites
} cell_val;

typedef struct{
	uint32_t	*nr_dim; // same as nr of dim in cube
	size_t		*idx;    // cell index in a flat structure
	size_t		*idxs;   // an array of di index in each dimension
} cell;
#define cellStructSize(nr_dim) ( sizeof(uint32_t) + sizeof(size_t) + nr_dim * sizeof(size_t) )
#define initCell(_cell,_ptr) do { \
    _cell.nr_dim = (uint32_t *)_ptr; \
    _cell.idx  = (size_t *)((uint32_t *)_ptr + 1); \
    _cell.idxs = _cell.idx + 1; \
} while(0);
#define setCellNrDims(_cell,_nr_dims) do { \
		*(_cell.nr_dim ) = (uint32_t)_nr_dims; \
} while(0);
#define setCellIdx(_cell,_dim, _idx) do { \
		*(_cell.idxs + _dim ) = _idx; \
} while(0);
#define setCellFlatIdx(_cell, _idx) do { \
		*(_cell.idx) = _idx; \
} while(0);

#define getCellDiIndex(_cell, _dim) (*(_cell.idxs + _dim ))

typedef struct {
	uint32_t *nr_dim;// size = 1
	uint32_t *nr_di; // size = nr_dim. nr of di in each dim
} cube;
#define cubeStructSize(nr_dim) sizeof(uint32_t)*(nr_dim + 1)
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
	uint32_t *dim_index_in_cube; // 1 element
	uint32_t *nr_di;  // 1 element. Nr of id in this dimension
	uint32_t *di_order; // nr_di elements. level. 0 = lowest level

	//*type		// I assume that order == 0 -> type == simple
} dim;
#define initDim(_dim,_ptr) do { \
    _dim.nr_di    = (uint32_t *)_ptr; \
    _dim.di_order = (uint32_t *)((uint32_t *)_ptr + 1); \
} while(0);

#define dimStructSize(nr_di) ( sizeof(uint32_t) + nr_di * sizeof(uint32_t) )
#define setDimNrDi(_dim,_nr_di) do { \
		*(_dim.nr_di ) = (uint32_t)_nr_di; \
} while(0);
#define setDimDiOrder(_dim,_nr_di,_di_order) do { \
		*(_dim.di_order + _nr_di )= (uint32_t)_di_order; \
} while(0);
#define getDimNrDi(_dim)  (*(_dim.nr_di))
#define getDimDiOrder(_dim, _di_idx)  (*(_dim.di_order + _di_idx))


typedef struct {
	uint32_t *nr_child;
	uint32_t *child;
	uint32_t *nr_parent;
	uint32_t *parent;
	uint32_t *nr_char_formula;
	char* formula;
} di;

#endif
