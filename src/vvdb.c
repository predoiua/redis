#include "redis.h"

#include "vvdb.h"
#include "debug.h"

// Class implementation
static 	int  	vvdb_free     	(struct vvdb_struct * _vvdb);
static 	int 	getDimIdx	  	(struct vvdb_struct * _vvdb, char* dim_code);
static	int 	getDimItemIdx   (struct vvdb_struct * _vvdb, int dim_idx, char* di_code);
static	void*   getCellValue	(struct vvdb_struct *_vvdb, void* _cell);

// Internal
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
	_vvdb->getCellValue	= getCellValue;
	// NULL initialization for members
	_vvdb->db 		= NULL;
	_vvdb->cube 	= NULL;
	_vvdb->cube_data = NULL;
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
}

static int getIdFromHash (redisDb *_db, robj *hash_code, char* field_code) {
	int idx = -1;
	robj* o = lookupKeyRead(_db, hash_code);
	if (o == NULL) {
		LOG( "I don't find the hash table : %s ", field_code);
		return idx;
	}
	//Field
	sds s_field=  sdsnewlen(field_code, strlen(field_code));
	robj *field = createObject(REDIS_STRING,s_field);

	int ret;
	if (o->encoding == REDIS_ENCODING_ZIPLIST) {

		INFO( "Zip Encoding ");
		unsigned char *vstr = NULL;
		unsigned int vlen = UINT_MAX;
		long long vll = LLONG_MAX;

		ret = hashTypeGetFromZiplist(o, field, &vstr, &vlen, &vll);
		if (ret < 0) {
			LOG( "ERROR: Didn't find the key :%s: \n", s_field);
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

static	void*  getCellValue	(struct vvdb_struct *_vvdb, void* _cell) {
	cell	*_c = (cell	*)_cell;
	return (cell_val*)_vvdb->cube_data + _c->idx;
}
