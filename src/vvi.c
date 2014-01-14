#include "redis.h"
#include "vvi.h"

int di_is_simple(dim _dim, uint32_t dim, cell _cell ) {
	size_t idx = getCellDiIndex(_cell, dim);
	uint32_t order = getDimDiOrder(_dim,idx);
	return (0 == order); // a == b -> 1 if a equal to b; 0 otherwise
}


int c_set( cell _cell, cell_val value ) {

}
int c_break_back(dim _dim, cell _cell, cell_val value) {
	/*
    for(int i=0; i<_cube->rowCount();++i){
        DDimension* dim = _cube->getChild(i);
        d_set( dim, idx, value);

        DDimensionItem* di = dim->getDimItem( idx.at(i));
        if( Planning::DIM_ITEM_TYPE_COMPLEX == di->getType() ){
            //## - daca este item complex se apeleaza din nou functia cate o data pentru fiecare elem simplu copil
            QList<int> pc;
            pozitii_copii(pc, dim, di, Planning::DIM_ITEM_TYPE_SIMPLE);
            foreach (int c, pc) {
                QList<int> idxc(idx);
                idxc[ dim->getPosition()] = c;
                CellValue cv = _storage->getValue(idxc);
                c_break_back(idxc, nr_dim+1, cv.getDouble());
            }
        }
    }
    */
	if ( di_is_simple( _dim, dim, _cell ) ){
		;//set_simple_cell_value_at_index(cell);
	} else {
		;
		//double val_init = d_recalculeaza_cell(dim, idx);
	}
	return 0;
}
/*
 void CubeOperation::d_set( DDimension* dim, QList<int> idx, double value) {
   // qDebug() << "D Set:" << dim->getCode() << idx << " val:" << value;

    DDimensionItem* di = dim->getDimItem( idx.at(dim->getPosition()) );
    if (Planning::DIM_ITEM_TYPE_SIMPLE == di->getType()){
        _storage->setValue(idx, CellValue(value));
        return;
    }
    double val_initiala_parinte = d_recalculeaza_cell(dim, di, idx);

    //TODO FIXE:Functioneaza doar daca agregarea este cu total.
    double val_copil_nou = 0.;
    if (0. == val_initiala_parinte){
        int nr_copii_nohold = getNrChildWithNoHols (dim, di, idx);
        if ( 0 != nr_copii_nohold ){
            val_copil_nou = value / nr_copii_nohold;
        }
    }
    double sum_parent_holds = 0.;
    double sum_child_no_holds = 0.;
    if (_storage->hasHold()){
        sum_parent_holds = getSumHolds (dim, di, idx);
        sum_child_no_holds = val_initiala_parinte - sum_parent_holds;

    }
   //qDebug() << "Val initiala pt " << dim->getCode() <<":" <<di->getCode() << "=" << val_initiala;
    for(int i=0; i<di->rowCount();++i) {
        DDimensionItem* dic = di->getChild(i);
        idx[dim->getPosition()] = dic->getPosition();
        if (0. != val_initiala_parinte){
            if( _storage->hasHold() ){
                CellValue cv = _storage->getValue(idx);
                double val_copil = cv.getDouble();
                val_copil_nou = (value - sum_parent_holds) * val_copil / sum_child_no_holds;
            } else{
                CellValue cv = _storage->getValue(idx);
                double val_copil = cv.getDouble();
                val_copil_nou =  value * val_copil/val_initiala_parinte;
                //qDebug() << "Val init !=0" << val_initiala << ", set " << val_copil_nou;
            }
        }
        if (_storage->hasHold()) {
            //qDebug()<< idx << " val:" << val_copil_nou;
            CellValue cv = _storage->getValue(idx);
            if (!(cv.getBit() & Planning::BIT_CELL_HAS_HOLDS)){
                d_set(dim, idx, val_copil_nou);
            }
        } else {
            d_set(dim, idx, val_copil_nou);
        }
    }
}
 */
int compute_index(cube _cube,  cell _cell ) {
	int nr_dim;
	size_t k=0;
	nr_dim = getCubeNrDim(_cube);
    for (int i = nr_dim - 1; i >= 0; --i){
        uint32_t idx_dis = getCellDiIndex(_cell,i);
        uint32_t nr_elem = getCubeNrDi(_cube,i);
        if ( nr_elem <= idx_dis ) {
        	redisLog(REDIS_WARNING,"Index for dimension %d : %d must be < %d \n",i, idx_dis, nr_elem);
        	return REDIS_ERR;
        }
        k = k * nr_elem + idx_dis;
    }
    setCellFlatIdx(_cell, k);
    return REDIS_OK;
}


int set_simple_cell_value_at_index_(void* ptr, size_t idx, double value) {
	//*( (uint32_t*)c_data->ptr + idx ) = 120;
	//redisLog(REDIS_WARNING,"\n init -> after = %p -> = %p \n", data, (char*)data + idx * CELL_BYTES + 1  );
	((cell_val*)ptr + idx)->val = value;// in the first byte i keep some info about the cell -> +1
	return REDIS_OK;
}
int get_cube_value_at_index_(void  *ptr, size_t idx, cell_val* value) {
	//redisLog(REDIS_WARNING, "Read from cube data, at index : %zu", idx);

	//*value = *(uint32_t*)((char*)data + idx * CELL_BYTES + 1 )  / 100;
	//*value =(cell_val) *((cell_val*)ptr + idx) / 100.;
	*value = *((cell_val*)ptr + idx) ;

	return REDIS_OK;
}
