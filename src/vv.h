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
#define cellStructSizeDin(nr_dim) ( sizeof(cell) + nr_dim * sizeof(size_t) )
#define initCellDin(_cell,_ptr) do { \
    _cell->idxs  = (size_t *)( (char*)_ptr + sizeof(cell) ); \
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
	uint32_t *numeric_code;// an numerical identifier for cube
	uint32_t *nr_dim;//
	uint32_t *nr_di; // size = nr_dim. nr of di in each dim
} cube;

#define getCubeSize(nr_elem)  ( (nr_elem + 2 ) * sizeof(uint32_t) )
#define initCube(_str,_ptr) do { \
	(_str)->numeric_code 	= (uint32_t *)_ptr; \
	(_str)->nr_dim 		= (uint32_t *)((uint32_t *)_ptr + 1); \
	(_str)->nr_di 		= (uint32_t *)((uint32_t *)_ptr + 2); \
} while(0);

#define setCubeNrDims(_str,_nr_dims) do { \
		*((_str)->nr_dim ) = (uint32_t)_nr_dims; \
} while(0);
#define setCubeNumericCode(_str,_numeric_code) do { \
		*((_str)->numeric_code ) = (uint32_t)_numeric_code; \
} while(0);
#define setCubeNrDi(_str,_dim_idx,_nr_di) do { \
		*((_str)->nr_di + _dim_idx )= (uint32_t)_nr_di; \
} while(0);
#define getCubeNrDi(_str,_dim_idx)  (*(_str->nr_di + _dim_idx ))

typedef struct {
	uint32_t *nr_child;// nr of children
	uint32_t *child_id; // array of dimension item indexes
} di_children;

#define initDiChildren(_str,_ptr) do { \
    _str->nr_child = (uint32_t *)_ptr; \
    _str->child_id = (uint32_t *)((uint32_t *)_ptr + 1); \
} while(0);

#define setDiChildrenNrChildren(_str,nr_child) do { \
		*(_str->nr_child ) = (uint32_t)nr_child; \
} while(0);
#define setDiChildrenChildId(_str,_idx,_di) do { \
		*(_str->child_id + _idx )= (uint32_t)_di; \
} while(0);
#define getDiChildrenChildId(_str,_idx)  (*(_str->child_id + _idx ))

// Negative values = error/un-initialized
typedef struct {
	int32_t	nr_elem;
	int32_t	curr_elem;  // An index < nr_elem
	int32_t	curr_level; // at what level I am
	int32_t max_level;  // Highest level on this dimension
	int32_t	*elems;
} elements;
#define getElementsSize(nr_elem)  (sizeof(elements) + nr_elem * sizeof(int32_t) )
#define initElements(_el,_ptr) do { \
    _el->elems = (int32_t *) ( (char*)_ptr + sizeof(elements)  ); \
} while(0);
#define setElementsElement(_el,_idx,_elem) do { \
		*(_el->elems + _idx ) = (int32_t)_elem; \
} while(0);
#define getElementsElement(_el,_idx)  *(_el->elems + _idx )

typedef struct {
	uint32_t		nr_dim;
	elements**		ptr; // array of pointer elements
} slice;  // represent a cube slice ( selection ). If on each dim I have one item -> cell

#define getSliceSize(nr_dim)  ( sizeof(slice) +  nr_dim * sizeof(elements*) )

#define initSlice(_sl,_ptr) do { \
		_sl->ptr = (elements**) ( (char*)_ptr +  sizeof(slice)  ); \
} while(0);

#define setSliceElement(_sl,_idx,_elem) do { \
		*(_sl->ptr + _idx )= (elements*)_elem; \
} while(0);
#define getSliceElement(_sl,_idx)  (elements*)(*(_sl->ptr + _idx ))


#endif
