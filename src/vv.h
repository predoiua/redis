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

// Negative values = error/un-initialized
typedef struct {
	int32_t	nr_elem;
	int32_t	curr_elem;  // An index < nr_elem
	int32_t	curr_level; // at what level I should be
	int32_t	*elems;
} elements;
#define getElementsSize(nr_elem)  (sizeof(elements) + nr_elem * sizeof(int32_t) )
#define getElementsCurrLevel(_el)  _el->curr_level;
#define initElements(_el,_ptr) do { \
    _el->elems = (int32_t *) ( (char*)_ptr + sizeof(elements)  ); \
} while(0);

#define setElementsNrElem(_el,_nr_elem) do { \
		_el->nr_elem = (int32_t)_nr_elem; \
} while(0);

#define setElementCurrElement(_el, elem ) do { \
		_el->curr_elem = elem; \
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
