#ifndef __VVCHANGECOLLECTOR_H
#define __VVCHANGECOLLECTOR_H

#include "vv.h"
// Collect all changes and send them to the client
typedef	struct vvcc_struct{
	void							*client;
	void							*vvdb;
	void 							*reply;

	long 							nr_writes;

	int 			(*writeResponse)		(struct vvcc_struct *_vvcc, cell *_cell, cell_val *_cell_val);
	int				(*setValueWithResponse)	(struct vvcc_struct *_vvcc, cell *_cell, long double new_value);
	int 			(*flush)     	 		(struct vvcc_struct *_vvcc);
	int 			(*free)     	 		(struct vvcc_struct *_vvcc);
} vvcc;

vvcc* vvccNew(void *_client, void *_vvdb);

#endif
