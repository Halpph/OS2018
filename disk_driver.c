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
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
    // checking if params are ok
    if(block_num > disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL ){
		printf("DiskDriver_readBlock: bad parameters");
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
