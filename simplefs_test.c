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
		block_header.previous_block=2;
		block_header.next_block=2;
		block_header.block_in_file=2;
		
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
    
    FileBlock* file_block4 = malloc(sizeof(FileBlock));
    file_block4->header = block_header;
    char data4[BLOCK_SIZE-sizeof(BlockHeader)];
    for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++)
        data4[i] = '4'; // populate with block_num
    data4[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0'; // end of file
    strcpy(file_block4->data,data4); // saving string on my file_block data
    
    FileBlock* file_block5 = malloc(sizeof(FileBlock));
    file_block5->header = block_header;
    char data5[BLOCK_SIZE-sizeof(BlockHeader)];
    for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++)
        data5[i] = '5'; // populate with block_num
    data5[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0'; // end of file
    strcpy(file_block5->data,data5); // saving string on my file_block data

    printf("...initializing disk with 5 blocks\n");
    DiskDriver_init(disk_driver,filename,5);
    printf("we have Num blocks:%d\n", disk_driver->header->num_blocks);
		printf("we have Bitmap Blocks: %d\n", disk_driver->header->bitmap_blocks);
		printf("we have Bitmap Entries: %d\n", disk_driver->header->bitmap_entries);
		printf("we have Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("we have First free block:%d\n", disk_driver->header->first_free_block);
    printf("we have Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
    //printf("...flushing disk\n");
    //DiskDriver_flush(disk_driver);
		
		printf("\n\n...writing blocks\n");
		
		printf("---WRITING ON BLOCK 0---\n");
    DiskDriver_writeBlock(disk_driver,file_block1,0);
    //DiskDriver_flush(disk_driver);
    printf("Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("First free block:%d\n", disk_driver->header->first_free_block);
    printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
    
    printf("---WRITING ON BLOCK 1---\n");
    DiskDriver_writeBlock(disk_driver,file_block2,DiskDriver_getFreeBlock(disk_driver,0));
    //DiskDriver_flush(disk_driver);
    printf("Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("First free block:%d\n", disk_driver->header->first_free_block);
    printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
    
    printf("---WRITING ON BLOCK 2---\n");
    DiskDriver_writeBlock(disk_driver,file_block3,DiskDriver_getFreeBlock(disk_driver,1));
    //DiskDriver_flush(disk_driver);
    printf("Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("First free block:%d\n", disk_driver->header->first_free_block);
    printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
    
    printf("---WRITING ON BLOCK 3---\n");
    DiskDriver_writeBlock(disk_driver,file_block4,DiskDriver_getFreeBlock(disk_driver,2));
    //DiskDriver_flush(disk_driver);
    printf("Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("First free block:%d\n", disk_driver->header->first_free_block);
    printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
    
    printf("---WRITING ON BLOCK 4---\n");
    DiskDriver_writeBlock(disk_driver,file_block5,DiskDriver_getFreeBlock(disk_driver,3));
    //DiskDriver_flush(disk_driver);
    printf("Free blocks:%d\n", disk_driver->header->free_blocks);
	  printf("First free block:%d\n", disk_driver->header->first_free_block);
    printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
  	
  	printf("\n\n...reading blocks\n");
  	
  	FileBlock* test_buffer = malloc(sizeof(FileBlock));
		printf("Read Block 0:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 0);
		printf("%s\n", test_buffer->data);
		printf("Read Block 1:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 1);
		printf("%s\n", test_buffer->data);
		printf("Read Block 2:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 2);
		printf("%s\n", test_buffer->data);
		printf("Read Block 3:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 3);
		printf("%s\n", test_buffer->data);
		printf("Read Block 4:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 4);
		printf("%s\n\n", test_buffer->data);
		
		printf("\n\n...freeing blocks\n");
		
		printf("Free Block 1\n");
		DiskDriver_freeBlock(disk_driver, 1);
		printf("Try to read block 1:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 1);
		printf("free blocks:%d\n", disk_driver->header->free_blocks);
		printf("first free block:%d\n", disk_driver->header->first_free_block);
		printf("bitmap:%d\n\n", disk_driver->bitmap_data[0]);
	
		printf("Free Block 2\n");
		DiskDriver_freeBlock(disk_driver, 2);
		printf("Try to read block 2:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 2);
		printf("Free blocks:%d\n", disk_driver->header->free_blocks);
		printf("First free block:%d\n", disk_driver->header->first_free_block);
		printf("Bitmap:%d\n\n", disk_driver->bitmap_data[0]);
		
		printf("Free Block 0\n");
		DiskDriver_freeBlock(disk_driver, 0);
		printf("Try to read block 0:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 0);
		printf("free blocks:%d\n", disk_driver->header->free_blocks);
		printf("first free block:%d\n", disk_driver->header->first_free_block);
		printf("bitmap:%d\n\n", disk_driver->bitmap_data[0]);
	
		printf("Free Block 3\n");
		DiskDriver_freeBlock(disk_driver, 3);
		printf("Try to read block 3:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 3);
		printf("free blocks:%d\n", disk_driver->header->free_blocks);
		printf("first free block:%d\n", disk_driver->header->first_free_block);
		printf("bitmap:%d\n\n", disk_driver->bitmap_data[0]);
	
		printf("Free Block 4\n");
		DiskDriver_freeBlock(disk_driver, 4);
		printf("Try to read block 4:\n");
		DiskDriver_readBlock(disk_driver, test_buffer, 4);
		DiskDriver_flush(disk_driver);
		printf("free blocks:%d\n", disk_driver->header->free_blocks);
		printf("first free block:%d\n", disk_driver->header->first_free_block);
		printf("bitmap:%d\n\n", disk_driver->bitmap_data[0]);
		
		free(disk_driver);
		free(test_buffer);
		free(file_block1);
		free(file_block2);
		free(file_block3);
		free(file_block4);
		free(file_block5);
    
    /*printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
    printf("DataBlock size %ld\n", sizeof(FileBlock));
    printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
    printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));*/
    
    
    printf("@@@@@@@@@@@@@@@@@@@@@ SIMPLE-FS TEST @@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    

}
