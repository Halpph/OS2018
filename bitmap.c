#include "bitmap.h"

BitMapEntryKey BitMap_blockToIndex(int num){
// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
	int num_blocco = num/8-1;//posizione nell'array di char
	int posizione = num%8-1;//n-esimo bit nel char
	
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
	// converts a bit to a linear index
	int index;
	index = entry + bit_num % 8;
	return index;
	
}

int BitMap_get(BitMap* bmap, int start, int status ){
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
	
}



int BitMap_set(BitMap* bmap, int pos, int status){
	// sets the bit at index pos in bmap to status
	
}
