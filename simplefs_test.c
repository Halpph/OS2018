#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
    int i = 0;
    char entries1[8] = "11110011";
    unsigned char block1 = 0;
    char entries2[8] = "10100100";
    unsigned char block2 = 0;
    char entries3[8] = "00000000";
    unsigned char block3 = 0;
    printf("\n\n@ @ @ @ @ BITMAP TEST @ @ @ @ @\n\n");
    for ( i = 0; i < 8; ++i ){
        block1 |= (entries1[i] == '1') << (7 - i);
        block2 |= (entries2[i] == '1') << (7 - i);

    }
    printf("\nBLOCK[1]: %d\nBLOCK[2]: %d\n",block1,block2);
    BitMap* bit_map = malloc(sizeof(BitMap));
    bit_map->entries = entries3;
    bit_map->num_bits = 64;
    BitMap_print(bit_map);
    printf("\n*****BLOCK TO INDEX*****\n");
    for ( i = 1; i < 20; ++i ){
        BitMapEntryKey key = BitMap_blockToIndex(i);
        printf("BLOCCO[%d] -> Entry[%d] = Index[%d]\n",i,key.entry_num,key.bit_num);

    }
    printf("\n*****INDEX TO BLOCK*****\n");
    int k = 0;
    for ( i = 0; i < 8; ++i ){
        for(k = 0; k < 8; ++k){
            printf("Entry[%d] + Index[%d] = BLOCCO[%d]\n",i,k,BitMap_indexToBlock(i,k));
        }
    }

    printf("\n*****BITMAP GET-SET*****\n");
    printf("PRIMA %d\n",BitMap_get(bit_map,0,1));
    BitMap_set(bit_map,1,1);
    printf("DOPO %d\n",BitMap_get(bit_map,0,1));
    BitMap_set(bit_map,1,0);
    printf("DOPO %d\n",BitMap_get(bit_map,0,1));
    BitMap_print(bit_map);

    printf("\n\n@ @ @ @ @ DISK DRIVER @ @ @ @ @\n\n");
    DiskDriver* disk_driver = (DiskDriver*)malloc(sizeof(DiskDriver));

    /*printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
    printf("DataBlock size %ld\n", sizeof(FileBlock));
    printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
    printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));*/

}
