#ifndef __VVFCT_H
#define __VVFCT_H
#include "redis.h"

//vv fct
void vvcube(redisClient *c);
void vvset(redisClient *c);
void vvget(redisClient *c);

int write_cell_response(redisClient *c, cell *_cell, cell_val *_cell_val, long* nr_writes);
int set_value_with_response(redisClient *c, void* data, cell *_cell, cell_val *_cell_val, long *nr_writes  );

int buildCubeObj(redisClient *c, robj *cube_code, cube *pc );
int releaseCubeObj(cube *pc );
int buildCubeDataObj(redisClient *c, robj *cube_code, cube_data *cube_data );
int releaseCubeDataObj(cube_data *cube_data );
cell* buildCellObjFromClientDin(redisClient *c, cube *_cube );
int releaseCellObjDin(cell *_cell );

robj* build_key(robj* cube, sds ending);
void replace_store(redisDb *db,robj *key, sds store);
int decode_cell_idx(redisClient *c, cell *cell);
int set_simple_cell_value_at_index(sds data, size_t idx, double value);
int get_cube_value_at_index(sds data, size_t idx, cell_val* value);
int is_same_value(double old_value, double new_value);

//vvcp fct
cube* diBuild(redisClient *c, sds cube_code, int dim, int di);
void diRelease(cube* di);
int diIsSimple(cube* di);
int setValueDownward(redisClient *c, cube* _cube, cell* _cell, cell_val* _cell_val
		, cube_data* cube_data
		, int curr_dim  // Algorithm parameters
		, long* nr_writes // How many result has been written to client
		);
int recompute_value_up_full(cube* cube, cell* cell
		,cube_data* cube_data
		,int curr_level  // Algorithm parameters
		,int curr_dim
		);

#endif
