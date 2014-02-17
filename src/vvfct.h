#ifndef __VVFCT_H
#define __VVFCT_H
#include "redis.h"
#include "vvdb.h"

//vv fct
void vvcube(redisClient *c);
void vvset(redisClient *c);
void vvget(redisClient *c);

int write_cell_response(redisClient *c, cell *_cell, cell_val *_cell_val, long* nr_writes);
int set_value_with_response(redisClient *c, vvdb* _vvdb, cell *_cell, long double _cell_val, long *nr_writes  );

int cubeBuild(redisClient *c, robj *cube_code, cube *pc );

cell* cellBuildFromClient(redisClient *c, cube *_cube );
cell* cellBuildEmpty(cube* _cube );
cell* cellBuildCell(cell* _cell );
int   cellRelease(cell *_cell );

robj* build_key(robj* cube, sds ending);
void replace_store(redisDb *db,robj *key, sds store);
int decode_cell_idx(redisClient *c, cell *cell);
int is_same_value(double old_value, double new_value);

//vvcp fct
slice* sliceBuild(cube *_cube);
int    sliceRelease(slice* _slice);
int    sliceAddCell(vvdb *_vvdb, cube* _cube, slice* _slice, cell *_cell);

di_children* diBuild(redisClient *c, int cube, int dim, int di);
void  diRelease(di_children* di);
int   diIsSimple(di_children* di);

int cellSetValueDownward(redisClient *c, cube* _cube, cell* _cell, long double _cell_val
		, vvdb*  _vvdb
		, slice* _slice
		, int curr_dim  // Algorithm parameters
		, long* nr_writes // How many result has been written to client
		);
int sliceSetValueUpward(redisDb *_db, cube *_cube, slice *_slice);

#endif
