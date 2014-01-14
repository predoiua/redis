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

    // Response
    addReplyLongLong(c,cell_nr);
}

int decode_cell_idx(redisClient *c, cell cell){
	long long temp_nr;// Working variable

	for(int i=0; i < *cell.nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[2+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Invalid dimension item index");
			return REDIS_ERR;
		}
		setCellIdx(cell, i, temp_nr);
		//redisLog(REDIS_WARNING,"Index :%d Value before : %d ,after write %d\n", i, temp_nr, getCellDiIndex(cell,i) );
	}
	return REDIS_OK;
}


int set_simple_cell_value_at_index(robj* o, size_t idx, double value) {
	if ( sdslen(o->ptr) < idx ){
    	redisLog(REDIS_WARNING,"Index is greater than data size (%zu < %zu) ! \n",sdslen(o->ptr), idx);
    	return REDIS_ERR;
	}
	return set_simple_cell_value_at_index_(o->ptr, idx, value);
}

int get_cube_value_at_index(robj* o, size_t idx, cell_val* value) {
	return get_cube_value_at_index_(o->ptr,idx, value);
}

// idx_di1 idx_diN val
int write_cell_response(redisClient *c, cell _cell, cell_val _cell_val, long* nr_writes) {
	sds s = sdsempty();
	for(int i=0; i < *_cell.nr_dim; ++i ){
		s = sdscatprintf(s,"%zu ", getCellDiIndex(_cell, i) );
	}
	s = sdscatprintf(s,"%.2f", _cell_val.val);
	//addReplySds(c, s);
	addReplyBulkCBuffer(c, s, sdslen(s));
	sdsfree(s);
	++(*nr_writes);
	return REDIS_OK;
}
int is_same_value(cell_val cv, double new_value) {
	 return abs(cv.val - new_value) < 0.00001 ? 1 : 0;
}
int set_value_with_response(redisClient *c, robj* data, cell cell, double new_value, long* nr_writes  ) {
	cell_val cv;
	get_cube_value_at_index_(data->ptr,  *cell.idx, &cv);
	if ( ! is_same_value(cv, new_value) ) {
		set_simple_cell_value_at_index(data, *cell.idx, new_value);
		get_cube_value_at_index_(data->ptr,  *cell.idx, &cv); // Read again the new value
		write_cell_response(c, cell, cv, nr_writes);
	}
	return REDIS_OK;
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
//=== Boilerplate code
	robj* cube_dim_key  = c->argv[1];
	robj* cube_data_key = build_key(c->argv[1], CUBE_DATA_END);

	robj* c_cube = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);

	freeStringObject(cube_data_key);

	if (c_cube == NULL || c_data == NULL){
		addReplyError(c,"Invalid cube code");
		return;
	}

	// Build cube - details about cube
	cube cube;
	initCube(cube, c_cube->ptr);
	// Build cell - details about cell address
	cell cell;
	char cell_idx[ cellStructSize( *cube.nr_dim ) ];
	initCell(cell , cell_idx); setCellNrDims(cell,*cube.nr_dim);
	if ( REDIS_OK != decode_cell_idx(c, cell) ) {
		addReplyError(c,"Fail to compute  cell index");
		return;
	}
	if ( REDIS_OK != compute_index(cube , cell) ) {
		addReplyError(c,"Fail to compute  flat index");
		return;
	}
//==========

	long double target=0.;
	if (REDIS_OK != getLongDoubleFromObject(c->argv[ c->argc  - 1], &target) ) {
		addReplyError(c,"Fail read cell value as a double");
		return;
	}


    // Response
    //addReplyLongLong(c, 1);
    void *replylen = NULL;
    long cell_resp = 0;
    replylen = addDeferredMultiBulkLength(c);

    //int set_value_with_response(redisClient *c, robj* data, cell _cell, double new_val, long* nr_writes  ) {
    set_value_with_response(c, c_data, cell, target, &cell_resp );
    //set_simple_cell_value_at_index(c_data, *cell.idx, target);
    //write_cell_response(c, cell, target, &cell_resp);

    setDeferredMultiBulkLength(c, replylen, cell_resp);

}
void vvget(redisClient *c) {
//=== Boilerplate code
	robj* cube_dim_key  = c->argv[1];
	robj* cube_data_key = build_key(c->argv[1], CUBE_DATA_END);

	robj* c_cube = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);
	// Clean cube codes;
	freeStringObject(cube_data_key);

	if (c_cube == NULL || c_data == NULL){
		addReplyError(c,"Invalid cube code");
		return;
	}

	// Build cube - details about cube
	cube cube;
	initCube(cube, c_cube->ptr);
	// Build cell - details about cell address
	cell cell;
	char cell_idx[ cellStructSize( *cube.nr_dim ) ];
	initCell(cell , cell_idx); setCellNrDims(cell,*cube.nr_dim);

	if ( REDIS_OK != decode_cell_idx(c, cell) ) {
		addReplyError(c,"Fail to compute  cell index");
		return;
	}
	if ( REDIS_OK != compute_index(cube , cell) ) {
		addReplyError(c,"Fail to compute  flat index");
		return;
	}
//==========
	cell_val target;
	get_cube_value_at_index(c_data, *cell.idx, &target);

	// Response
//	addReplyDouble(c, target.val);
    void *replylen = NULL;
    long cell_resp = 0;
    replylen = addDeferredMultiBulkLength(c);
    write_cell_response(c, cell, target, &cell_resp);
    setDeferredMultiBulkLength(c, replylen, cell_resp);

}
