/*
 * vvdb.h
 *
 *  Created on: Feb 13, 2014
 *      Author: vagrant
 */

#ifndef VVDB_H_
#define VVDB_H_

#include <stdint.h>
#include "vv.h"

// Isolate DB operation
typedef	struct vvdb_struct{
	void							*db;
	void                            *cube;
	void							*cube_data;
	//void			(*setValue)    (struct vvdb_struct * _vvdb, double val);
	// get a dimension index for a code
	int 			(*getDimIdx)     	 		(struct vvdb_struct * _vvdb, char* dim_code);
	// get a dimension item index for a code
	int 			(*getDimItemIdx)     		(struct vvdb_struct * _vvdb, int dim_idx, char* di_code);

	up_links 		(*getUpLinks) 				(struct vvdb_struct *_vvdb, uint32_t dim, uint32_t di);

	// get , cv valud
	void*      		(*getCellValue)				(struct vvdb_struct *_vvdb, void* _cell);

	int32_t  		(*getLevel)					(struct vvdb_struct *_vvdb, uint32_t dim_idx, size_t di_idx);
	di_children* 	(*getDiChildren) 			(struct vvdb_struct *_vvdb, int dim, int di);

	char* 			(*getFormula) 				(struct vvdb_struct *_vvdb, int dim, int di);
	//
	int 			(*free)     				(struct vvdb_struct * _vvdb);
} vvdb;

vvdb* vvdbNew(void *_db, void* _cube);

#endif /* VVDB_H_ */
