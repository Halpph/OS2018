#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    const char* filename = "./disk_driver.txt";
    BlockHeader block_header; // header for each block

    printf("...populating blocks\n");
    FileBlock* file_block1 = malloc(sizeof(FileBlock));
    file_block1->header = block_header;
    char data1[BLOCK_SIZE-sizeof(BlockHeader)];
    for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++)
        data1[i] = '1'; // populate with block_num
    data1[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0'; // end of file
    strcpy(file_block1->data,data1); // saving string on my file_block data

    FileBlock* file_block2 = malloc(sizeof(FileBlock));
    file_block2->header = block_header;
    char data2[BLOCK_SIZE-sizeof(BlockHeader)];
    for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++)
        data2[i] = '2'; // populate with block_num
    data2[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0'; // end of file
    strcpy(file_block2->data,data2); // saving string on my file_block data

    FileBlock* file_block3 = malloc(sizeof(FileBlock));
    file_block3->header = block_header;
    char data3[BLOCK_SIZE-sizeof(BlockHeader)];
    for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++)
        data3[i] = '3'; // populate with block_num
    data3[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0'; // end of file
    strcpy(file_block3->data,data3); // saving string on my file_block data

    printf("...initializing disk with 3 blocks\n");
    DiskDriver_init(disk_driver,filename,3);
    //printf("...flushing disk\n");
    //DiskDriver_flush(disk_driver);


    /*printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
    printf("DataBlock size %ld\n", sizeof(FileBlock));
    printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
    printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));*/

}
