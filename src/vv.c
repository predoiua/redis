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
/*
#     cube_code   dim_number    d1_nr     d2_nr   d3_nr
vvcube      c1         3           20        3      44
1. Create a "cube_dim" string for cube dimension
2. Create a "cube_data" string for cube data.
*/
void vvcube(redisClient *c) {
	robj *o;
	struct redisClient *fakeClient;
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

	sds cube_data_store = sdsempty();
	cube_data_store = sdsgrowzero(cube_dim_store, cell_nr * CELL_BYTES );

	robj* cube_dim_key = build_cube_key(c->argv[1], CUBE_DIM_END);
	robj* cube_data_key = build_cube_key(c->argv[1], CUBE_DATA_END);

	//
	// Do db operations
	//
	fakeClient = createFakeClient();

	fakeClient->db = c->db;

	//delete the existing cube
	fakeClient->argc = 3;
	fakeClient->argv = zmalloc(sizeof(robj*)*fakeClient->argc);
	fakeClient->argv[1] = cube_dim_key;
	fakeClient->argv[2] = cube_data_key;
	delCommand(fakeClient);

	//Add new cube structures
    o = createObject(REDIS_STRING,cube_dim_store);
    dbAdd(c->db,cube_dim_key,o);

    o = createObject(REDIS_STRING,cube_data_store);
    dbAdd(c->db,cube_data_key,o);
    //
    // Clean up
    //
    freeStringObject(cube_dim_key);
    freeStringObject(cube_data_key);
    zfree(fakeClient->argv);
    freeFakeClient(fakeClient);

    // Resonse
    addReplyLongLong(c,cell_nr);
}
long long compute_index(int nr_dim, uint32_t* dis, uint32_t* dims ) {
	long long k = 0;
    for (int i = nr_dim - 1; i >= 0; --i){
        uint32_t idx_dis = *(dis+i);
        uint32_t nr_elem = *(dims+i);;

        k = k * nr_elem + idx_dis;
    }
    return k;
}
//hl = high level
long long compute_index_hl(redisClient *c,int nr_dim_cmd, robj* c_dim){
	int nr_dim = *( (uint32_t*)c_dim->ptr );
	if ( nr_dim != nr_dim_cmd ) {
		printf( "Number of dimension in cube : %d \n", nr_dim);
		return -1;
	}
	uint32_t dis[nr_dim];
	long long temp_nr;// Working variable
	for(int i=0; i < nr_dim; ++i ){
		if (getLongLongFromObject(c->argv[2+i], &temp_nr) != REDIS_OK) {
			addReplyError(c,"Invalid dimension item index");
			return -1;
		}
		dis[i]= temp_nr;
	}
	return compute_index(nr_dim, dis, (uint32_t*)c_dim->ptr + 1 );
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

	long long idx = compute_index_hl(c, c->argc - 3, c_dim);
	if ( idx == -1) {
		return;
	}

	*( (uint32_t*)c_data->ptr + idx ) = 120;
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

	long long idx = compute_index_hl(c, c->argc - 2, c_dim);
	if ( idx == -1) {
		return;
	}

	int64_t res = *( (uint32_t*)c_data->ptr + idx );

	// Resonse
	addReplyLongLong(c, res);
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
