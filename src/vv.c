/*
 * Cube operation
 */
//#include <stdio.h>

#include "redis.h"
#include "vv.h"
#include "vvfct.h"
#include "vvi.h"
#include "vvdb.h"


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
		//redisLog(REDIS_WARNING,"Index :%d Value before : %lld ,after write %zu", i, temp_nr, getCellDiIndex(cell,i) );
	}
	return REDIS_OK;
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
	long long cube_code_number = 0; // A numerical code for cube
	//
	// Compute some useful info
	//
	if (getLongLongFromObject(c->argv[2], &cube_code_number) != REDIS_OK) {
		addReplyError(c,"Second argument( number of dim) is not a number");
		return;
	}
	long long nr_dim = c->argc - 3;
	sds cube_dim_store = sdsempty();
	// Build cube dim store
	cube_dim_store = sdsgrowzero(cube_dim_store, getCubeSize(nr_dim)  );
	cube _cube;
	initCube(&_cube, cube_dim_store);
	setCubeNrDims(&_cube, nr_dim);
	setCubeNumericCode(&_cube, cube_code_number);
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[3+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Number of elements in dimension is not a number");
			return;
		}
		setCubeNrDi(&_cube,i, temp_nr);
		cell_nr *= temp_nr;
		//redisLog(REDIS_WARNING,"Total cells: %d cells , %d number di.",(uint32_t)cell_nr, (uint32_t)temp_nr);
	}

	// Build cube
	sds cube_data_store = sdsempty();
	cube_data_store = sdsgrowzero(cube_data_store, cell_nr * CELL_BYTES );

	robj	*cube_dim_key = c->argv[1];
	sds 	s_key = sdscatprintf(sdsempty(), "%d_data", (int)*_cube.numeric_code);
	robj	*cube_data_key = createObject(REDIS_STRING,s_key);

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

int set_value_with_response(redisClient *c, vvdb *_vvdb, cell *_cell, long double _cell_val, long *nr_writes  ) {
	long double round = roundf(_cell_val*100.0f)/100.0f;// !! Just a test

	cell_val *_cv = _vvdb->getCellValue(_vvdb,_cell);
	if ( ! is_same_value(_cv->val, round) ) {
		_cv->val = round;
		write_cell_response(c, _cell, _cv, nr_writes);
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

cell* cellBuildFromClient(redisClient *c, cube* cube ){
	sds space = sdsempty();
	space = sdsMakeRoomFor( space, cellStructSizeDin( *cube->nr_dim ) );
	cell *_cell = (cell*) space;
	initCellDin(_cell , space); setCellNrDims(_cell,*cube->nr_dim);

	if ( REDIS_OK != decode_cell_idx(c, _cell) ) {
		cellRelease(_cell);
		redisLog(REDIS_WARNING,"Fail to compute  cell index");
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
	redisLog(REDIS_WARNING, "Cube numberic_code:%d nr dim:%d nr di in 1st:%d", (int) *cube.numeric_code, (int) *cube.nr_dim, (int) *cube.nr_di);
	for(int i =0 ; i< *cube.nr_dim; ++i){
		redisLog(REDIS_WARNING, "Nr di:%d or %d", (int) *(cube.nr_di + i), getCubeNrDi((&cube),i));

	}
	vvdb* _vvdb = vvdbNew(c->db, &cube);
	if( NULL == _vvdb ){
		addReplyError(c,"Missing data objects");
		return;
	}

	cell *cell = cellBuildFromClient(c, &cube);
	if ( NULL == cell ) {
		addReplyError(c,"Fail to build cell");
		return;
	}

//==========
	slice* _slice = sliceBuild(&cube);

    // Response
    void *replylen = NULL;
    long nr_writes = 0;
    replylen = addDeferredMultiBulkLength(c);

    // Actual work..
    cellSetValueDownward(c, &cube, cell, target
    		, _vvdb
    		,_slice
    		,  0  // Algorithm parameters
    		, &nr_writes // How many result has been written to client
    		);

	sliceSetValueUpward(c->db, &cube, _slice);
    setDeferredMultiBulkLength(c, replylen, nr_writes);

    //Clear
	sliceRelease(_slice);
    cellRelease(cell);
    _vvdb->free(_vvdb);
}
void vvget(redisClient *c) {
//=== Boilerplate code
	// Build all required object
	cube cube;
	if ( REDIS_OK !=  cubeBuild(c, c->argv[1], &cube) ) return;
	vvdb* _vvdb = vvdbNew(c->db, &cube);
	if( NULL == _vvdb ){
		addReplyError(c,"Missing data objects");
		return;
	}
	cell *_cell = cellBuildFromClient(c, &cube);
	if ( NULL == _cell ) {
		addReplyError(c,"Fail to build cell");
		return;
	}

//==========
	cell_val* target = _vvdb->getCellValue(_vvdb, _cell);
	// Response
//	addReplyDouble(c, target.val);
    void *replylen = NULL;
    long cell_resp = 0;
    replylen = addDeferredMultiBulkLength(c);
    write_cell_response(c, _cell, target, &cell_resp);
    setDeferredMultiBulkLength(c, replylen, cell_resp);

    //Clear
    cellRelease(_cell);
    _vvdb->free(_vvdb);

}
