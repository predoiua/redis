#include "redis.h"
#include "vvi.h"

int di_is_simple(dim _dim, uint32_t dim, cell _cell ) {
	size_t idx = getCellDiIndex(_cell, dim);
	uint32_t order = getDimDiOrder(_dim,idx);
	return (0 == order); // a == b -> 1 if a equal to b; 0 otherwise
}

// Break along  dim dimension
// dim = dim index
// cell = cell address as a list of dim items indexes
int d_set(dim _dim, uint32_t dim, cell _cell, cell_val value ){
	if ( di_is_simple( _dim, dim, _cell ) ){
		;//set_simple_cell_value_at_index(cell);
	} else {
		;
		//double val_init = d_recalculeaza_cell(dim, idx);
	}
	return 0;
}

int compute_index(cube _cube,  cell _cell, size_t* res ) {
	int nr_dim;
	size_t k=0;
	nr_dim = getCubeNrDim(_cube);
    for (int i = nr_dim - 1; i >= 0; --i){
        uint32_t idx_dis = getCellDiIndex(_cell,i);
        uint32_t nr_elem = getCubeNrDi(_cube,i);
        if ( nr_elem <= idx_dis ) {
        	redisLog(REDIS_WARNING,"Index for dimension %d : %d must be < %d \n",i, idx_dis, nr_elem);
        	return -1;
        }
        k = k * nr_elem + idx_dis;
    }
    *res = k;
    return REDIS_OK;
}


int set_simple_cell_value_at_index_(void* ptr, size_t idx, cell_val value) {
	//*( (uint32_t*)c_data->ptr + idx ) = 120;
	//redisLog(REDIS_WARNING,"\n init -> after = %p -> = %p \n", data, (char*)data + idx * CELL_BYTES + 1  );
	*((cell_val*)ptr + idx) = (cell_val)(value * 100.);// in the first byte i keep some info about the cell -> +1
	return REDIS_OK;
}
int get_cube_value_at_index_(void  *ptr, size_t idx, cell_val* value) {
	//*value = *(uint32_t*)((char*)data + idx * CELL_BYTES + 1 )  / 100;
	*value =(cell_val) *((cell_val*)ptr + idx) / 100.;
	return REDIS_OK;
}
