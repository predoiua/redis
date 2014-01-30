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
	/*
	di = {dim = cube->get_dim( curr_dim) ; dim ->get_di ( cell->idx at curr_dim ) }

	if  curr_dim != _cube.nr_dims - 1  // if not last version
		set_value_down_full ( ++curr_dim ) // move to next dimension
	else
		set raw value ( cube_data, cell.idx ) // set store value

	if ( !di is simple)
		compute new_value // curr_cell_val = value / nr_children
		for each (di->getChildren)
			build new_cell
			set set_value_down_full ( new_cell, new_value )
			release new_cell
	 */
	return REDIS_OK;
}

typedef struct {
	uint32_t	nr_elem;
	uint32_t*	elem;
} elements;
#define initElements(_el,_ptr) do { \
    _el->nr_elem = (uint32_t)(*_ptr); \
    _el->elem = (uint32_t *) ( (char*)_ptr + sizeof(uint32_t) ); \
} while(0);

#define setElementsNrElem(_el,_nr_elem) do { \
		*(_el->nr_elem ) = (uint32_t)_nr_elem; \
} while(0);
#define setElementsElement(_el,_idx,_elem) do { \
		*(_el->elem + _idx )= (uint32_t)_elem; \
} while(0);
#define getElementsElement(_el,_idx)  (*(_el->elem + _idx ))

typedef struct {
	uint32_t	nr_dim;
	elements**		ptr; // array of pointer elements
} slice;  // represent a cube slice ( selection ). If on each dim I have one item -> cell



slice* buildSlice(cube *_cube){
	uint32_t nr_dim = *_cube->nr_dim;
	sds res_space =   sdsnewlen(NULL, sizeof(uint32_t) + nr_dim * sizeof(elements*));
	slice* res = (slice*)res_space;
	res->nr_dim = nr_dim;
	for(uint32_t i=0; i< nr_dim; ++i) {
		uint32_t nr_di = getCubeNrDi(_cube, i);
		sds dim_space = sdsnewlen(NULL, sizeof(uint32_t) + nr_di * sizeof(uint32_t));
		elements* el; initElements(el, dim_space);

	}

	return res;
}
/*
 int recompute_values(cube*, slice*
 	  curr_cell
  	  curr_level
  	  curr_dim
 	 ) {

	if  curr_dim != _cube.nr_dims  // if not last version
		recompute_values ( ++curr_dim ) // move to next dimension
	else
		formula->get formula for cell
		cell_val = eval formula
		store (cell, cell_valu)

	elem 			= get_elem( curr _dim );
	curr_dim_level  = get_curr dim level
	get next index at same level
	if ( found ) {
		recompute_values ..
	} else {
		if curr_dim_level  < cube level

		else

	}


 }


 */


	/*
int recompute_value_up_full(cube* cube, cell* cell
		,cube_data* cube_data
		,int curr_level  // Algorithm parameters
		,int curr_dim
		){

	/// Recompute only when cell change( so dim =  0 )
	if curr_dim == 0
		recompute_cell

	get di ( curr_dim )
	//Move up a level on this dim, if possible
	if di->level == curr_level
		if di->hasParents
			for each di->getParent
				new_cell
				recompute_value_up_full( new_cell, curr_dim = 0 )
				release new_cell

	//Move to next dimension
	 if curr_dim == last dim
	 	 if curr_leve == cube.max_level
	 	 	 exit
		// One level up
		recompute_value_up_full( curr_level++ , curr_dim = 0 )
	else
		// try next dimension
		recompute_value_up_full( curr_dimm + 1 )

	return REDIS_OK;
}
	 */

