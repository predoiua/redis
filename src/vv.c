/*
 * Cube operation
 */
//#include <stdio.h>

#include "redis.h"
#include "vv.h"
#include "vvfct.h"
#include "vvdb.h"
#include "vvchangecollector.h"
#include "vvfilter.h"

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
#     cube_code   cube_code     d1_nr     d2_nr   d3_nr
vvcube      c1         100         20        3      44
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

int sliceAddUplinks(vvcc *_vvcc, slice* _slice, vvdb* _vvdb) {
	// i = dimension index
	for( uint32_t i =0; i <_slice->nr_dim; ++i){
		elements* el = getSliceElement(_slice, i);
		// j = di index
		for(uint32_t j=0; j< el->nr_elem; ++j ){
			//redisLog(REDIS_WARNING,"Uplink dim:%d, di:%d", i, j);
			uint32_t level = getElementsElement(el, j);
			if ( -1 == level ){
				// _vvdb->getUpLinks()
				up_links links = _vvdb->getUpLinks(_vvdb, i, j);
				if (NULL != links){
					// _vvdb->getLevel
					// _slice->set level
					for(int k=0; k <nr_up_links(links); ++k){
						int32_t level = _vvdb->getLevel(_vvdb, i, k);
						if ( level != -1 ) {
							setElementsElement(el, k ,level);
						}
					}
				}
			}
		}
	}
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

	cell *_cell = cellBuildFromClient(c, &cube);
	if ( NULL == _cell ) {
		addReplyError(c,"Fail to build cell");
		return;
	}

// Response
	vvcc* _vvcc = vvccNew(c, _vvdb);
// Cube changes
	slice* _slice = sliceBuild(&cube);

    // Actual work..
    cellSetValueDownward(_vvcc, &cube, _cell, target
    		, _vvdb
    		,_slice
    		,  0  // Algorithm parameters
    		);

    // Add up links
    sliceAddUplinks(_vvcc,_slice, _vvdb);

    //
	sliceSetValueUpward(_vvcc, c->db, &cube, _slice);

	_vvcc->flush(_vvcc);

    //Clear
	sliceRelease(_slice);
	_vvcc->free(_vvcc);
    cellRelease(_cell);
    _vvdb->free(_vvdb);
}
void print_cell(cell* _cell){
	sds s = sdsempty();
	for(int i=0; i < _cell->nr_dim; ++i ){
		s = sdscatprintf(s,"%d ", (int)getCellDiIndex(_cell, i) );
	}
	redisLog(REDIS_WARNING, "Print cell: result :%s ", s);
	sdsfree(s);
}
int do_it_write_response(void* p1, void* p2 ){
	vvcc* _vvcc = (vvcc*) p1;
	cell* _cell = (cell*) p2;
	print_cell(_cell);
	vvdb* _vvdb =  (vvdb*)_vvcc->vvdb;
	cell_val* target = _vvdb->getCellValue(_vvdb, _cell);
	_vvcc->writeResponse(_vvcc, _cell, target);

	return REDIS_OK;
}

int set_selected_items(redisClient *c, vvfilter *_vvfilter){
	long long temp_nr;// Working variable
	int cell_elem_stat_pos = 2; // 0 = cmd, 1 = cube name, 2 = first dim index ..
	int nr_dim = *_vvfilter->_cube->nr_dim;
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[cell_elem_stat_pos+i], &temp_nr) != REDIS_OK) {
			redisLog(REDIS_WARNING,"Invalid dimension item index. It must be numerical.");
			return REDIS_ERR;
		}
		// Convention -1 = all
		if ( -1 == temp_nr ){
			_vvfilter->addSelectedAll(_vvfilter, i);
		} else {
			_vvfilter->addSelectedDi(_vvfilter, i, temp_nr);
		}
	}
	return REDIS_OK;
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
//	cell *_cell = cellBuildFromClient(c, &cube);
//	if ( NULL == _cell ) {
//		addReplyError(c,"Fail to build cell");
//		return;
//	}

//==========
	// Response
	vvcc* _vvcc = vvccNew(c, _vvdb);

	vvfilter* _vvfilter = vvfilterNew(&cube);
	if( REDIS_OK == set_selected_items(c, _vvfilter) ) {
		_vvfilter->exec( _vvfilter, _vvcc, do_it_write_response);
	}

	_vvcc->flush(_vvcc);

    //Clear
	_vvfilter->free(_vvfilter);
	_vvcc->free(_vvcc);
//    cellRelease(_cell);
    _vvdb->free(_vvdb);

}
