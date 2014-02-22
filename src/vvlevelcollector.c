#include "redis.h"
#include "vvlevelcollector.h"
#include "vv.h"

// implementation in qsort.c
void vvqsort(void* obj, void *base, size_t nmemb, size_t size, int (*compar)(void*, const void *, const void *));

// Class implementation
static 	int  			vvlvl_free     			(struct vvlvl_struct *_vvlvl);
static  int				addCurrentLevel			(struct vvlvl_struct *_vvlvl, void* _slice);
static 	int* 			getLevels				(struct vvlvl_struct *_vvlvl, int position);
static 	int 			sort     	 			(struct vvlvl_struct *_vvlvl);

vvlvl* vvlvlNew(int nrOfDimensions) {
	vvlvl* _vvlvl = (vvlvl*)sdsnewlen(NULL,sizeof(vvlvl));
	//Functions
	_vvlvl->addCurrentLevel			= addCurrentLevel;
	_vvlvl->getLevels				= getLevels;
	_vvlvl->sort 					= sort;

	_vvlvl->free 					= vvlvl_free;
	//
	_vvlvl->levels  = sdsempty();
	_vvlvl->weights = sdsempty();
	_vvlvl->idxs    = sdsempty();

	// NULL initialization for members
	_vvlvl->nrOfElements		= 0;
	_vvlvl->nrOfDimensions		= nrOfDimensions;

	return _vvlvl;
}

static 	int  			vvlvl_free     			(struct vvlvl_struct *_vvlvl) {
	sdsfree((sds)_vvlvl );
	_vvlvl = 0;
	return REDIS_OK;
}
static  int				addCurrentLevel			(struct vvlvl_struct *_vvlvl, void* _s){
	//_vvlvl->levels = sdsgrowzero()
	int level[_vvlvl->nrOfDimensions];
	int weight = 0;
	int idx = 0;
	slice* _slice = (slice*) _s;
	for(uint32_t i =0; i< _slice->nr_dim; ++i){
    	elements* el = getSliceElement(_slice, i);
    	level[i] = el->curr_level;
    	weight  += el->curr_level;
	}
	idx = _vvlvl->nrOfElements;
	_vvlvl->levels  = sdscatlen(_vvlvl->levels, (char*)level, _vvlvl->nrOfDimensions * sizeof(int) );
	_vvlvl->weights = sdscatlen(_vvlvl->weights,(char*)&weight, sizeof(int));
	_vvlvl->idxs    =  sdscatlen(_vvlvl->idxs,(char*)&idx, sizeof(int));
	++_vvlvl->nrOfElements;
	return 0;
}

static 	int* 			getLevels				(struct vvlvl_struct *_vvlvl, int position){
	int lvl_idx = *( (int*)_vvlvl->idxs +position );
	return (int*)_vvlvl->levels + lvl_idx * _vvlvl->nrOfDimensions;
}

int compare_weight(void* obj, const void *a, const void *b) {
	struct vvlvl_struct *_vvlvl =(struct vvlvl_struct *) obj;
	int first_weight = *( (int*)_vvlvl->weights + *(int*)a );
	int second_weight = *( (int*)_vvlvl->weights + *(int*)b );
	return first_weight - second_weight;
}

static 	int 			sort     	 			(struct vvlvl_struct *_vvlvl){
	//    qsort(a, maxnum, sizeof(T), Comp);
	vvqsort( (void*)_vvlvl,
			(void*)_vvlvl->idxs, _vvlvl->nrOfElements, sizeof(int), compare_weight);
	return REDIS_OK;
}
