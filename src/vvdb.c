#include "redis.h"

#include "vvdb.h"
#include "debug.h"

// Class implementation
static 	int  	vvdb_free     	(struct vvdb_struct * _vvdb);
static 	int 	getDimIdx	  	(struct vvdb_struct * _vvdb, char* dim_code);
static	int 	getDimItemIdx   (struct vvdb_struct * _vvdb, int dim_idx, char* di_code);
// Internal
//static int cubeBuild(redisDb *_db, robj *_cube_code, cube *_cube );
static int getIdFromHash (redisDb *_db, robj *hash_code, char* field_code);
/*
int cubeBuild(redisDb *_db, robj *_cube_code, cube *_cube ){
	robj* c_cube = lookupKeyRead(_db, _cube_code);
	if (c_cube == NULL ){
		printf("Invalid cube code : %s\n", (char*)_cube_code->ptr);
		return REDIS_ERR;
	}
	initCube(_cube, c_cube->ptr);
	return REDIS_OK;
}
*/
vvdb* vvdbNew(void *_db, void* _cube) {
	vvdb* _vvdb = malloc(sizeof(vvdb));
	//Functions
	_vvdb->free 			= vvdb_free;
	_vvdb->getDimIdx 		= getDimIdx;
	_vvdb->getDimItemIdx	= getDimItemIdx;
	// NULL initialization for members
	_vvdb->db 		= NULL;
	_vvdb->cube 	= NULL;

	// Start to put real values
	_vvdb->db 	 	= _db;
	_vvdb->cube		= _cube;
//	if ( REDIS_ERR == cubeBuild(_vvdb->db, cube_code, _vvdb->cube ) ){
//		_vvdb->free(_vvdb);
//		return NULL;
//	}
	return _vvdb;
}

static int  vvdb_free     (struct vvdb_struct * _vvdb) {
	//free(_vvdb->cube);
	free(_vvdb);
	return 0;
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
