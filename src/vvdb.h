/*
 * vvdb.h
 *
 *  Created on: Feb 13, 2014
 *      Author: vagrant
 */

#ifndef VVDB_H_
#define VVDB_H_

#include "vv.h"

// Isolate DB operation
typedef	struct vvdb_struct{
	void							*db;
	void                            *cube;

	//void			(*setValue)    (struct vvdb_struct * _vvdb, double val);
	// get a dimension index for a code
	int 		(*getDimIdx)     	 		(struct vvdb_struct * _vvdb, char* dim_code);
	// get a dimension item index for a code
	int 		(*getDimItemIdx)     		(struct vvdb_struct * _vvdb, int dim_idx, char* di_code);

	// get
    double      (*getValueByDimItemId) 		(struct vvdb_struct *_vvdb, int di_idx);
    double      (*getValueByIds)			(struct vvdb_struct *_vvdb,	int nr_dim, int* dim, int* dim_item);

	int (*free)     (struct vvdb_struct * _vvdb);
} vvdb;

vvdb* vvdbNew(void *_db, void* _cube);

#endif /* VVDB_H_ */
