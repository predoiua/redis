#include "redis.h"

#include "vv.h"
#include "vvfct.h"
#include "vvformula.h"
#include "vvdb.h"
#include "vvlevelcollector.h"


void diRelease(di_children* di){
	sdsfree( (sds)di);
}

int cellSetValueDownward(vvcc *_vvcc, cube* _cube, cell* _cell, long double _val
		, vvdb*  _vvdb
		, slice* _slice
		, int curr_dim  // Algorithm parameters
		){
	//sliceAddCell(_vvdb,_cube, _slice, _cell); // FIXME: It is ok here ?

	int di_idx = getCellDiIndex(_cell, curr_dim);
	if ( ( *_cube->nr_dim - 1 )!= curr_dim ) {
		//redisLog(REDIS_WARNING, "Is Simple: move to :%d", curr_dim + 1);
		int next_dim = curr_dim + 1;
		cellSetValueDownward(_vvcc, _cube, _cell, _val, _vvdb
				,_slice
				, next_dim);
	} else {
		//redisLog(REDIS_WARNING, "Set value : %.2f", (double)_val);
		_vvcc->setValueWithResponse(_vvcc, _cell, _val);
		sliceAddCell(_vvdb,_cube, _slice, _cell);
	}
	di_children *di = _vvdb->getDiChildren (_vvdb, curr_dim, di_idx);

	if ( NULL != di ){
		//redisLog(REDIS_WARNING, "Set value at index :%zu value: %.2f", _cell->idx, _cell_val->val);
		long double new_val;
		new_val = _val / *di->nr_child;
		for(int i=0; i< *di->nr_child;++i){
			uint32_t new_di_idx = getDiChildrenChildId(di,i);
			uint32_t old_di_idx = getCellDiIndex(_cell, curr_dim);
			// Instead of create a new cell, reuse the old one
			setCellIdx(_cell,curr_dim, new_di_idx);
			//Go one level downward
			cellSetValueDownward(_vvcc, _cube, _cell, new_val, _vvdb
						, _slice
						, curr_dim);
			//revert to old index
			setCellIdx(_cell,curr_dim, old_di_idx);
		}
	}
	diRelease(di);

	return REDIS_OK;
}



slice* sliceBuild(cube *_cube){
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
			setElementsElement(el, j, -1);
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
	int32_t at_level = _el->curr_level;
	for(int32_t i=0; i < _el->nr_elem; ++i){
		int32_t level = getElementsElement(_el, i);
		if ( level == at_level ) {
			_el->curr_elem = i;
			return REDIS_OK; //Take the first element at this level
		}
	}
	return REDIS_ERR;
}

int sliceResetElementsLevel(slice *_slice, uint32_t up_to) {
	for(uint32_t i =0; i< up_to; ++i){
		elements* el = getSliceElement(_slice, i);
		el->curr_level = el->min_level;
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
		//redisLog(REDIS_WARNING,"Dimension : %d min lvl:%d max lvl: %d", i, min_level, max_level );

		if (INT32_MAX == min_level || INT32_MIN ==  max_level){
			redisLog(REDIS_WARNING,"All levels on dimension : %d are uninitialized", i );
			return REDIS_ERR;
		}
		// It can be ...
		if ( 0 != min_level ) {
			redisLog(REDIS_WARNING,"For dimension : %d i have minimum level != 0 (%d)", i, min_level );
		}
		el->curr_level = min_level;
		el->min_level = min_level;
		el->max_level = max_level;
	}
	return REDIS_OK;
}

int sliceResetElementsCurrElement(slice *_slice) {
	int32_t sum_curr_levels = 0;
	for(uint32_t i =0; i< _slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		sum_curr_levels += el->curr_level;
		if ( REDIS_OK != elementResetCurrentPosition(el) )
			return REDIS_ERR;
	}
	// All are simple elements ( lowest level )
	if ( 0 == sum_curr_levels )
		return REDIS_ERR;
	return REDIS_OK;
}


int sliceAddCell(vvdb *_vvdb, cube* _cube, slice* _slice, cell *_cell){
	//redisLog(REDIS_WARNING,"Slice address :%p", (void*)_slice);
	//redisLog(REDIS_WARNING,"Slice number of dimensions:%d", _slice->nr_dim);

	for(uint32_t i =0; i<_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		size_t di_idx = getCellDiIndex(_cell, i);
		int32_t level = _vvdb->getLevel(_vvdb, i, di_idx);
		if ( level != -1 ) {
			setElementsElement(el, di_idx ,level);
		}
		//redisLog(REDIS_WARNING,"Set at dim:%d dim index : %d at element address: %p level:%d", (int)i, (int)di_idx , (void*)el, level);
	}

	return REDIS_OK;
}
cell* cellBuildEmpty(cube* _cube ){
	sds space = sdsgrowzero( sdsempty(),  cellStructSizeDin( *_cube->nr_dim ) );
	cell *_cell = (cell*) space;
	initCellDin(_cell , space); setCellNrDims(_cell,*_cube->nr_dim);
	return _cell;
}

cell* cellBuildCell(cell* _cell ){
	cell *_cell_new = (cell *)sdsdup ((sds)_cell );
	initCellDin(_cell_new , (void*)_cell_new); // set idxs pointer.

	return _cell_new;
}

// Basic implementation
int getFormulaAndDimSimple(vvdb *_vvdb, cube* _cube, cell* _cell,
		char** formula,
		int*  dimension
		) {
	for(int i=0;i<*_cube->nr_dim;++i){
		size_t di = getCellDiIndex (_cell, i);
//		int32_t lvl = _vvdb->getLevel(_vvdb,i, di);
//		if (0 != lvl)
		*formula = _vvdb->getFormula(_vvdb, i, di );
		if ( NULL != *formula ) {
			*dimension = i;
			return REDIS_OK;
		}
	}
	return REDIS_ERR;
}


int getFormulaAndDim(redisDb *_db,cube *_cube,cell* _cell,
		char** formula,
		int*  dimension
		) {
	int res = REDIS_OK;
	vvdb *_vvdb = vvdbNew(_db,_cube);
	uint32_t measure_di_index = (uint32_t) getCellDiIndex(_cell, *_cube->measure_dim_idx);
	formula_selector fs = _vvdb->getFormulaSelector(_vvdb, measure_di_index);


	for(int i=0;i<nr_formula_selectors(fs);++i){
		formula_selector _fs = next_formula_selector(fs, i);

		int fs_dim		  = getFormulaSelectorDimension(_fs);
		int fs_formula_id = getFormulaSelectorFormulaFormula(_fs);

		redisLog(REDIS_WARNING, "Formula match values dim:%d formula_id:%d target dimension:%d", fs_dim, fs_formula_id, getFormulaSelectorFormulaDimension(_fs)  );

		if ( -2 == fs_formula_id ) {//Non aggregable measure
			res = REDIS_ERR;
			break;
		}

		//uint32_t working_di = (uint32_t) getCellDiIndex(_cell, i);//*_cube->measure_dim_idx);
		if ( -1 == fs_formula_id ){ // All ( Any ) formula

			res = getFormulaAndDimSimple(_vvdb, _cube,  _cell,
					formula,
					dimension);
			break;
		}

		*dimension = getFormulaSelectorFormulaDimension(_fs);
		*formula = _vvdb->getSpecialFormula(_vvdb,  *_cube->measure_dim_idx, measure_di_index, fs_formula_id );
		break;
	}

	_vvdb->free(_vvdb);
	return res;
}

int cellRecompute(vvcc *_vvcc,redisDb *_db,cube *_cube,cell* _cell){

	vvdb *_vvdb = vvdbNew(_db,_cube);
	char* prog = NULL;
	int   dim = -1;
	if ( REDIS_OK != getFormulaAndDim( _db,_cube,_cell, &prog, &dim) ){
		//redisLog(REDIS_WARNING, "Fail to find the appropriate formula for the cell." );
		_vvdb->free(_vvdb);
		return REDIS_ERR;
	}
	if ( NULL == prog){
		redisLog(REDIS_WARNING, "ERROR : Return formula == null, but return code in OK." );
		_vvdb->free(_vvdb);
		return REDIS_ERR;
	}
	// Trace
	//============
	sds s = sdsempty();
	for(int i=0; i < _cell->nr_dim; ++i ){
		s = sdscatprintf(s,"%d ", (int)getCellDiIndex(_cell, i) );
	}
	redisLog(REDIS_WARNING, "Cell:%s formula:%s: dim:%d ",s, prog, dim );
	//redisLog(REDIS_WARNING, "Recompute cell at idx:%s ", s);
	sdsfree(s);
	//============


	formula *f = formulaNew(
			_db,
			 //void *_cube, _cell, int _dim_idx, , const char* _program
			_cube, _cell, dim,  prog
			);

	long double val = f->eval(f, _cell);
	//redisLog(REDIS_WARNING, "Value return after formula eval:%f", (double)val );
	_vvcc->setValueWithResponse(_vvcc, _cell, val);

	f->free(f);
	_vvdb->free(_vvdb);

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
int sliceRecomputeLevel(vvcc* _vvcc, redisDb *_db,cube *_cube,slice *_slice){

	//Trace
	//==============
//	sds s = sdsempty();
//	for(int i=0; i < _slice->nr_dim; ++i ){
//    	elements* el = getSliceElement(_slice, i);
//		s = sdscatprintf(s,"%d ", el->curr_level );
//	}
//	redisLog(REDIS_WARNING, "Levels:%s", s );
//	sdsfree(s);
	//==============
	cell *_cell = cellBuildEmpty(_cube );
	if ( REDIS_ERR == sliceResetElementsCurrElement(_slice) ){
		// No valid combination for this levels combination
		return REDIS_OK;
	}
	while(1){

		// Build initial cell based on element.curr_element
		for( uint32_t i =0; i <_slice->nr_dim; ++i){
			elements* el = getSliceElement(_slice, i);
			setCellIdx(_cell, i, el->curr_elem);
		}
		// Recompute cell
		cellRecompute(_vvcc, _db, _cube, _cell);
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

int32_t sliceGetOverallMaxim(slice *_slice ){
	int slice_max_level = -1;
	for(uint32_t i =0; i< _slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		if ( slice_max_level < el->max_level )
			slice_max_level = el->max_level;
	}
	return slice_max_level;
}
int sliceSetValueUpward(vvcc *_vvcc, redisDb *_db, cube *_cube,slice *_slice) {
	if ( REDIS_ERR == sliceResetElementsLevelFull(_slice) ){
		return REDIS_ERR;
	}
	int done = 0;// exit from outer loop (instead of return)
	vvlvl* _vvlvl = vvlvlNew(_slice->nr_dim);
	while(1) {
		_vvlvl->addCurrentLevel( _vvlvl, _slice);

		int curr_dim = 0;
        while(1) {
        	elements* el=getSliceElement(_slice, curr_dim);
            ++el->curr_level;
            if( el->curr_level == el->max_level + 1 ){ // like std::end. 1 over the end.
                if( curr_dim == (_slice->nr_dim - 1) ){
                	// we are done.
                	done = 1;
                	break;
                    //return REDIS_OK;
                } else {
                    // cascade
                	el->curr_level = 0;
                    ++curr_dim;
                }
            } else {
                // normal
                break;
            }
        }
        if ( done )
        	break;
    }
	_vvlvl->sort(_vvlvl);
	// Now get sorted level, and recompute level
	for(int i=0;i<_vvlvl->nrOfElements;++i){
		int* lvls =  _vvlvl->getLevels(_vvlvl, i);
		for(uint32_t j =0; j< _slice->nr_dim; ++j){
			elements* el = getSliceElement(_slice, j);
			el->curr_level = lvls[j];
		}
		sliceRecomputeLevel(_vvcc, _db, _cube,_slice);
	}
	_vvlvl->free(_vvlvl);

	return REDIS_OK;
}
