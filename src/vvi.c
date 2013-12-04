#include "redis.h"
#include "vvi.h"

int di_is_simple( uint32_t dim_in_cube, dim* dim, uint32_t* cell ) {
	uint32_t di_idx = *(cell + dim_in_cube);
	//Let's get some details about this di
	return 0;

}

// Break along  dim dimension
// dim = dim index
// cell = cell address as a list of dim items indexes
int d_set(uint32_t dim, uint32_t* cell, cell_val value ){
	if ( 0) { //di_is_simple( dim, cell ) ){
		;//set_simple_cell_value_at_index(cell);
	} else {
		;
		//double val_init = d_recalculeaza_cell(dim, idx);
	}
	return 0;
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
