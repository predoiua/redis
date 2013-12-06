/*
 * Cube operation
 */
//#include <stdio.h>

#include "redis.h"
#include "vv.h"
#include "vvi.h"



robj* build_key(robj* cube, sds ending){
	// Key for cube structure
	sds key = sdsempty();
	key= sdscatlen(key, cube->ptr, sdslen(cube->ptr));
	key= sdscatlen(key, ending , strlen(ending));
	return createObject(REDIS_STRING,key);
}
void replace_store(redisDb * db,robj* key, sds store){
	if (dbDelete(db,key)) {
		signalModifiedKey(db,key);
		notifyKeyspaceEvent(REDIS_NOTIFY_GENERIC,"del",key,db->id);
		server.dirty++;
	}
	//Add new cube structures
    robj* o = createObject(REDIS_STRING,store);
    dbAdd(db,key,o);
}
/*
 * It's not really necessary to have this command, but make further development easier
 * vvdim dim_code nr_di
 *
 */
void vvdim(redisClient *c) {
	long long nr_di = 0; // Nr of dimensions items in dimension
	if (getLongLongFromObject(c->argv[2], &nr_di) != REDIS_OK) {
		addReplyError(c,"Second argument( number of di) is not a number");
		return;
	}

	sds store = sdsempty();
	store = sdsgrowzero(store, dimStructSize(nr_di));

	robj* key = c->argv[1];

	replace_store(c->db,key, store);

	addReplyLongLong(c,nr_di);
}
/*
#     cube_code   dim_number    d1_nr     d2_nr   d3_nr
vvcube      c1         3           20        3      44
1. Create a "cube_dim" string for cube dimension
2. Create a "cube_data" string for cube data.
*/
void vvcube(redisClient *c) {
	long long cell_nr = 1; // Nr cell in the cube
	long long nr_dim = 0; // Nr of dimension in cube
	//
	// Compute some useful info
	//
	if (getLongLongFromObject(c->argv[2], &nr_dim) != REDIS_OK) {
		addReplyError(c,"Second argument( number of dim) is not a number");
		return;
	}
	if ( c->argc != nr_dim + 3){
		addReplyError(c,"Difference between number of parameters in command and value of nr_dim");
		return;
	}
	sds cube_dim_store = sdsempty();
	// Build cube dim store
	cube_dim_store = sdsgrowzero(cube_dim_store, cubeStructSize(nr_dim));
	cube _cube;
	initCube(_cube, cube_dim_store);
	setCubeNrDims(_cube, nr_dim);
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[3+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Number of elements in dimension is not a number");
			return;
		}
		setCubeNrDi(_cube,i, temp_nr);
		cell_nr *= temp_nr;
	}
	// Build cube
	sds cube_data_store = sdsempty();
	cube_data_store = sdsgrowzero(cube_data_store, cell_nr * CELL_BYTES );

	robj* cube_dim_key = c->argv[1];
	robj* cube_data_key = build_key(c->argv[1], CUBE_DATA_END);

	//
	// Do db operations
	//
	replace_store(c->db,cube_dim_key, cube_dim_store);
	replace_store(c->db,cube_data_key, cube_data_store);

    // Clean up
    freeStringObject(cube_data_key);

    // Resonse
    addReplyLongLong(c,cell_nr);
}


//hl = high level
// nr_dim_com = number of dimension from command.
int compute_index_hl(redisClient *c, int nr_dim_cmd, robj* c_cube, size_t* res){
	cube _cube;
	initCube(_cube, c_cube->ptr);
	uint32_t nr_dim = getCubeNrDim(_cube);
	if ( nr_dim != nr_dim_cmd ) {
        redisLog(REDIS_WARNING,"Number of dimension in cube : %d \n", nr_dim);
		return REDIS_ERR;
	}
	char cell_idx[ cellStructSize(nr_dim) ];
	cell _cell;

	initCell(_cell , cell_idx);  setCellNrDims(_cell, nr_dim);
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[2+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Invalid dimension item index");
			return REDIS_ERR;
		}
		setCellIdx(_cell, i, temp_nr);
	}
	return compute_index( _cube, _cell, res );
}



int set_simple_cell_value_at_index(robj* o, size_t idx, cell_val value) {
	if ( sdslen(o->ptr) < idx ){
    	redisLog(REDIS_WARNING,"Index is greater than data size (%zu < %zu) ! \n",sdslen(o->ptr), idx);
    	return REDIS_ERR;
	}
	return set_simple_cell_value_at_index_(o->ptr, idx, value);
}
int get_cube_value_at_index(robj* o, size_t idx, cell_val* value) {
	return get_cube_value_at_index_(o->ptr,idx, value);
}
/*
 * set a value of a cell
   vvset cubecode di1 di2         dix value
   vvset c1 1 1 1 100.
 */
/*
 * Algorithm:
 * 1. Somehow I must find a unique path(s) to the lowest level
 * 2. Do value spreading. ( bear hold in mind :) )
 */
void vvset(redisClient *c) {
	robj* cube_dim_key  = c->argv[1];
	robj* cube_data_key = build_key(c->argv[1], CUBE_DATA_END);

	robj* c_dim = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);

	freeStringObject(cube_data_key);

	if (c_dim == NULL || c_data == NULL){
		addReplyError(c,"Invalid cube code");
		return;
	}
	size_t idx; ;
	if ( REDIS_OK != compute_index_hl(c, c->argc - 3, c_dim, &idx) ) {
		addReplyError(c,"Fail to compute  cell index");
		return;
	}
	long double target=0.;
	if (REDIS_OK != getLongDoubleFromObject(c->argv[ c->argc  - 1], &target) ) {
		addReplyError(c,"Fail read cell value as a double");
		return;
	}
	set_simple_cell_value_at_index(c_data, idx, target);

    // Response
    addReplyLongLong(c, 1);

}
void vvget(redisClient *c) {
	robj* cube_dim_key  = c->argv[1];
	robj* cube_data_key = build_key(c->argv[1], CUBE_DATA_END);

	robj* c_dim = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);
	// Clean cube codes;
	freeStringObject(cube_data_key);

	if (c_dim == NULL || c_data == NULL){
		addReplyError(c,"Invalid cube code");
		return;
	}

	size_t idx;
	if ( REDIS_OK != compute_index_hl(c, c->argc - 2, c_dim, &idx) ) {
		addReplyError(c,"Fail to compute index");
		return;
	}
	cell_val target=0.;
	get_cube_value_at_index(c_data, idx, &target);
	//int64_t res = *( (uint32_t*)c_data->ptr + idx );

	// Resonse
	addReplyDouble(c, target);
}
