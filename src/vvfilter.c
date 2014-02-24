/*
 * vvfilter.c
 *
 *  Created on: Feb 22, 2014
 *      Author: adrian
 */

#include "vvfilter.h"

// Class implementation
static 	int  			vvfilter_free     		(struct vvfilter_struct *_vvfilter);
static 	int 			addSelectedAll     	 	(struct vvfilter_struct *_vvfilter, int dim_idx);
static 	int 			addSelectedDi     	 	(struct vvfilter_struct *_vvfilter, int dim_idx, int di_idx);
static 	int				exec					(struct vvfilter_struct *_vvfilter, void* obj, int (*do_it)(void * obj, void *cell));
static 	int				compact					(struct vvfilter_struct *_vvfilter);
//Internal
static  int 			resetStructCurrElement(slice* _slice);
static  int				fillCell(cell* _cell, slice* _slice);

static  int 			resetStructCurrElement(slice* _slice){
	for(uint32_t i=0; i< _slice->nr_dim; ++i) {
		elements* el = getSliceElement(_slice,i);
		if (0 == el->nr_elem){
			redisLog(REDIS_WARNING,"No items selected on dimension :%d", (int)i);
			return REDIS_ERR;
		}
		el->curr_elem = 0;
	}
	return REDIS_OK;
}
static  int				fillCell(cell* _cell, slice* _slice) {
	for(uint32_t i=0; i< _slice->nr_dim; ++i) {
		elements* el=getSliceElement(_slice,i);
		int idx = getElementsElement(el, el->curr_elem);
		setCellIdx(_cell, i, idx);
	}
	return REDIS_OK;
}
static 	int		exec (struct vvfilter_struct *_vvfilter, void* obj, int	(*do_it)(void * obj, void *cell)) {
	slice *_slice = _vvfilter->_slice;
	cube *_cube = _vvfilter->_cube;

	if( REDIS_OK != _vvfilter->compact(_vvfilter) ) {
		return REDIS_ERR;
	}
	// Init cell values
	if( REDIS_OK != resetStructCurrElement(_slice) ) {
		return REDIS_ERR;
	}
	cell* _cell = cellBuildEmpty(_cube );
	while(1) {
		fillCell(_cell, _slice);
		do_it(obj, _cell);
		int curr_dim = 0;
        while(1) {
        	elements* el=getSliceElement(_slice, curr_dim);
            ++el->curr_elem;
            if( el->curr_elem == el->nr_elem ){ // like std::end. 1 over the end.
                if( curr_dim == (_slice->nr_dim - 1) ){
                    return REDIS_OK;
                } else {
                    // cascade
                	el->curr_elem = 0;
                    ++curr_dim;
                }
            } else {
                // normal
                break;
            }
        }
    }
	cellRelease(_cell);
	return REDIS_OK;
}

static 	int				compact					 (struct vvfilter_struct *_vvfilter){
	slice* _slice = _vvfilter->_slice;
	for(uint32_t i=0; i< _slice->nr_dim; ++i) {
		elements* el=getSliceElement(_slice,i);
		int k = 0;
		for(int32_t j=0; j<el->nr_elem;++j){
			int32_t value = getElementsElement(el, j);
			if ( -1 != value ) {
				if (k != j){
					setElementsElement(el,k,value);
					setElementsElement(el,j, -1);
				}
				++k;
			}
		}
		if (0 == k) {
			redisLog(REDIS_WARNING,"No items selected on dimension :%d", (int)i);
			return REDIS_ERR;
		}
		el->nr_elem = k;
	}
	return REDIS_OK;
}

static 	int 			addSelectedAll     	 	(struct vvfilter_struct *_vvfilter, int dim_idx) {
	elements* el=getSliceElement(_vvfilter->_slice, dim_idx);
	for(int i =0; i<el->nr_elem;++i){
		setElementsElement(el, i, i);
	}
	return REDIS_OK;
}

static 	int 			addSelectedDi     	 	(struct vvfilter_struct *_vvfilter, int dim_idx, int di_idx){
	elements* el=getSliceElement(_vvfilter->_slice, dim_idx);
	setElementsElement(el, di_idx, di_idx);
	return REDIS_OK;
}

vvfilter* vvfilterNew(cube* _cube) {
	vvfilter* _vvfilter = (vvfilter*)sdsnewlen(NULL,sizeof(vvfilter));
	//Functions
	_vvfilter->addSelectedAll 			= addSelectedAll;
	_vvfilter->addSelectedDi			= addSelectedDi;
	_vvfilter->compact	 				= compact;
	_vvfilter->exec 					= exec;
	_vvfilter->free 					= vvfilter_free;
	//Members
	_vvfilter->_cube				= _cube;
	_vvfilter->_slice               =  sliceBuild(_cube);
	return _vvfilter;
}

static 	int  			vvfilter_free     			(struct vvfilter_struct *_vvfilter) {
	sliceRelease( _vvfilter->_slice );
	sdsfree((sds)_vvfilter );_vvfilter = 0;
	return REDIS_OK;
}



