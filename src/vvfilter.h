/*
 * vvfilter.h
 *
 *  Created on: Feb 22, 2014
 *      Author: adrian
 */

#ifndef VVFILTER_H_
#define VVFILTER_H_
#include "redis.h"
#include "vv.h"

typedef	struct vvfilter_struct{
	cube	    *_cube;
	slice		*_slice;
	int 			(*addSelectedAll)     	 (struct vvfilter_struct *_vvfilter, int dim_idx);
	int 			(*addSelectedDi)     	 (struct vvfilter_struct *_vvfilter, int dim_idx, int di_idx);
	int				(*compact)				 (struct vvfilter_struct *_vvfilter);
	int				(*exec)					 (struct vvfilter_struct *_vvfilter, void* obj, int 			(*do_it)(void * obj, void *cell));
	int 			(*free)     	 		 (struct vvfilter_struct *_vvfilter);
} vvfilter;

vvfilter* vvfilterNew(cube* _cube);

#endif /* VVFILTER_H_ */
