/*
 * Cube operation
 */
//#include <stdio.h>

#include "redis.h"
#include "vv.h"
#include "vvfct.h"
#include "vvi.h"



robj* build_key(robj* cube, sds ending){
	// Key for cube structure
	sds key = sdsempty();
	key= sdscatlen(key, cube->ptr, sdslen(cube->ptr));
	key= sdscatlen(key, ending , strlen(ending));
	return createObject(REDIS_STRING,key);
}
void replace_store(redisDb *db,robj *key, sds store){
	if (dbDelete(db,key)) {
		signalModifiedKey(db,key);
		notifyKeyspaceEvent(REDIS_NOTIFY_GENERIC,"del",key,db->id);
		server.dirty++;
	}
	//Add new cube structures
    robj* o = createObject(REDIS_STRING,store);
    dbAdd(db,key,o);
}

int decode_cell_idx(redisClient *c, cell *cell){
	long long temp_nr;// Working variable
	int cell_elem_stat_pos = 2; // 0 = cmd, 1 = cube name, 2 = first dim index ..

	for(int i=0; i < cell->nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[cell_elem_stat_pos+i], &temp_nr) != REDIS_OK) {
			//addReplyError(c,"Invalid dimension item index");
			redisLog(REDIS_WARNING,"Invalid dimension item index. It must be numerical.");
			return REDIS_ERR;
		}
		setCellIdx(cell, i, temp_nr);
		//redisLog(REDIS_WARNING,"Index :%d Value before : %lld ,after write %zu\n", i, temp_nr, getCellDiIndex(cell,i) );
	}
	return REDIS_OK;
}


int set_simple_cell_value_at_index(sds data, size_t idx, double value) {
	if ( sdslen(data) < idx ){
    	redisLog(REDIS_WARNING,"Index is greater than data size (%zu < %zu) ! \n",sdslen(data), idx);
    	return REDIS_ERR;
	}
	return set_simple_cell_value_at_index_(data, idx, value);
}

int get_cube_value_at_index(sds data, size_t idx, cell_val* value) {
	return get_cube_value_at_index_(data,idx, value);
}
/*
 * It's not really necessary to have this command, but make further development easier
 * vvdim dim_code nr_di
 *
 */
/*
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
*/
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
	cube_dim_store = sdsgrowzero(cube_dim_store, sizeof(uint32_t)*(nr_dim + 1) );
	cube _cube;
	initCube((&_cube), cube_dim_store);
	setCubeNrDims((&_cube), nr_dim);
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[3+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Number of elements in dimension is not a number");
			return;
		}
		setCubeNrDi((&_cube),i, temp_nr);
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
	decrRefCount(cube_data_key);

    // Response
    addReplyLongLong(c,cell_nr);
}



// idx_di1 idx_diN val
int write_cell_response(redisClient *c, cell *_cell, cell_val *_cell_val, long* nr_writes) {
	sds s = sdsempty();
	for(int i=0; i < _cell->nr_dim; ++i ){
		s = sdscatprintf(s,"%zu ", getCellDiIndex(_cell, i) );
	}
	s = sdscatprintf(s,"%.2f", _cell_val->val);
	//addReplySds(c, s);
	addReplyBulkCBuffer(c, s, sdslen(s));
	sdsfree(s);
	++(*nr_writes);
	return REDIS_OK;
}
int is_same_value(double old_value, double new_value) {
	 return abs(old_value - new_value) < 0.00001 ? 1 : 0;
}

int set_value_with_response(redisClient *c, void *data, cell *_cell, cell_val *_cell_val, long *nr_writes  ) {
	cell_val cv;
	get_cube_value_at_index_(data, _cell->idx, &cv);
	if ( ! is_same_value(cv.val, _cell_val->val) ) {
		set_simple_cell_value_at_index(data, _cell->idx, _cell_val->val);
		get_cube_value_at_index_(data,  _cell->idx, &cv); // Read again the new value
		write_cell_response(c, _cell, &cv, nr_writes);
	}
	return REDIS_OK;
}
int cubeBuild(redisClient *c, robj *cube_code, cube *pc ){
	robj* c_cube = lookupKeyRead(c->db, cube_code);
	if (c_cube == NULL ){
		addReplyError(c,"Invalid cube code");
		return REDIS_ERR;
	}
	initCube(pc, c_cube->ptr);
	return REDIS_OK;
}
int cubeRelease(cube *pc ){
	return REDIS_OK;
}
int buildCubeDataObj(redisClient *c, robj *cube_code, cube_data *cube_data ){
	robj* cube_data_key = build_key(cube_code, CUBE_DATA_END);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);
	decrRefCount(cube_data_key);
	if (c_data == NULL){
		addReplyError(c,"Cube data is missing");
		return REDIS_ERR;
	}
	//redisLog(REDIS_WARNING, "Data address :%p", c_data->ptr);

	cube_data->ptr = c_data->ptr;
	if ( NULL == c_data->ptr ) {
		addReplyError(c,"No space ( null pointer) in Cube data");
		return REDIS_ERR;

	}
	//redisLog(REDIS_WARNING, "Value for data :%p from %p", *pdata, c_data->ptr);
	return REDIS_OK;
}
int releaseCubeDataObj(cube_data *cube_data ){
	return REDIS_OK;
}
cell* cellBuildFromClient(redisClient *c, cube* cube ){
	sds space = sdsempty();
	space = sdsMakeRoomFor( space, cellStructSizeDin( *cube->nr_dim ) );
	cell *_cell = (cell*) space;
	initCellDin(_cell , space); setCellNrDims(_cell,*cube->nr_dim);

	if ( REDIS_OK != decode_cell_idx(c, _cell) ) {
		cellRelease(_cell);
		addReplyError(c,"Fail to compute  cell index");
		return NULL;
	}
	if ( REDIS_OK != compute_index(cube , _cell) ) {
		addReplyError(c,"Fail to compute  flat index");
		return NULL;
	}
	return _cell;
}

int cellRelease(cell *_cell ){
	sdsfree( (sds)_cell );
	_cell = NULL;
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
	long double target=0.;
	if (REDIS_OK != getLongDoubleFromObject(c->argv[ c->argc - 1], &target) ) {
		addReplyError(c,"Fail read cell value as a double");
		return;
	}
//=== Boilerplate code

	// Build all required object
	cube cube;
	if ( REDIS_OK !=  cubeBuild(c, c->argv[1], &cube) ) return;
	cube_data cube_data;
	if ( REDIS_OK !=  buildCubeDataObj(c, c->argv[1], &cube_data) ) return;
	cell *cell = cellBuildFromClient(c, &cube);
	if ( cell == NULL ) return;
//==========
	slice* _slice = sliceBuild(&cube);

	//redisLog(REDIS_WARNING, "Cell flat index :%zu", cell.idx);
    // Response
    //addReplyLongLong(c, 1);
    void *replylen = NULL;
    long nr_writes = 0;
    replylen = addDeferredMultiBulkLength(c);

    // Actual work..
    cell_val cv;
    cv.val = target;
    //set_value_with_response(c, cube_data.ptr, &cell, &cv, &nr_writes );
//    nt setValueDownward(redisClient *c, cube* _cube, cell* _cell, cell_val* _cell_val
//    		, cube_data* cube_data
//    		, int curr_dim  // Algorithm parameters
//    		, long* nr_writes // How many result has been written to client
//    		);

    cellSetValueDownward(c, &cube, cell, &cv
    		, &cube_data
    		,_slice
    		,  0  // Algorithm parameters
    		, &nr_writes // How many result has been written to client
    		);

	int res = sliceSetValueUpward(&cube, _slice);
	//redisLog(REDIS_WARNING,"Done sliceSetValueUpward");

    setDeferredMultiBulkLength(c, replylen, nr_writes);
    //Clear
	sliceRelease(_slice);
    cellRelease(cell);
    cubeRelease(&cube);
    releaseCubeDataObj(&cube_data);


}
void vvget(redisClient *c) {
//=== Boilerplate code
	// Build all required object
	cube cube;
	if ( REDIS_OK !=  cubeBuild(c, c->argv[1], &cube) ) return;
	cube_data cube_data;
	if ( REDIS_OK !=  buildCubeDataObj(c, c->argv[1], &cube_data) ) return;
	cell *cell = cellBuildFromClient(c, &cube);
	if ( cell == NULL ) return;


//==========
	cell_val target;
	get_cube_value_at_index(cube_data.ptr, cell->idx, &target);

	// Response
//	addReplyDouble(c, target.val);
    void *replylen = NULL;
    long cell_resp = 0;
    replylen = addDeferredMultiBulkLength(c);
    write_cell_response(c, cell, &target, &cell_resp);
    setDeferredMultiBulkLength(c, replylen, cell_resp);

}
