/*
 * Cube operation
 */
//#include <stdio.h>

#include "redis.h"
#define CUBE_DIM_END "_dim"
#define CUBE_DATA_END "_data"

// Number of bytes per cube cell
#define CELL_BYTES 4

robj* build_cube_key(robj* cube, sds ending){
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
	cube_dim_store = sdsgrowzero(cube_dim_store, sizeof(uint32_t)*(nr_dim + 1));
	*(cube_dim_store) = (uint32_t)nr_dim;
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[3+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Number of elements in dimension is not a number");
			return;
		}
		*(cube_dim_store + sizeof(uint32_t) * (1+i)) = temp_nr;
		cell_nr *= temp_nr;
	}
	// Build cube
	sds cube_data_store = sdsempty();
	cube_data_store = sdsgrowzero(cube_data_store, cell_nr * CELL_BYTES );

	robj* cube_dim_key = build_cube_key(c->argv[1], CUBE_DIM_END);
	robj* cube_data_key = build_cube_key(c->argv[1], CUBE_DATA_END);

	//
	// Do db operations
	//

	//delCommand(fakeClient);	// Code from delCommand, less answer part. It seams that communication part, generate a crash
	replace_store(c->db,cube_dim_key, cube_dim_store);
	replace_store(c->db,cube_data_key, cube_data_store);

    //
    // Clean up
    //
    freeStringObject(cube_dim_key);
    freeStringObject(cube_data_key);

    // Resonse
    addReplyLongLong(c,cell_nr);
}
int compute_index(int nr_dim, uint32_t* dis, uint32_t* dims, size_t* res ) {
	size_t k=0;
    for (int i = nr_dim - 1; i >= 0; --i){
        uint32_t idx_dis = *(dis+i);
        uint32_t nr_elem = *(dims+i);;
        if ( nr_elem <= idx_dis ) {
        	redisLog(REDIS_WARNING,"Index for dimension %d : %d must be < %d \n",i, idx_dis, nr_elem);
        	return -1;
        }
        k = k * nr_elem + idx_dis;
    }
    *res = k;
    return REDIS_OK;
}

//hl = high level
int compute_index_hl(redisClient *c,int nr_dim_cmd, robj* c_dim, size_t* res){
	uint32_t nr_dim = *( (uint32_t*)c_dim->ptr );
	if ( nr_dim != nr_dim_cmd ) {
        redisLog(REDIS_WARNING,"Number of dimension in cube : %d \n", nr_dim);
		return REDIS_ERR;
	}
	uint32_t dis[nr_dim];
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[2+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Invalid dimension item index");
			return REDIS_ERR;
		}
		dis[i]= temp_nr;
	}
	return compute_index(nr_dim, dis, (uint32_t*)c_dim->ptr + 1, res );
}



int set_cube_value_at_index(robj* o, size_t idx, double value) {
	if ( sdslen(o->ptr) < idx ){
    	redisLog(REDIS_WARNING,"Index is greater than data size (%zu < %zu) ! \n",sdslen(o->ptr), idx);
    	return REDIS_ERR;
	}
	//*( (uint32_t*)c_data->ptr + idx ) = 120;
	//redisLog(REDIS_WARNING,"\n init -> after = %p -> = %p \n", data, (char*)data + idx * CELL_BYTES + 1  );
	*((uint64_t*)o->ptr + idx) = (uint64_t)(value * 100.);// in the first byte i keep some info about the cell -> +1
	return REDIS_OK;
}
int get_cube_value_at_index(robj* o, size_t idx, double* value) {
	//*value = *(uint32_t*)((char*)data + idx * CELL_BYTES + 1 )  / 100;
	*value =(double) *((uint64_t*)o->ptr + idx) / 100.;
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
	//
	robj* cube_dim_key = build_cube_key(c->argv[1], CUBE_DIM_END);
	robj* cube_data_key = build_cube_key(c->argv[1], CUBE_DATA_END);

	robj* c_dim = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);
	if (c_dim == NULL || c_data == NULL){
		addReplyError(c,"Invalid cube code");
		return;
	}
	printf("1");
	size_t idx; ;
	if ( REDIS_OK != compute_index_hl(c, c->argc - 3, c_dim, &idx) ) {
		addReplyError(c,"Fail to compute  cell index");
		return;
	}
	printf("2");
	long double target=0.;
	if (REDIS_OK != getLongDoubleFromObject(c->argv[ c->argc  - 1], &target) ) {
		addReplyError(c,"Fail read cell value as a double");
		return;
	}
	printf("2");
	set_cube_value_at_index(c_data, idx, target);
	//
	// Clean up
	//
	freeStringObject(cube_dim_key);
	freeStringObject(cube_data_key);

    // Resonse
    addReplyLongLong(c, 1);

}
void vvget(redisClient *c) {
	robj* cube_dim_key = build_cube_key(c->argv[1], CUBE_DIM_END);
	robj* cube_data_key = build_cube_key(c->argv[1], CUBE_DATA_END);

	robj* c_dim = lookupKeyRead(c->db, cube_dim_key);
	robj* c_data = lookupKeyRead(c->db, cube_data_key);
	// Clean cube codes;
	freeStringObject(cube_dim_key);
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
	double target=0.;
	get_cube_value_at_index(c_data, idx, &target);
	//int64_t res = *( (uint32_t*)c_data->ptr + idx );

	// Resonse
	addReplyDouble(c, target);
}

/*
 * Call :
   VVINIT cube max_length
 *
 * It is based on :
SETBIT key offset bitvalue
void setbitCommand(redisClient *c)
 *
 */
// Obsolete
/*
void vvinit(redisClient *c) {
	// Prepare a SETBIT command
	// Set new values in redisClient
	robj **argv = c->argv;
	int setbit_argc = 4;
    c->argv = zmalloc(sizeof(robj*)*setbit_argc);

    c->argc = 0;
    c->argv[c->argc] = createStringObject("SETBIT", 6);
    zfree(argv[0]);

    ++c->argc;
    c->argv[c->argc] = argv[1];

    ++c->argc;
    c->argv[c->argc] = argv[2];

    c->argc = 0;
    c->argv[c->argc] = createStringObject("0", 1);

    zfree(argv);

    // Call real working function
    setbitCommand(c);
}
*/
