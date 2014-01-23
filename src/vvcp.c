#include "redis.h"
#include "vv.h"

cube* diBuild(redisClient *c, sds cube_code, int dim, int di){
	sds s = sdsnew("di_");
	s = sdscatsds(s, cube_code);
	s = sdscatprintf(s,"_%d_%d",  dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(c->db, so);
	decrRefCount(so);

	if (redis_data == NULL ){
		addReplyError(c,"Invalid key code");
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
int setValueDownward(redisClient *c, cube* cube, cell* cell, cell_val value
		, cube_data* cube_data
		, int curr_dim  // Algorithm parameters
		){
	int di_idx = getCellDiIndex(cell, curr_dim);
	cube* di = diBuild(c, c->argv[0]->ptr, di_idx);
	if ( NULL == di ){
		return REDIS_ERR;
	}
	if ( diIsSimple(di) ) {
		if ( (*cube->nr_dim - 1 )== curr_dim ) {
			setValueDownward(c, cube, cell, value, cube_data, ++curr_dim);
		}
	} else {
		;
	}


	diRelease(di);
	/*
	di = {dim = cube->get_dim( curr_dim) ; dim ->get_di ( cell->idx at curr_dim ) }

	if (di is simple)
		if  curr_dim != _cube.nr_dims - 1  // if not last version
			set_value_down_full ( ++curr_dim ) // move to next dimension
		set raw value ( cube_data, cell.idx ) // set store value
		// Normal exit
	else
		set raw value ( cube_data, cell.idx ) // set store value
		for each (di->getChildren)
			compute new_value // curr_cell_val = value / nr_children
			build new_cell
			set set_value_down_full ( new_cell, new_value )
			release new_cell
	 */
	return REDIS_OK;
}

int recompute_value_up_full(cube* cube, cell* cell
		,cube_data* cube_data
		,int curr_level  // Algorithm parameters
		,int curr_dim
		){
	/*
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


	 */
	return REDIS_OK;
}
