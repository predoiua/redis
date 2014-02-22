/*
 * vvlevelcollector.h
 *
 *  Created on: Feb 21, 2014
 *      Author: adrian
 */

#ifndef VVLEVELCOLLECTOR_H_
#define VVLEVELCOLLECTOR_H_


// Store levels combination and sort them by sum of level value
/*
[3965] 21 Feb 15:09:38.202 # Levels:1 0 0            - w:1     - m:1
[3965] 21 Feb 15:09:38.203 # Levels:1 1 0            - w:2     - m:1
[3965] 21 Feb 15:09:38.209 # Levels:0 1 0            - w:1     - m:1
return
1 0 0
0 1 0
1 0 0
*/
typedef	struct vvlvl_struct{
	sds			levels;  // 1 element = nrOfDimensions of int
	sds			weights; // 1 element = int = sum of level in a level set
	sds			idxs;	  // 1 element = int = index of level order by weight
	int				nrOfElements;
	int 			nrOfDimensions;
	int 			(*addCurrentLevel)		(struct vvlvl_struct *_vvlvl, void* _slice);
	int*			(*getLevels)			(struct vvlvl_struct *_vvlvl, int position);
	int				(*sort)					(struct vvlvl_struct *_vvlvl);
	int 			(*free)     	 		(struct vvlvl_struct *_vvlvl);
} vvlvl;

vvlvl* vvlvlNew(int nrOfDimensions);

#endif /* VVLEVELCOLLECTOR_H_ */
