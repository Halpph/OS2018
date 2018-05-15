#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){
    DirectoryHandle* dir_handle = malloc(sizeof(DirectoryHandle));
    fs->disk = disk;

    FirstDirectoryBlock * fdb = malloc(sizeof(FirstDirectoryBlock));

    int ret = DiskDriver_readBlock(disk,fdb,0); // reading first block
    if(ret == -1){ // no block
        free(fdb);
        return NULL;
    }

    DirectoryHandle* dir_handle = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    dir_handle->sfs = fs;
    dir_handle->dcb = fdb;
    dir_handle->directory = NULL;
    dir_handle->pos_in_block = 0;

    return dir_handle;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
	if (fs == NULL){
		printf("Impossible to format: Wrong Parameters\n");
		return -1;
    }

	int ret = 0;

	FirstDirectoryBlock rootDir = {0}; //create the block for root directory, set to 0 to clean old data
    //populate header
	rootDir.header.block_in_file = 0;
	rootDir.header.previous_block = -1;
	rootDir.header.next_block = -1;

    //populate fcb
	rootDir.fcb.directory_block = -1; //root has no parents => -1
	rootDir.fcb.block_in_disk = 0;
	rootDir.fcb.is_dir = 1; // is directory: YES
	strcpy(rootDir.fcb.name, "/"); //name = "/"

	fs->disk->header->free_blocks = fs->disk->header->num_blocks;  //all blocks are free
	fs->disk->header->first_free_block = 0;
	int bitmap_size = fs->disk->header->bitmap_entries; //num_blocks entries
	memset(fs->disk->bitmap_data,'\0', bitmap_size); //put 0 in every bytes for bitmap_size length

	ret = DiskDriver_writeBlock(fs->disk, &rootDir, 0);		//write root directory on block 0
	if (ret == -1)
		printf("Impossible to format: problem on writeBlock\n");
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock

// 3 scenarios:
// 1) file already exists
// 2) there's no space in disk
// 3) everything is ok, just find a free space and write the block
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){
    int ret;

    FirstDirectoryBlock* fdb = d->dcb;
    DiskDriver* disk_driver = d->fs->disk;

    if(fdb->num_entries > 0){ // directory isn't empty
        // checking file esistence in FirstDirectoryBlock
        FirstFileBlock to_check;
        int i;
        for(i = 0; i < entries; i++){
            if(fdb->file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,&to_check,file_blocks[i]) != -1)){ //check if block is free
                if(strncmp(to_check.fdb.name,filename,128)){
                    printf("createFile: File already exists!\n");
                    return NULL;
                }
            }
        }

        // checking file esistence in DirectoryBlock
        int next_block = fdb->header.next_block;
        DirectoryBlock db;

        while(next_block != -1){ // while I have blocks
            ret = DiskDriver_readBlock(disk_driver,db,next_block); // reading each block
            if(ret == -1){
                printf("createFile: error in readBlock\n");
                return NULL;
            }
            // check file esistence in each block
            for(i = 0; i < entries; i++){
                if(db.file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,&to_check,file_blocks[i]) != -1)){ //check if block is free
                    if(strncmp(to_check.fdb.name,filename,128)){
                        printf("createFile: File already exists!\n");
                        return NULL;
                    }
                }
            }
            next_block = db.header.next_block;
        }
    }

    // if I'm here FILE ('filename') doesn't exists
    // => I can create it!
    // getting first_free_block index
    int first_free_block = DiskDriver_getFreeBlock(disk_driver,disk_driver->header,first_free_block);
    if(first_free_block == -1){
        printf("ERROR: There are no free blocks!");
        return NULL;
    }

    FirstFileBlock* new_file_block = calloc(1,sizeof(FirstFileBlock)); // allocate and fill with 0
    // initializing header
    new_file_block->header.block_in_file = 0; // new file hasn't blocks
    new_file_block->header.next_block = -1; // new file doesn't have next block
    new_file_block->header.previous_block = -1; // new file doesnt'have previous block

    // insert in directory
    new_file_block->fcb.directory_block = fdb->fcb.block_in_disk; // first_block in parent directory
    new_file_block->fcb.block_in_disk = first_free_block; // saving my position in disk
    new_file_block->fcb.is_dir = 0; // is a file (==0), not a directory (==-1)
    strncpy(new_file_block->fcb.name,filename,128); // passing filename to fcb.name

    ret = DiskDriver_writeBlock(disk_driver,new_file,first_free_block); // write file on disk in position new_block
    if(ret == -1){
        printf("ERROR: cannot create file, problem in write_block!\n");
        return NULL;
    }

    int i = 0;
    int found = 0; // 1 if I found space in an already created block
    int entry = 0; // index of the file_block space found
    int where_i_found_space = 0; // 0 space on FDB, 1 space on DB
    int where_i_save = 0; // 0 saves on FDB, 1 saves on DB
    int block_position = 0; // used to save index of block in directory
    int block_number = fdb->fcb.block_in_disk; // used to save current block in disk
    int max_free_space_in_fdb = (BLOCK_SIZE-sizeof(BlockHeader)-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
    int max_free_space_in_db = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
    DirectoryBlock directory_block;

    if(fdb->num_entries < max_free_space_in_fdb){ // check if there's enough space in FirstDirectoryBlock
        int* blocks = fdb->file_blocks;
        for(i = 0; i < max_free_space_in_fdb; i++){
            if(blocks[i] == 0){
                found = 1;
                entry = i;
                break;
            }
        }
    }else{
        where_i_found_space = 1; // i'm in DB now
        // if there isn't space in FirstDirectoryBlock
        // check in DirectoryBlock
        next_block = fdb->header.next_block;

        while(next_block != -1 && found == 0){ // while there's a next block, and while I've not found a free space
            ret = DiskDriver_readBlock(disk_driver,directory_block,next_block); // reading each block
            if(ret == -1){
                printf("createFile: error in readBlock\n");
                DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);
                return NULL;
            }

            blocks = directory_block.file_blocks;
            block_position += 1; // change block => update variables
            block_number = next_block;
            for(i = 0; i < max_free_space_in_db; i++){ // looping to find free space
                if(blocks[i] == 0){
                    found = 1;
                    entry = i;
                    break;
                }
            }
            where_i_save = 1;
            next_block = db.header.next_block;

        }

    }

    if(found == 0){ // if all blocks are full, i've to create a new DirectoryBlock
        DirectoryBlock new_db = {0};
        new_db.header.next_block = -1;
        new_db.header.block_in_file = block;
        new_db.header.previous_block = block_number;
        new_db.file_blocks[0] = new_file_block; // populating first block, with the new file created

        int new_dir_block = DiskDriver_getFreeBlock(disk_driver, disk->header->first_free_block);
        if(new_dir_block == -1){
            printf("ERROR: cannot create new directory! No free blocks\n");
            DiskDriver_freeBlock(disk_driver,fdb->fcb.block_in_disk);
            return NULL;
        }

        ret = DiskDriver_writeBlock(disk_driver,&new_db,new_dir_block); // write directory block on disk
        if(ret == -1){
            printf("ERROR: cannot write directory on disk!\n");
            DiskDriver_freeBlock(disk_driver,fdb->fcb.block_in_disk);
            return NULL;
        }

        if(where_i_save == 0){ // save on FDB
            fdb->header.next_block = new_dir_block;
        }else // save on DB
            directory_block.next_block = new_dir_block;
        directory_block = new_db;
        block_number = new_dir_block;
    }

    if(where_i_found_space == 0){ // space in FDB
        fdb->num_entries++;
        fdb->file_block[entry] = new_file_block;
        DiskDriver_freeBlock(disk_driver,fdb->fcb.block_in_disk);
        DiskDriver_writeBlock(disk_driver,fdb,fdb->fcb.block_in_disk);
    }else{ // space in DB
        fdb->num_entries++;
        DiskDriver_freeBlock(disk_driver,fdb->fcb.block_in_disk);
        DiskDriver_writeBlock(disk_driver,fdb,fdb->fcb.block_in_disk);
        directory_block.file_blocks[entry] = new_file_block;
        DiskDriver_freeBlock(disk_driver,block_number);
        DiskDriver_writeBlock(disk_driver,&directory_block,block_number);
    }

    FileHandle* file_handle = malloc(sizeof(FileHandle));
    file_handle->sfs = d->sfs;
    file_handle->fcb = new_file_block;
    file_handle->directory = fdb;
    file_handle->pos_in_file = 0;

    return file_handle;
}

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
