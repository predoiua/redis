#include "redis.h"

#include "vvchangecollector.h"
#include "debug.h"
#include "vvdb.h"

// Class implementation
static 	int  			vvcc_free     			(struct vvcc_struct *_vvcc);
static  int				setValueWithResponse	(struct vvcc_struct *_vvcc, cell *_cell, long double new_value);
static 	int 			writeResponse			(struct vvcc_struct *_vvcc, cell *_cell, cell_val *_cell_val);

static 	int 			flush     	 			(struct vvcc_struct *_vvcc);

// Private members
static 	int isSameValue	(double old_value, double new_value);

vvcc* vvccNew(void* _client, void *_vvdb) {
	vvcc* _vvcc = sdsnewlen(NULL,sizeof(vvcc));
	//Functions
	_vvcc->writeResponse		= writeResponse;
	_vvcc->setValueWithResponse	= setValueWithResponse;
	_vvcc->flush 				= flush;

	_vvcc->free 				= vvcc_free;

	// NULL initialization for members
	_vvcc->client 			= _client;
	_vvcc->vvdb 			= _vvdb;
	_vvcc->nr_writes		= 0;

	_vvcc->reply	 		= addDeferredMultiBulkLength(_client);

	return _vvcc;
}

// idx_di1 idx_diN val
static int writeResponse(struct vvcc_struct *_vvcc, cell *_cell, cell_val *_cell_val) {
	redisClient *c = (redisClient *)_vvcc->client;
	sds s = sdsempty();
	for(int i=0; i < _cell->nr_dim; ++i ){
		s = sdscatprintf(s,"%zu ", getCellDiIndex(_cell, i) );
	}
	s = sdscatprintf(s,"%.2f", _cell_val->val);
	if ( isCellHold(_cell_val) ){ s = sdscat(s," 1"); } else { s = sdscat(s," 0"); }
	if ( isCellActual(_cell_val) ){ s = sdscat(s," 1"); } else { s = sdscat(s," 0"); }
	//addReplySds(c, s);
	addReplyBulkCBuffer(c, s, sdslen(s));
	sdsfree(s);

	++_vvcc->nr_writes;
	return REDIS_OK;
}

// Old function name
//int set_value_with_response(redisClient *c, vvdb *_vvdb, cell *_cell, long double _cell_val, long *nr_writes  ) {
static int				setValueWithResponse	(struct vvcc_struct *_vvcc, cell *_cell, long double new_value) {
	vvdb *_vvdb = (vvdb *)_vvcc->vvdb;
	long double round = roundf(new_value*100.0f)/100.0f;// !! Just a test
	//long double round = new_value;
	cell_val *_cv = _vvdb->getCellValue(_vvdb, _cell);
	if ( ! isSameValue(_cv->val, round) ) {
		_cv->val = round;
		writeResponse(_vvcc, _cell, _cv);
	}
	return REDIS_OK;
}

static int isSameValue(double old_value, double new_value) {
	 //printf( "Compare %f with %f , result : %d", old_value, new_value, abs(old_value - new_value) < 0.00001 ? 1 : 0);
  	 double diff = old_value - new_value;
  	 if (diff > 0.00001) return 0;
  	 if (diff < -0.00001) return 0;
	 return 1;
}

static 	int 			flush     	 			(struct vvcc_struct *_vvcc) {
	redisClient *c = _vvcc->client;
	setDeferredMultiBulkLength(c, _vvcc->reply, _vvcc->nr_writes);
	return REDIS_OK;
}

static 	int  			vvcc_free     	(struct vvcc_struct *_vvcc){
	sdsfree((sds)_vvcc );
	_vvcc = 0;
	return REDIS_OK;
}
