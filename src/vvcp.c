#include "redis.h"
#include "vvi.h"

#include "vv.h"
#include "vvfct.h"

cube* diBuild(redisClient *c, sds cube_code, int dim, int di){
	sds s = sdsnew("children_");
	s = sdscatsds(s, cube_code);
	s = sdscatprintf(s,"_%d_%d",  dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(c->db, so);
	//redisLog(REDIS_WARNING, "Data address :%s", s);

	decrRefCount(so);
	if (redis_data == NULL ){
		addReplyErrorFormat(c, "Invalid key code for di: cube_code:?: dim :%d di:%d", dim, di);
		return NULL;
	}

	cube* res = (cube*)sdsnewlen(NULL, sizeof(cube));
	initCube(res, redis_data->ptr);
	return res;
}

void diRelease(cube* di){
	sdsfree( (sds)di);
}
int diIsSimple(cube* di){
	if ( 0 == *di->nr_dim )
		return 1;
	else
		return 0;
}
int setValueDownward(redisClient *c, cube* _cube, cell* _cell, cell_val* _cell_val
		, cube_data* cube_data
		, int curr_dim  // Algorithm parameters
		, long* nr_writes // How many result has been written to client
		){
	int di_idx = getCellDiIndex(_cell, curr_dim);
	cube *di = diBuild(c, c->argv[1]->ptr, curr_dim, di_idx);
	if ( NULL == di ){
		return REDIS_ERR;
	}
	if ( ( *_cube->nr_dim - 1 )!= curr_dim ) {
		//redisLog(REDIS_WARNING, "Is Simple: move to :%d", curr_dim + 1);
		int next_dim = curr_dim + 1;
		setValueDownward(c, _cube, _cell, _cell_val, cube_data, next_dim, nr_writes);
	} else {
		redisLog(REDIS_WARNING, "Set value at index :%zu value: %.2f", _cell->idx, _cell_val->val);
		set_value_with_response(c, cube_data->ptr, _cell, _cell_val, nr_writes);
	}

	//redisLog(REDIS_WARNING, "Is simple :%d on dim: %d/%d", diIsSimple(di), curr_dim, *_cube->nr_dim -1 );
	if ( ! diIsSimple(di) ) {
		//redisLog(REDIS_WARNING, "Set value at index :%zu value: %.2f", _cell->idx, _cell_val->val);
		//set_value_with_response(c, cube_data->ptr, _cell, _cell_val, nr_writes);
		cell_val new_val;
		new_val.val = _cell_val->val / *di->nr_dim;
		for(int i=0; i< *di->nr_dim;++i){
			uint32_t new_di_idx = getCubeNrDi(di,i);
			uint32_t old_di_idx = getCellDiIndex(_cell, curr_dim);
			// Instead of create a new cell, reuse the old one
			setCellIdx(_cell,curr_dim, new_di_idx);
			//redisLog(REDIS_WARNING, "New index on dim :%d is :%d child nr:%d", curr_dim, new_di_idx,i);
			if ( REDIS_OK != compute_index(_cube , _cell) ) {
				redisLog(REDIS_WARNING,"ABORT: Fail to compute  flat index for the new cell");
				return REDIS_ERR;
			}
			//Go one level downward
			setValueDownward(c, _cube, _cell, &new_val, cube_data, curr_dim, nr_writes);
			//revert to old index
			setCellIdx(_cell,curr_dim, old_di_idx);
		}
	}
	diRelease(di);

	return REDIS_OK;
}

// Negative values = error/un-initialized
typedef struct {
	int32_t	nr_elem;
	int32_t	curr_elem; // An index < nr_elem
	int32_t	*elems;
} elements;
#define getElementsSize(nr_elem)  ( 2 * sizeof(int32_t) + nr_elem * sizeof(int32_t*) )

#define initElements(_el,_ptr) do { \
    _el->elems = (int32_t *) ( (char*)_ptr + 2 * sizeof(int32_t)  ); \
} while(0);

#define setElementsNrElem(_el,_nr_elem) do { \
		*(_el->nr_elem ) = (int32_t)_nr_elem; \
} while(0);
#define setElementsElement(_el,_idx,_elem) do { \
		*(_el->elem + _idx )= (int32_t)_elem; \
} while(0);
#define getElementsElement(_el,_idx)  (*(_el->elems + _idx ))

typedef struct {
	uint32_t		nr_dim;
	elements**		ptr; // array of pointer elements
} slice;  // represent a cube slice ( selection ). If on each dim I have one item -> cell

#define getSliceSize(nr_dim)  ( sizeof(uint32_t) + nr_dim * sizeof(elements*) )

#define initSlice(_sl,_ptr) do { \
    _sl->ptr = (elements**) ( (char*)_ptr +  sizeof(int32_t)  ); \
} while(0);
#define setSliceElement(_sl,_idx,_elem) do { \
		*(_sl->ptr + _idx )= (elements*)_elem; \
} while(0);
#define getSliceElement(_slice,_idx)  (elements*)(_slice->ptr + _idx )

slice* buildSlice(cube *_cube){
	uint32_t nr_dim = *_cube->nr_dim;
	sds res_space =   sdsnewlen(NULL, getSliceSize(nr_dim));
	slice* res = (slice*)res_space; initSlice(res, res_space);
	res->nr_dim = nr_dim;
	for(uint32_t i=0; i< nr_dim; ++i) {
		uint32_t nr_di = getCubeNrDi(_cube, i);
		sds dim_space = sdsnewlen(NULL,getElementsSize(nr_di));
		elements* el=(elements*)dim_space; initElements(el, dim_space);
		setSliceElement(res, i, el);
	}
	return res;
}
int releaseSlice(slice* _slice){
	uint32_t nr_dim = _slice->nr_dim;
	for(uint32_t i=0; i< nr_dim; ++i) {
		elements* el=getSliceElement(_slice,i);
		sdsfree((sds)el);
		el = NULL;
	}
	sdsfree((sds)_slice);
	_slice = NULL;
	return REDIS_OK;
}
// First index with them minimum level
int resetAtInitialPosition(elements* _el) {
	int32_t curr_level = INT32_MAX;
	int res = REDIS_ERR;
	for(int32_t i=0; i < _el->nr_elem; ++i){
		int32_t level = getElementsElement(_el, i);
		if ( level > 0 ) {
			if ( level < curr_level ) {
				curr_level = level;
				_el->curr_elem = i;
				res = REDIS_OK;
			}
		}
	}
	return res;
}

int resetSliceElemnts(slice *_slice ) {
	for(uint32_t i =0; i<_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		if ( REDIS_OK != resetAtInitialPosition(el) )
			return REDIS_ERR;
	}
	return REDIS_OK;
}
int setValueUpward(slice *_slice) {
	//Init Slice
	resetSliceElemnts(_slice);
	while(1){
		return REDIS_OK;
	}
	return REDIS_ERR;
}
/*
   slice* _slice;
  init elements.current_idx;
  while(1) {
        // Compute the cell, and recompute the value
        cell _cell;

        for curr_dim = 0 .. cube.nr_dim
            element = slice.get_element( curr_dim )
            cell.i = element.curr_elem
        get formula ( _cell )
        execute formula

        slice_level = 0;

        // Made one chance in slice.elements ( advance one index )
        for(int curr_dim = 0; ; ) {
            element = slice.get_element( curr_dim )
            //elem_curr_level = element.get_curr_level();
            new_curr_elmen = element.get_next_elem( slice_level )

            if( new_curr_elmen is ok ) {
                // Now I have a new cell address
                element.curr_elem = new_curr_element;
                break;
            } else {
                if( curr_dim = last_dimension) {
                    if ( slice_level < cube level ) {
                        curr_dim = 0;
                        ++slice_level;
                    } else {
                        //I have done all the possible combination. Exit
                        return OK;
                     }
                } else {
                    element at curr_dim = first_item;
                    ++curr_dim
                }
            }
        }
    }

 */

