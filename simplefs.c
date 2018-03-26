#include "simplefs.h"
#include <stdlib.h>

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){
    DirectoryHandle* dir_handle = malloc(sizeof(DirectoryHandle));
    fs->disk = disk;
    dir_handle ->sfs = fs;

    FirstDirectoryBlock * dcb = malloc(sizeof(FirstDirectoryBlock));
    BlockHeader block_header = malloc(sizeof(BlockHeader));
    FileControlBlock fcb = malloc(sizeof(FileControlBlock));
    dcb->directory = null;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
	if (fs == NULL)
		printf("Impossible to format: Wrong Parameters\n");
		return -1;

	int ret = 0;

	FirstDirectoryBlock rootDir = {0}; //create the block for root directory, set to 0 to clean old data
	rootDir.header.block_in_file = 0;																//populate header
	rootDir.header.previous_block = -1;
	rootDir.header.next_block = -1;
																									//populate fcb
	rootDir.fcb.directory_block = -1;																//no parents => -1
	rootDir.fcb.block_in_disk = 0;
	rootDir.fcb.is_dir = 1;
	strcpy(rootDir.fcb.name, "/");

	fs->disk->header->free_blocks = fs->disk->header->num_blocks;									//clear bitmap to simple format disk
	fs->disk->header->first_free_block = 0;															//starts by 0 because writeBlock will change bitmap
	int bitmap_size = fs->disk->header->bitmap_entries;
	bzero(fs->disk->bitmap_data, bitmap_size);														//function to put 0 in every bytes for bitmap_size length

	ret = DiskDriver_writeBlock(fs->disk, &rootDir, 0);												//write root directory on block 0, offset of diskHeader and bitmap_data already calculated by write
	if (ret == -1)
		printf("%sImpossible to format: problem on writeBlock\n%s", COL_RED, COL_GRAY);				//can't return error becouse function return imposted on void
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory
int SimpleFS_readDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);
