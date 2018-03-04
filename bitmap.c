#include "bitmap.h"
#define byte_dim 8

BitMapEntryKey BitMap_blockToIndex(int num){
// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
	BitMapEntryKey map;
	int pos = num / 8;
	int offset = num % 8;
	map.entry_num = pos;
	map.offset = offset;
	return map;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
	// converts a bit to a linear index
	if( entry<0 || bit_num < 0 ) return -1;
	return (entry * byte_dim) + bit_num;
}

int BitMap_get(BitMap* bmap, int start, int status ){
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
	if (start > bmap->num_bits) return -1; //start maggiore dal numero dei bit
	 										//all'interno della bitmap
	int i = 0;
	int j = 0;
	char stat = status;
	for(i = 0; i < num_bits-start; i++){
		BitMapEntryKey map = BitMap_blockToIndex(start+i);
		if(bmap->entries[map.entry_num] >> map.bit_num) & status)
		 	return BitMap_indexToBlock(map.entry_num,offset);
	}
	return -1;
}



int BitMap_set(BitMap* bmap, int pos, int status){
	// sets the bit at index pos in bmap to status
	//posso fare uno shift, inserire il bit nella posizione richiesta
	// e poi fare un'or col numero
	//precedente in modo che sostituisco solo il numero in questione
	//se va sostituito(non sono sicuro che la or funzioni però)
	if(pos > bmap->num_bits) return -1;
	BitMapEntryKey map = BitMap_indexToBlock(pos);
	unsigned int flag = status; // flag = 0000.....00001
	flag = flag << pos; // flag = 0000...010...000   (shifted k positions)
	bmap->entries[map->entry_num] = bmap->entries[map->entry_num] | flag;
}
