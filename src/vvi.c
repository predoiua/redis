#include "redis.h"
#include "vvi.h"


int compute_index(cube* _cube,  cell* _cell ) {
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


int set_simple_cell_value_at_index_(void *ptr, size_t idx, double value) {
	//redisLog(REDIS_WARNING, "Set at :%p", ptr);

	((cell_val*)ptr + idx)->val = value;// in the first byte i keep some info about the cell -> +1
	return REDIS_OK;
}
int get_cube_value_at_index_(void *ptr, size_t idx, cell_val* value) {
	//redisLog(REDIS_WARNING, "Get at :%p", ptr);
	*value = *((cell_val*)ptr + idx) ;
	return REDIS_OK;
}
