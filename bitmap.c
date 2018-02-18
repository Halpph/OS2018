#include "bitmap.h"

BitMapEntryKey BitMap_blockToIndex(int num){
// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
	BitMapEntryKey* res= malloc(sizeof(BitMapEntryKey));
	res->entry_num=num;
	int i;
	for (i=0;i!=1;i++) res->bit_num=i;//scorre l'array fino alla prima posizione libera
	return BitMapEntryKey;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
	// converts a bit to a linear index
	//non ho capito(???)
	return 0;
	
}

int BitMap_get(BitMap* bmap, int start, int status
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
	int i=start;
	for(;i<num_bits;i++){
		if(bmap->entries[i] == status) return i;//ritorna l'indice se lo trova
	}
	return -1;//sennò torna -1
}



int BitMap_set(BitMap* bmap, int pos, int status){
	// sets the bit at index pos in bmap to status
	if (pos < bmap->num_bits)	{
		bmap->entries[pos]=status;
		return 0;//success
	}
	else return -1;//fail
}
