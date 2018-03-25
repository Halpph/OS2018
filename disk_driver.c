#include "disk_driver.h"
#include <stdio.h>

// opens the file (creating it if necessary)
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){

    int bitmap_size = num_blocks/8;
    if(bitmap_size == 0) bitmap_size++; //there must be almost 1 block

    int fd = open(filename, O_RDWR|O_CREAT|O_EXCL,0666);

    if(fd == -1 && EEXIST == errno){
        //file esiste gia
        fd = open(filename,O_RDWR);

        DiskHeader* disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader)+bitmap_size), PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
        if(disk_header == MAP_FAILED){
            close(fd);
        }
        disk->header = disk_header;
        disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);
    }else{
        DiskHeader* disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader)+bitmap_size), PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
        if(disk_header == MAP_FAILED){
            close(fd);
        }
        disk->header = disk_header;
        disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);

        disk_header->num_blocks = num_blocks;
        disk_header->bitmap_blocks = num_blocks;
        disk_header->bitmap_entries = bitmap_size;
        disk_header->free_blocks = num_blocks;
        disk_header->first_free_block = 0;
    }

    disk->fd = fd;
}

// reads the block in position block_num
// returns -1 if the block is free according to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
    // checking if params are ok
    if(block_num > disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL ){
		printf("DiskDriver_readBlock: bad parameters");
        return -1;
	}
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // check if that block is free
    if(block_num >= bit_map->num_bits) return -1;   // invalid block
    BitMapEntryKey entry_key = BitMap_blockToIndex(block_num);
    if(bit_map->entries[entry_key.entry_num] >> entry_key.bit_num & 0)
        return -1;

    int fd = disk->fd;
    // lseek on DISKHEADER+BITMAPENTRIES+MYBLOCKOFFSET
    off_t off = lseek(fd,sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);
    if(off == -1){
        printf("DiskDriver_readBlock: lseek error\n");
        return -1;
    }

    int ret, bytes_reads = 0;
    // read until the whole BLOCK_SIZE is covered
	while(bytes_reads < BLOCK_SIZE){
        // save bytes_read in dest location
		ret = read(fd, dest + bytes_reads, BLOCK_SIZE - bytes_reads);

		if (ret == -1 && errno == EINTR) continue;

		bytes_reads +=ret;
	}

    return 0;
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
    // checking if params are ok
    if(block_num > disk->header->bitmap_blocks || block_num < 0 || src == NULL || disk == NULL ){
		printf("DiskDriver_writeBlock: bad parameters");
        return -1;
	}
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // check if that block is not free
    if(block_num >= bit_map->num_bits) return -1;   // invalid block
    BitMapEntryKey entry_key = BitMap_blockToIndex(block_num);
    if(bit_map->entries[entry_key.entry_num] >> entry_key.bit_num & 1)
        return -1;

    // updating the first free block of the list
    if(block_num == disk->header->first_free_block)																//update first_free_block
        disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num+1);

    // decreasing free_blocks because i'm writing one
    BitMap_set(&bmap, block_num, 1);																			//block become full on bitmap
    disk->header->free_blocks -= 1;

    int fd = disk->fd;
    // lseek on DISKHEADER+BITMAPENTRIES+MYBLOCKOFFSET
    off_t off = lseek(fd,sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);
    if(off == -1){
        printf("DiskDriver_readBlock: lseek error\n");
        return -1;
    }

    int ret, bytes_written = 0;
    // read until the whole BLOCK_SIZE is covered
	while(bytes_written < BLOCK_SIZE){
        // write src on block_num pos
		ret = write(fd, src + bytes_written, BLOCK_SIZE - bytes_written);

		if (ret == -1 && errno == EINTR) continue;

		bytes_written +=ret;
	}
    return 0;
}

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
    if(disk == NULL || start < 0){
        printf("DiskDriver_getFreeBlock: bad parameters\n");
        return -1;
    }
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // getting the first block with status "0" (which means that is a free block) from position start
    return BitMap_get(&bit_map, start, 0);
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
    if(disk == NULL || block_num < 0){
        printf("DiskDriver_freeBlock: bad parameters\n");
        return -1;
    }
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // check if that block is already free
    if(block_num >= bit_map->num_bits) return -1;   // invalid block
    BitMapEntryKey entry_key = BitMap_blockToIndex(block_num);
    if(bit_map->entries[entry_key.entry_num] >> entry_key.bit_num & 0)
        return -1;

    int ret = BitMap_set(bit_map, block_num, 0);
    if(ret == -1){
        printf("DiskDriver_freeBlock: BitMap_set error\n");
        return -1;
    }
    // updating the first free block of the list
    // 1. is it before the current first_free block?
    // 2. is the current first_free block not set (-1)?
    if(block_num < disk->header->first_free_block || disk->header->first_free_block == -1)																//update first_free_block
        disk->header->first_free_block = block_num;

    return 0;
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){
    // TODO:
}
