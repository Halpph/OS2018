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
	return (entry + byte_dim) + bit_num;		
}

int BitMap_get(BitMap* bmap, int start, int status ){
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
	if (start > bmap->num_bits/byte_dim) return -1;//start maggiore dalla dimensione dei bit 																									 //all'interno della bitmap
	BitMapEntryKey map = BitMap_blockToIndex(start);
	char entry = bmap[map->entry_num];
	char offset = map[bit_num];
	char stat =status;
	int i=0,j=0;
	
	for (i; i < entry; i++){//scorro tutte le entry
		for (j; j< offset; j++){//scorro gli offset
			if((entry >> offset) == stat) //controllo che siano uguali i bit 
				return BitMap_indexToBlock(entry, offset);//ritorno l'index
		}
	}	
}



int BitMap_set(BitMap* bmap, int pos, int status){
	// sets the bit at index pos in bmap to status
	//posso fare uno shift, inserire il bit nella posizione richiesta e poi fare un'or col numero 	//precedente in modo che sostituisco solo il numero in questione se va sostituito(non sono 		//sicuro che la or funzioni però

 			int i = k/byte_dim;            // i = array index (use: A[i])
      int pos = k%byte_dim;          // pos = bit position in A[i]

      unsigned int flag = status;   // flag = 0000.....00001

      flag = flag << pos;      // flag = 0000...010...000   (shifted k positions)

      bmap->entries[i] = bmap->entries[i] | flag;      // Set the bit at the k-th position in A[
}
