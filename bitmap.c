#include "bitmap.h"
#define byte_dim 8

BitMapEntryKey BitMap_blockToIndex(int num){
// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
	BitMapEntryKey map;
	int byte = num / byte_dim;
	map.entry_num = byte;
	char offset = num -(byte*byte_dim);
	map.bit_num = offset;
	return map;	
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
	// converts a bit to a linear index
	if( entry<0 || bit_num < 0 ) return -1;
	return (entry + byte_dim)+ bit_num;		
}

int BitMap_get(BitMap* bmap, int start, int status ){
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
	
}



int BitMap_set(BitMap* bmap, int pos, int status){
	// sets the bit at index pos in bmap to status
	
}
