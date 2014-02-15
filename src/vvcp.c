#include "redis.h"
#include "vvi.h"

#include "vv.h"
#include "vvfct.h"
#include "vvformula.h"
#include "vvdb.h"


di_children* diBuild(redisClient *c, int cube, int dim, int di){
	sds s = sdsempty();
	s = sdscatprintf(s,"%d_%d_%d_children", cube, dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(c->db, so);
	//redisLog(REDIS_WARNING, "Data address :%s", s);

	decrRefCount(so);
	if (redis_data == NULL ){
		// This is possible -> no children. Just a convention, to reduce number of keys in db.
		//redisLog(REDIS_WARNING,"Invalid key :%s", s);
		return NULL;
	}

	di_children* res = (di_children*)sdsnewlen(NULL, sizeof(di_children));
	initDiChildren(res, redis_data->ptr);
	return res;
}

void diRelease(di_children* di){
	sdsfree( (sds)di);
}
//int diIsSimple(di_children* di){
//	if ( 0 == *di->nr_child )
//		return 1;
//	else
//		return 0;
//}
int cellSetValueDownward(redisClient *c, cube* _cube, cell* _cell, long double _val
		, vvdb*  _vvdb
		, slice* _slice
		, int curr_dim  // Algorithm parameters
		, long* nr_writes // How many result has been written to client
		){
	sliceAddCell(c,_cube, _slice, _cell); // FIXME: It is ok here ?

	int di_idx = getCellDiIndex(_cell, curr_dim);
	if ( ( *_cube->nr_dim - 1 )!= curr_dim ) {
		//redisLog(REDIS_WARNING, "Is Simple: move to :%d", curr_dim + 1);
		int next_dim = curr_dim + 1;
		cellSetValueDownward(c, _cube, _cell, _val, _vvdb
				,_slice
				, next_dim, nr_writes);
	} else {
		redisLog(REDIS_WARNING, "Set value at index :%zu value: %.2f", _cell->idx, (double)_val);
		set_value_with_response(c, _vvdb, _cell, _val, nr_writes);
		sliceAddCell(c,_cube, _slice, _cell);
	}
	di_children *di = diBuild(c, (int)*_cube->numeric_code, curr_dim, di_idx);

	//redisLog(REDIS_WARNING, "Is simple :%d on dim: %d/%d", diIsSimple(di), curr_dim, *_cube->nr_dim -1 );
	//if ( ! diIsSimple(di) ) {
	if ( NULL != di ){
		//redisLog(REDIS_WARNING, "Set value at index :%zu value: %.2f", _cell->idx, _cell_val->val);
		long double new_val;
		new_val = _val / *di->nr_child;
		for(int i=0; i< *di->nr_child;++i){
			uint32_t new_di_idx = getDiChildrenChildId(di,i);
			uint32_t old_di_idx = getCellDiIndex(_cell, curr_dim);
			// Instead of create a new cell, reuse the old one
			setCellIdx(_cell,curr_dim, new_di_idx);
			//redisLog(REDIS_WARNING, "New index on dim :%d is :%d child nr:%d", curr_dim, new_di_idx,i);
			if ( REDIS_OK != compute_index(_cube , _cell) ) {
				redisLog(REDIS_WARNING,"ABORT: Fail to compute  flat index for the new cell");
				return REDIS_ERR;
			}
			//Go one level downward
			cellSetValueDownward(c, _cube, _cell, new_val, _vvdb
						, _slice
						, curr_dim, nr_writes);
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
	slice *res = (slice*)res_space; initSlice(res, res_space);
	res->nr_dim = nr_dim;
	//redisLog(REDIS_WARNING,"Done Slice. Nr of dimensions: %d address :%p", res->nr_dim , (void*)res );
	for(uint32_t i=0; i< nr_dim; ++i) {
		uint32_t nr_di = getCubeNrDi(_cube, i);
		sds dim_space = sdsnewlen(NULL,getElementsSize(nr_di));
		elements* el=(elements*)dim_space; initElements(el, dim_space);
		el->nr_elem = nr_di;
		el->max_level = 0; // Will be recomputed before algorithm run.
		for(uint32_t j=0; j< nr_di; ++j) {
			setElementsElement(el, j, ELEMENT_DEF_LEVEL);
		}
		//redisLog(REDIS_WARNING,"Done Element. Position : %d element address : %p", i,(void*) el);
		setSliceElement(res, i, el);
	}

//	/// Test READ
//	for(uint32_t i =0; i < res->nr_dim; ++i){
//		elements* el = getSliceElement(res, i);
//		redisLog(REDIS_WARNING,"Dim:%d level: %d address: %p", (int)i, (int)getElementsElement(el, i) , (void*)el);
//	}


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
	int res = REDIS_ERR;
	int32_t at_level = _el->curr_level;
	for(int32_t i=0; i < _el->nr_elem; ++i){
		int32_t level = getElementsElement(_el, i);
		if ( level == at_level ) {
			_el->curr_elem = i;
			return REDIS_OK; //Take the first element at this level
		}
	}
	return res;
}

int sliceResetElementsLevel(slice *_slice, uint32_t up_to) {
	for(uint32_t i =0; i< up_to; ++i){
		elements* el = getSliceElement(_slice, i);
		el->curr_level = 0;
	}
	return REDIS_OK;
}

int sliceResetElementsLevelFull(slice *_slice ) {
	uint32_t up_to = _slice->nr_dim;
	for(uint32_t i =0; i< up_to; ++i){
		int32_t min_level = INT32_MAX;
		int32_t max_level = INT32_MIN;
		elements* el = getSliceElement(_slice, i);
		for(int32_t j=0; j<el->nr_elem;++j){
			int32_t level = getElementsElement(el, j);
			if ( -1 != level ) { // if is valid
				if ( level < min_level )
					min_level = level;
				if ( level > max_level )
					max_level = level;
			}
		}
		redisLog(REDIS_WARNING,"Dimension : %d min lvl:%d max lvl: %d", i, min_level, max_level );

		if (INT32_MAX == min_level || INT32_MIN ==  max_level){
			redisLog(REDIS_WARNING,"All levels on dimension : %d are uninitialized", i );
			return REDIS_ERR;
		}
		if ( 0 != min_level ) {
			redisLog(REDIS_WARNING,"ERROR: For dimension : %d i have minimum level != 0 (%d)", i, min_level );
		}
		el->curr_level = min_level;
		el->max_level = max_level;
	}
	return REDIS_OK;
}

int sliceResetElementsCurrElement(slice *_slice, uint32_t up_to) {
	for(uint32_t i =0; i< up_to; ++i){
		elements* el = getSliceElement(_slice, i);
		if ( REDIS_OK != elementResetCurrentPosition(el) )
			return REDIS_ERR;
	}
	return REDIS_OK;
}

int32_t getLevel( redisClient *c, cube *_cube,  uint32_t dim_idx, size_t di_idx){

	sds s = sdsempty();
	s = sdscatprintf(s,"%d_%d_%d_level", (int)*_cube->numeric_code, (int)dim_idx, (int)di_idx);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(c->db, so);
	//redisLog(REDIS_WARNING, "Data address :%s", s);

	decrRefCount(so);
	if (redis_data == NULL ){
		redisLog(REDIS_WARNING, "Invalid key : %s", s);
		return -1;
	}
	return *(int32_t*)redis_data->ptr;
}
int sliceAddCell(redisClient *c,cube* _cube, slice* _slice, cell *_cell){
	//redisLog(REDIS_WARNING,"Slice address :%p", (void*)_slice);
	//redisLog(REDIS_WARNING,"Slice number of dimensions:%d", _slice->nr_dim);

	for(uint32_t i =0; i<_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		size_t di_idx = getCellDiIndex(_cell, i);
		int32_t level = getLevel(c, _cube, i, di_idx);
		if ( level != -1 ) {
			setElementsElement(el, di_idx ,level);
		}
		//redisLog(REDIS_WARNING,"Set at dim:%d dim index : %d at element address: %p level:%d", (int)i, (int)di_idx , (void*)el, level);
	}

	return REDIS_OK;
}
cell* cellBuildEmpty(cube* _cube ){
	sds space = sdsempty();
	space = sdsMakeRoomFor( space, cellStructSizeDin( *_cube->nr_dim ) );
	cell *_cell = (cell*) space;
	initCellDin(_cell , space); setCellNrDims(_cell,*_cube->nr_dim);
	return _cell;
}
int cellRecompute(redisDb *_db,cube *_cube,cell* _cell){
	static int nr_tot = 0;
	sds s = sdsempty();
	for(int i=0; i < _cell->nr_dim; ++i ){
		s = sdscatprintf(s,"%d ", (int)getCellDiIndex(_cell, i) );
	}

	sdsfree(s);
	//get formula
	//execute it
	formula *f = formulaNew(
			_db,
			 //void *_cube, _cell, int _dim_idx, , const char* _program
			_cube, _cell, 0, "10 + 9"
			);
	double val = f->eval(f, _cell);
	//f->free(f);
	redisLog(REDIS_WARNING, " Number of cycles:%d cell idx:%s, value:%f ", ++nr_tot, s, val );
	return REDIS_OK;
}
// Return the index at the same level
int elementNextCurrElement(elements* _el){
	int32_t curr_level = _el->curr_level;
	// Search for something at same level
	for( int32_t i = _el->curr_elem + 1; i< _el->nr_elem; ++i){
		int32_t level = getElementsElement(_el, i);
		if  ( level == curr_level ) {
			_el->curr_elem = i;
			return REDIS_OK;
		}
	}
	return REDIS_ERR;
}
int sliceRecomputeLevel(redisDb *_db,cube *_cube,slice *_slice){

	static int nr_tot_level = 0;

	/*//Trace
	sds s = sdsempty();
	for(int i=0; i < _slice->nr_dim; ++i ){
    	elements* el = getSliceElement(_slice, i);
		s = sdscatprintf(s,"%d ", el->curr_level );
	}
	redisLog(REDIS_WARNING, " Number of cycles:%d Level:%s", ++nr_tot_level, s );
	sdsfree(s);
	 */
	cell *_cell = cellBuildEmpty(_cube );
	if ( REDIS_ERR == sliceResetElementsCurrElement(_slice, _slice->nr_dim) ){
		// No valid combination for this levels combination
		return REDIS_OK;
	}
	while(1){

		// Build initial cell based on element.curr_element
		for( uint32_t i =0; i <_slice->nr_dim; ++i){
			elements* el = getSliceElement(_slice, i);
			setCellIdx(_cell, i, el->curr_elem);
		}
		if ( REDIS_OK != compute_index(_cube , _cell) ) {
			return REDIS_ERR;
		}
		// Recompute cell
		cellRecompute(_db, _cube, _cell);
	   // Made one change in slice.elements ( advance one index )
		for(uint32_t curr_dim = 0; ; ) {
			elements* el = getSliceElement(_slice, curr_dim);
			if( REDIS_OK == elementNextCurrElement(el) ) {
				// Now I have a new cell address as curr_elem in slice
				break;
			} else {
				// if last dimension ...
				if( (_slice->nr_dim - 1) == curr_dim ) {
					//I have done all the possible combination. Exit
					cellRelease(_cell );
					return REDIS_OK;
				} else {
					elementResetCurrentPosition(el);
					++curr_dim;
				}
			}
		}
	}

	return REDIS_OK;
}
int sliceGetOverallMaxim(slice *_slice ){
	int slice_max_level = -1;
	for(uint32_t i =0; i< _slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		if ( slice_max_level < el->max_level )
			slice_max_level = el->max_level;
	}
	return slice_max_level;
}
int sliceSetValueUpward(redisDb *_db, cube *_cube,slice *_slice) {

	//Init Slice levels ( current and maxim )
	if ( REDIS_ERR == sliceResetElementsLevelFull(_slice) ){
		return REDIS_ERR;
	}
	// loop invariants
	int inital_run = 0;//!!<
	uint32_t curr_dim = 0;
	int slice_level  = 0;
	int slice_max_level = sliceGetOverallMaxim(_slice );
	//redisLog(REDIS_WARNING,"Overall slice max value : %d", slice_max_level);
	while(1){
		if ( !inital_run ) { // At first run all levels are 0, so no need recompute
		   sliceRecomputeLevel(_db, _cube,_slice);
		   inital_run = 0;
		}
        // Made one change in slice.elements ( advance one index )
        while (1) {
        	elements* el = getSliceElement(_slice, curr_dim);
        	//if( REDIS_OK == elementNextCurrElement(el, slice_level) ) {
        	if(    el->curr_level < el->max_level ){
        		if ( el->curr_level < slice_level ) {
            		// Now I have a new cell address as curr_elem in slice
					++el->curr_level;
					sliceResetElementsLevel(_slice, curr_dim);
					curr_dim = 0;
					break;
        		} else {
        			if ( curr_dim == _slice->nr_dim  - 1) {
        				++slice_level;
        				curr_dim = 0;
        			} else {
						++curr_dim;
        			}
        		}
        	} else {
        		if( curr_dim == _slice->nr_dim - 1) {
        			if ( slice_level < slice_max_level ) {
        				++slice_level;
        				curr_dim = 0;
        			} else {
        				// SUCCESS !! - Normal exit
        				return REDIS_OK;
        			}
        		} else {
        			++curr_dim;
        		}
        	}
        }
	}
	redisLog(REDIS_WARNING,"sliceSetValueUpward : This is not a valid output... Something gone wrong...");
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

        // Made one change in slice.elements ( advance one index )
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

