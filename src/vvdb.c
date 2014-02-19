#include "redis.h"

#include "vvdb.h"
#include "debug.h"

// Class implementation
static 	int  			vvdb_free     	(struct vvdb_struct *_vvdb);
static 	int 			getDimIdx	  	(struct vvdb_struct *_vvdb, char* dim_code);
static	int 			getDimItemIdx   (struct vvdb_struct *_vvdb, int dim_idx, char* di_code);
static	void*   		getCellValue	(struct vvdb_struct *_vvdb, void* _cell);
static int32_t  		getLevel		(struct vvdb_struct *_vvdb, uint32_t dim_idx, size_t di_idx);
static di_children* 	getDiChildren 	(struct vvdb_struct *_vvdb, int dim, int di);
static char* 			getFormula 		(struct vvdb_struct *_vvdb, int dim, int di);
static up_links 		getUpLinks 		(struct vvdb_struct *_vvdb, uint32_t dim, uint32_t di);

// Internal
static int compute_index(cube* _cube,  cell* _cell );
//static int cubeBuild(redisDb *_db, robj *_cube_code, cube *_cube );
static int   getIdFromHash (redisDb *_db, robj *hash_code, char* field_code);
// return the pointer to cube data
static void* getCubeData(redisDb *_db, cube* _cube);
//static int set_simple_cell_value_at_index(void *ptr, size_t idx, double value);
//static int get_cube_value_at_index(void *ptr, size_t idx, cell_val* value);
//
//static int set_simple_cell_value_at_index(void *ptr, size_t idx, double value) {
////	if ( sdslen((sds)ptr) < idx ){
////    	redisLog(REDIS_WARNING,"Index is greater than data size (%zu < %zu) !",sdslen(data), idx);
////    	return REDIS_ERR;
////	}
//
//	((cell_val*)ptr + idx)->val = value;// in the first byte i keep some info about the cell -> +1
//	return REDIS_OK;
//}
//static int get_cube_value_at_index(void *ptr, size_t idx, cell_val* value) {
//	//redisLog(REDIS_WARNING, "Get at :%p", ptr);
//	*value = *((cell_val*)ptr + idx) ;
//	return REDIS_OK;
//}
static void* getCubeData(redisDb *_db, cube* _cube) {
	sds s_key = sdscatprintf(sdsempty(), "%d_data", (int)*_cube->numeric_code);
	robj* o_key = createObject(REDIS_STRING,s_key);
	robj* c_data = lookupKeyRead(_db, o_key);
	decrRefCount(o_key);
	if (c_data == NULL){
		redisLog(REDIS_WARNING,"Cube data is missing");
		return NULL;
	}
	return c_data->ptr;
}

vvdb* vvdbNew(void *_db, void* _cube) {
	//vvdb* _vvdb = malloc(sizeof(vvdb));
	vvdb* _vvdb = sdsnewlen(NULL,sizeof(vvdb));
	//Functions
	_vvdb->free 			= vvdb_free;
	_vvdb->getDimIdx 		= getDimIdx;
	_vvdb->getDimItemIdx	= getDimItemIdx;
	_vvdb->getCellValue		= getCellValue;
	_vvdb->getLevel			= getLevel;
	_vvdb->getDiChildren	= getDiChildren;
	_vvdb->getFormula		= getFormula;
	_vvdb->getUpLinks	 	= getUpLinks;
	// NULL initialization for members
	_vvdb->db 			= NULL;
	_vvdb->cube 		= NULL;
	_vvdb->cube_data 	= NULL;
	// Start to put real values
	_vvdb->db 	 	= _db;
	_vvdb->cube		= _cube;
	_vvdb->cube_data = getCubeData( (redisDb*)_db, (cube*) _cube);
//	if ( REDIS_ERR == cubeBuild(_vvdb->db, cube_code, _vvdb->cube ) ){
//		_vvdb->free(_vvdb);
//		return NULL;
//	}
	if ( NULL == _vvdb->cube_data){
		_vvdb->free(_vvdb);
		return NULL;
	}
	return _vvdb;
}

static int  vvdb_free     (struct vvdb_struct * _vvdb) {
	sdsfree((sds)_vvdb );
	_vvdb = 0;
	return REDIS_OK;
}

static int getIdFromHash (redisDb *_db, robj *hash_code, char* field_code) {
	int idx = -1;
	robj* o = lookupKeyRead(_db, hash_code);
	if (o == NULL) {
		LOG( "I don't find the hash table : %s ", (char*)hash_code->ptr);
		return idx;
	}
	//Field
	sds s_field=  sdsnewlen(field_code, strlen(field_code));
	robj *field = createObject(REDIS_STRING,s_field);

	int ret;
	if (o->encoding == REDIS_ENCODING_ZIPLIST) {

		//INFO( "Zip Encoding ");
		unsigned char *vstr = NULL;
		unsigned int vlen = UINT_MAX;
		long long vll = LLONG_MAX;

		ret = hashTypeGetFromZiplist(o, field, &vstr, &vlen, &vll);
		if (ret < 0) {
			LOG( "ERROR: Didn't find the key :%s: in hash:%s:\n", s_field, (char*)hash_code->ptr);
		} else {
			if (vstr) {
				// THIS IS THE REAL EXIT
				idx = (int) (* (int*)vstr);
			} else {
				INFO( "ERROR: Result as char long long");
			}
		}

	} else if (o->encoding == REDIS_ENCODING_HT) {
		INFO( "SURPRISE : HT Encoding for hashtable");
		robj *value;
		ret = hashTypeGetFromHashTable(o, field, &value);
		if (ret < 0) {
			LOG( "ERROR: Didn't find the key :%s: \n", (char*)field->ptr);
		} else {
			idx = (int) (* (int*)value->ptr);
		}
	}

	decrRefCount(field);
	return idx;
}

static int 	getDimIdx	  (struct vvdb_struct * _vvdb, char* dim_code) {
	int idx = -1;
	// Sample key : "100_dim_code_to_idx"
	cube    *_cube = (cube*) _vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;
	//Generate the key
	sds s = sdsempty();
	s = sdscatprintf(s,"%d_dim_code_to_idx", (int)*_cube->numeric_code);
	robj *so = createObject(REDIS_STRING,s);
	idx = getIdFromHash( _db, so, dim_code );
	decrRefCount(so);
	LOG( "For hash: %s, field:%s => value %d ", (char*)so->ptr, dim_code, idx);

    return idx;
}

static	int 	getDimItemIdx   (struct vvdb_struct * _vvdb, int dim_idx, char* di_code) {
	int idx = -1;
	// Sample key: "100_0_di_code_to_idx"
	cube    *_cube = (cube*) _vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;

	sds s = sdsempty();
	s = sdscatprintf(s,"%d_%d_di_code_to_idx", (int)*_cube->numeric_code, dim_idx);
	robj *so = createObject(REDIS_STRING,s);
	idx = getIdFromHash( _db, so, di_code );
	decrRefCount(so);
    return idx;
}

static int compute_index(cube* _cube,  cell* _cell ) {
	int nr_dim;
	size_t k=0;
	nr_dim = *(_cube->nr_dim);
    for (int i = nr_dim - 1; i >= 0; --i){
        uint32_t idx_dis = getCellDiIndex(_cell,i);
        uint32_t nr_elem = getCubeNrDi(_cube,i);
        if ( nr_elem <= idx_dis ) {
        	redisLog(REDIS_WARNING,"Index for dimension %d : %d must be < %d",i, idx_dis, nr_elem);
        	return REDIS_ERR;
        }
        k = k * nr_elem + idx_dis;
    }
    //redisLog(REDIS_WARNING,"Flat index: %zu\n",k);
    _cell->idx = k;
    return REDIS_OK;
}

static	void*  getCellValue	(struct vvdb_struct *_vvdb, void* _cell) {
	cell	*_c = (cell	*)_cell;
	cube    *_cube = (cube*)_vvdb->cube;

	if( REDIS_OK != compute_index(_cube, _c ) ){
		redisLog(REDIS_WARNING,"Fail to compute  flat index");
		_c->idx = 0; // :(
	}

	return (cell_val*)_vvdb->cube_data + _c->idx;
}

static int32_t getLevel( struct vvdb_struct *_vvdb, uint32_t dim_idx, size_t di_idx){
	cube    *_cube = (cube*)_vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;

	sds s = sdsempty();
	s = sdscatprintf(s,"%d_%d_%d_level", (int)*_cube->numeric_code, (int)dim_idx, (int)di_idx);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(_db, so);

	decrRefCount(so);
	if (redis_data == NULL ){
		redisLog(REDIS_WARNING, "Invalid key : %s", s);
		return -1;
	}
	return *(int32_t*)redis_data->ptr;
}

static di_children* 	getDiChildren (struct vvdb_struct *_vvdb, int dim, int di) {
//	di_children* diBuild(redisClient *c, int cube, int dim, int di){
	cube    *_cube = (cube*)_vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;

	sds s = sdsempty();
	s = sdscatprintf(s,"%d_%d_%d_children", *_cube->numeric_code, dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(_db, so);
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

static char* 	getFormula (struct vvdb_struct *_vvdb, int dim, int di){
	cube    *_cube = (cube*)_vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;

	sds s = sdscatprintf(sdsempty(),"%d_%d_%d_formula", *_cube->numeric_code, dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(_db, so);
	if (redis_data == NULL ){
		redisLog(REDIS_WARNING,"No formula with key :%s", s);
		return NULL;
	}
	//redisLog(REDIS_WARNING,"key :%s=>%s", s, (char*)redis_data->ptr);

	decrRefCount(so);
	return (char*)redis_data->ptr;
}

static up_links	getUpLinks (struct vvdb_struct *_vvdb, uint32_t dim, uint32_t di) {
	cube    *_cube = (cube*)_vvdb->cube;
	redisDb *_db = (redisDb*)_vvdb->db;

	sds s = sdscatprintf(sdsempty(),"%d_%d_%d_up_links", *_cube->numeric_code, dim, di);
	robj *so = createObject(REDIS_STRING,s);
	robj* redis_data = lookupKeyRead(_db, so);
	if (redis_data == NULL ){
		redisLog(REDIS_WARNING,"No up links with key :%s", s);
		return NULL;
	}
	decrRefCount(so);
	return (up_links) redis_data->ptr;

}
