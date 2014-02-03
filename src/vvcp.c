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
int cellSetValueDownward(redisClient *c, cube* _cube, cell* _cell, cell_val* _cell_val
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
		cellSetValueDownward(c, _cube, _cell, _cell_val, cube_data, next_dim, nr_writes);
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
			cellSetValueDownward(c, _cube, _cell, &new_val, cube_data, curr_dim, nr_writes);
			//revert to old index
			setCellIdx(_cell,curr_dim, old_di_idx);
		}
	}
	diRelease(di);

	return REDIS_OK;
}



slice* sliceBuild(cube *_cube){
	static uint32_t ELEMENT_DEF_LEVEL = -1;
	uint32_t nr_dim = *_cube->nr_dim;
	sds res_space =   sdsnewlen(NULL, getSliceSize(nr_dim));
	slice* res = (slice*)res_space; initSlice(res, res_space);
	res->nr_dim = nr_dim;
	//redisLog(REDIS_WARNING,"Done Slice. Nr of dimensions: %d", res->nr_dim );
	for(uint32_t i=0; i< nr_dim; ++i) {
		uint32_t nr_di = getCubeNrDi(_cube, i);
		sds dim_space = sdsnewlen(NULL,getElementsSize(nr_di));
		elements* el=(elements*)dim_space; initElements(el, dim_space);
		setElementsNrElem(el,nr_di);
		for(uint32_t j=0; j< nr_di; ++j) {
			setElementsElement(el, j, ELEMENT_DEF_LEVEL);
		}
		//redisLog(REDIS_WARNING,"Done Element. Position : %d element address : %p", i,(void*) el);
		setSliceElement(res, i, el);
	}
	return res;
}
int sliceRelease(slice* _slice){
	uint32_t nr_dim = _slice->nr_dim;
	for(uint32_t i=0; i< nr_dim; ++i) {
		elements* el=getSliceElement(_slice,i);
		sdsfree((sds)el);
		//redisLog(REDIS_WARNING,"Done free element. Position : %d", i );
		el = NULL;
	}
	sdsfree((sds)_slice);
	_slice = NULL;
	//redisLog(REDIS_WARNING,"Done free slice." );

	return REDIS_OK;
}
// First index with them minimum level
int elementResetCurrentPosition(elements* _el) {
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

int sliceResetElementsCurrentPosition(slice *_slice ) {
	for(uint32_t i =0; i<_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		if ( REDIS_OK != elementResetCurrentPosition(el) )
			return REDIS_ERR;
	}
	return REDIS_OK;
}

int sliceAddCell(slice* _slice, cell *_cell){
	for(uint32_t i =0; i<_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		size_t di_idx = getCellDiIndex(_cell, i);
		int32_t level = 1; // FIXME. Get the real level
		//redisLog(REDIS_WARNING,"Set at dim:%d dim index : %d at element address: %p", (int)i, (int)di_idx , (void*)el);
		setElementsElement(el, di_idx ,level)
	}

	return REDIS_OK;
}
int cellSetValueUpward(cube *_cube, cell *_cell) {
	slice* _slice = sliceBuild(_cube);
	//redisLog(REDIS_WARNING,"Done build slice");
	sliceAddCell(_slice, _cell);
	//redisLog(REDIS_WARNING,"Done add cell");
	int res;
	res = sliceSetValueUpward(_cube, _slice);
	//redisLog(REDIS_WARNING,"Done sliceSetValueUpward");
	sliceRelease(_slice);
	return res;
}
cell* cellBuildEmpty(cube* _cube ){
	sds space = sdsempty();
	space = sdsMakeRoomFor( space, cellStructSizeDin( *_cube->nr_dim ) );
	cell *_cell = (cell*) space;
	initCellDin(_cell , space); setCellNrDims(_cell,*_cube->nr_dim);
	return _cell;
}
int cellRecompute(cube *_cube,cell* _cell){
	//get formula
	//execute it
	return REDIS_OK;
}
int sliceSetValueUpward(cube *_cube,slice *_slice) {
	//Init Slice
	sliceResetElementsCurrentPosition(_slice);
	cell *_cell = cellBuildEmpty(_cube );
	while(1){
		for( uint32_t i =0; i <_slice->nr_dim; ++i){
			elements* el = getSliceElement(_slice, i);
			setCellIdx(_cell, i, el->curr_elem);
		}
		if ( REDIS_OK != compute_index(_cube , _cell) ) {
			return REDIS_ERR;
		}
		cellRecompute(_cube, _cell);
		int slice_level  = 0;

		cellRelease(_cell ); // FIXME: Move before the real exit
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

