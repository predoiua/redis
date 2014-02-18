#include "redis.h"
#include "vvi.h"

//
//int compute_index(cube* _cube,  cell* _cell ) {
//	int nr_dim;
//	size_t k=0;
//	nr_dim = *(_cube->nr_dim);
//    for (int i = nr_dim - 1; i >= 0; --i){
//        uint32_t idx_dis = getCellDiIndex(_cell,i);
//        uint32_t nr_elem = getCubeNrDi(_cube,i);
//        if ( nr_elem <= idx_dis ) {
//        	redisLog(REDIS_WARNING,"Index for dimension %d : %d must be < %d",i, idx_dis, nr_elem);
//        	return REDIS_ERR;
//        }
//        k = k * nr_elem + idx_dis;
//    }
//    //redisLog(REDIS_WARNING,"Flat index: %zu\n",k);
//    _cell->idx = k;
//    return REDIS_OK;
//}

