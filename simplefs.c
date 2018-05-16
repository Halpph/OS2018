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
                    if(strncmp(to_check.fdb.name,filename,128) == 0){
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
    int new_block = DiskDriver_getFreeBlock(disk, disk->header->first_free_block); // search the first free block
	if (new_block == -1){
		printf("Impossible to create file: impossible to find free block\n");
		return NULL;
	}

	FirstFileBlock* new_file = calloc(1, sizeof(FirstFileBlock)); // calloc puts all 0 on the file
	new_file->header.block_in_file = 0;
	new_file->header.next_block = -1;
	new_file->header.previous_block = -1;

	new_file->fcb.directory_block = fdb->fcb.block_in_disk; // // first block of the parent directory
	new_file->fcb.block_in_disk = new_block; // position in disk
	new_file->fcb.is_dir = 0; // '0' is a file, '1' is a directory
	new_file->fcb.written_bytes = 0;
	strncpy(new_file->fcb.name, filename, MAX_NAME_LEN); // copying filename on fcb.name

	ret = DiskDriver_writeBlock(disk, new_file, new_block);	// write file on disk
	if (ret == -1){
		printf("Impossible to create file: problem on writeBlock to write file on disk\n");
		return NULL;
	}


	int i = 0;
	int found = 0;	// '0' free space not found, '1' free space found
	int block_number = fdb->fcb.block_in_disk;	//number of the current block inside the disk
	DirectoryBlock db_last;	//if there's no space in FirstDirectoryBlock I will use this reference

    int entry = 0;	// when a free space is found, I save the number of the entry in file_blocks of the directoryBlock / firstDirectoryBlock
    int blockInFile = 0; // number of the block in the directory
    int fdb_or_db_save = 0; // '0' save the next block after fdb, '1' after db_last
    int fdb_or_max_free_space_db = 0;	//'0' there is space in firstDirectoryBlock, '1' space in another block

    int max_free_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
    int max_free_space_db = (BLOCK_SIZE - sizeof(BlockHeader))/sizeof(int);

    if (fdb->num_entries < max_free_space_fdb){	//check if there's free space in FirstDirectoryBlock
        int* blocks = fdb->file_blocks;
        for(i=0; i<max_free_space_db; i++){	// loop to find the position
            if (blocks[i] == 0){ // free space in fdb->blocks[i]
                found = 1;
                entry = i;
                break;
            }
        }
    } else{	// in case of not space in FirstDirectoryBlock, now loop in DirectoryBlocks
        fdb_or_max_free_space_db = 1;
        int next = fdb->header.next_block;

        while (next != -1 && !found){	// while I've another block, and while I've not found a free space
            ret = DiskDriver_readBlock(disk, &db_last, next); // read new directory block
            if (ret == -1){
                printf("Impossible to create file: problem on readBlock to read directory and change status\n");
                DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// to complete operations
                return NULL;
            }
            int* blocks = db_last.file_blocks;
            blockInFile++;	//switch block => update variables states
            block_number = next;
            for(i = 0; i < max_free_space_db; i++){	// loop to find the position
                if (blocks[i] == 0){
                    found = 1;
                    entry = i;
                    break;
                }

            }
            fdb_or_db_save = 1;
            next = db_last.header.next_block;
        }
    }

    if (!found){	// in case there's no free space
		DirectoryBlock new_db = {0}; // create new directoryBlock and populate heder
		new_db.header.next_block = -1;
		new_db.header.block_in_file = blockInFile;
		new_db.header.previous_block = block_number;
		new_db.file_blocks[0] = new_block;	// save the FirstFileBlock of the file just created

		int new_dir_block = DiskDriver_getFreeBlock(disk, disk->header->first_free_block); // index of a free block for the directoryBlock in the disk
		if (new_block == -1){
			printf("Impossible to create file: impossible to find free block to create a new block for directory\n");
			DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// to complete operations
			return NULL;
		}

		ret = DiskDriver_writeBlock(disk, &new_db, new_dir_block); // write block on the disk
		if (ret == -1){
				printf("Impossible to create file: problem on writeBlock to write file on disk\n");
				DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// to complete operations
				return NULL;
		}

		if (fdb_or_db_save == 0){
			fdb->header.next_block = new_dir_block;	// update header current block
		} else{
			db_last.header.next_block = new_dir_block;
		}
		db_last = new_db;
		block_number = new_dir_block;

	}

	if (fdb_or_max_free_space_db == 0){	// there's space in FirstDirectoryBlock
		fdb->num_entries++;
		fdb->file_blocks[entry] = new_block; // save new_block position in FirstDirectoryBlock
        DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(disk, fdb, fdb->fcb.block_in_disk);
	} else{ // there's space in DirectoryBlock
		fdb->num_entries++;
        DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(disk, fdb, fdb->fcb.block_in_disk);
		db_last.file_blocks[entry] = new_block;
        DiskDriver_freeBlock(disk,block_number);
		DiskDriver_writeBlock(disk, &db_last, block_number);
	}

	FileHandle* fh = malloc(sizeof(FileHandle)); // create and populate handle to return
	fh->sfs = d->sfs;
	fh->fcb = new_file;
	fh->directory = fdb;
	fh->pos_in_file = 0;

	return fh;
}

// reads in the (preallocated) blocks array, the name of all files in a directory
int SimpleFS_readDir(char** names, DirectoryHandle* d){
	if (d == NULL || names == NULL)
		ERROR_HELPER(-1, "Impossible to read directory: Bad Parameters\n");

	int ret = 0, num_tot = 0;
	FirstDirectoryBlock *fdb = d->dcb;
	DiskDriver* disk = d->sfs->disk;

    int max_free_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
    int max_free_space_db = (BLOCK_SIZE - sizeof(BlockHeader))/sizeof(int);

	if (fdb->num_entries > 0){	// directory is not empty
		int i;
		FirstFileBlock to_check; // used as a reference to save the current file that is currently checked

		int* blocks = fdb->file_blocks;
		for (i = 0; i < max_free_space_fdb; i++){	// checks every block entry in the directory FirstDirectoryBlock
			if (blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, blocks[i]) != -1){ // blocks[i] > 0 => to_check not empty, read to check the name
				names[num_tot] = strndup(to_check.fcb.name, MAX_NAME_LEN); // save the name in the buffer
                num_tot++;
			}
		}

		if (fdb->num_entries > i){	// check if more entries in other blokcs of the directory
			int next = fdb->header.next_block;
			DirectoryBlock db;

			while (next != -1){	 // check all blocks (next == -1 => no more block to be checked)
				ret = DiskDriver_readBlock(disk, &db, next); // read new directory block
				if (ret == -1){
					printf("Impossible to read all direcory, problems on next block\n");
					return -1;
				}

				int* blocks = db.file_blocks;
				for (i = 0; i < max_free_space_db; i++){	 // checks every block indicator in the directory FirstDirectoryBlock
					if (blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, blocks[i]) != -1){ // blocks[i] > 0 => to_check not empty, read to check the name
						names[num_tot] = strndup(to_check.fcb.name, MAX_NAME_LEN); // save the name in the buffer
                        num_tot++;
					}
				}

				next = db.header.next_block; // goto next directoryBlock
			}
		}
	}
	return 0;
}

// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	if (d == NULL || filename == NULL)
		ERROR_HELPER(-1, "Impossible to open file: Bad Parameters\n");

	int ret = 0;
	FirstDirectoryBlock *fdb = d->dcb;
	DiskDriver* disk = d->sfs->disk;

	if (fdb->num_entries > 0){	// directory is not empty
		FileHandle* fh = malloc(sizeof(FileHandle)); // create and populate handle to return
		fh->sfs = d->sfs;
		fh->directory = fdb;
		fh->pos_in_file = 0;

		int found = 0;	// '0' file found, '1' file not found
		FirstFileBlock* to_check = malloc(sizeof(FirstFileBlock));

        // checking file esistence in FirstDirectoryBlock
        int i,pos;
        for(i = 0; i < entries; i++){
            if(fdb->file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,to_check,fdb->file_blocks[i]) != -1)){ //check if block is free
                if(strncmp(to_check.fdb.name,filename,128) == 0){
                    found = 1;
                    pos = i;
    				DiskDriver_readBlock(disk, to_check, fdb->file_blocks[i]);	//read again the correct file to complete handler
    				fh->fcb = to_check;
                    break;
                }
            }
        }

        // checking file esistence in DirectoryBlock
		int next = fdb->header.next_block;
		DirectoryBlock db;

		while (next != -1 && !found){	// while I've next block and while file is not found
			ret = DiskDriver_readBlock(disk, &db, next);											//read new directory block
			if (ret == -1){
				printf("Impossible to create file: problem on readBlock to read next block and find already made same file\n");
				return NULL;
			}

            for(i = 0; i < entries; i++){
                if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,to_check,db.file_blocks[i]) != -1)){ //check if block is free
                    if(strncmp(to_check.db.name,filename,128) == 0){
                        found = 1;
                        pos = i;
        				DiskDriver_readBlock(disk, to_check, db.file_blocks[i]);	// read again the correct file to complete handler
        				fh->fcb = to_check;
                        break;
                    }
                }
            }

			next = db.header.next_block; // goto next directoryBlock
		}

		if (found){
			return fh;
		} else {
			printf("Impossible to open file: file doesn't exist\n");
			free(fh);
			return NULL;
		}

	} else { //if 0 entries => directory is empty
		printf("Impossible to open file: direcoty is empty\n");
		return NULL;
	}
}

// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
	free(f->fcb);
	free(f);
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	FirstFileBlock* ffb = f->fcb;

	int written_bytes = 0;
	int to_write = size;
	int off = f->pos_in_file;
    int max_free_space_ffb = BLOCK_SIZE - sizeof(FileControlBlock) - sizeof(BlockHeader)
    int max_free_space_fb = BLOCK_SIZE - sizeof(BlockHeader)

	if(off < max_free_space_ffb && to_write <= max_free_space_ffb-off){ // If byte to write are smaller or equal of space available
		memcpy(ffb->data+off, (char*)data, to_write);	// write the bytes
		written_bytes += to_write;
		if(f->pos_in_file+written_bytes > ffb->fcb.written_bytes)
			ffb->fcb.written_bytes = f->pos_in_file+written_bytes;
        // update the block
        DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
    	DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
		return written_bytes;
	}
	else if(off < max_free_space_ffb && to_write > max_free_space_ffb-off){ // If byte to write are greater of space available
		memcpy(ffb->data+off, (char*)data, max_free_space_ffb-off);
		written_bytes += max_free_space_ffb-off;
		to_write = size - written_bytes;
        DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
    	DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
		off = 0;
	}
	else off -= max_free_space_ffb;

	int block_in_disk = ffb->fcb.block_in_disk;	 // index on disk of current block
	int next_block = ffb->header.next_block; // index on disk of next block
	int block_in_file = ffb->header.block_in_file; //index on file of current block
	FileBlock fb_tmp; // aux that will store the new file created to contain all data*
	int one_block = 0;
	if(next_block == -1) one_block = 1;

	while(written_bytes < size){
		if(next_block == -1){ // allocate new block if there's no space for data
			FileBlock n_fb = {0};
			n_fb.header.block_in_file = block_in_file+1;
			n_fb.header.next_block = -1;
			n_fb.header.previous_block = block_in_disk;

			next_block = DiskDriver_getFreeBlock(f->sfs->disk, block_in_disk); // update index of next_block
			if(one_block == 1){	// we are on FirstFileBlock
				ffb->header.next_block = next_block;
                // update changes on disk
                DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
            	DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
				one_block = 0;
			}
			else{
				fb_tmp.header.next_block = next_block; // update index nextblock of prevoius fileblock
                // update disk
                DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
            	DiskDriver_writeBlock(f->sfs->disk, &fb_tmp, ffb->fcb.block_in_disk);
			}
			DiskDriver_writeBlock(f->sfs->disk, &n_fb, next_block); // write changes on disk

			fb_tmp = n_fb;
		}
		else
			if(DiskDriver_readBlock(f->sfs->disk, &fb_tmp, next_block) == -1) return -1;

		if(off < max_free_space_fb && to_write <= max_free_space_fb-off){	 // if byte to be written are smaller of space available
			memcpy(fb_tmp.data+off, (char*)data+written_bytes, to_write); // then we write the last bytes and exit from cycle
			written_bytes += to_write;	//update changes
			if(f->pos_in_file+written_bytes > ffb->fcb.written_bytes)
				ffb->fcb.written_bytes = f->pos_in_file+written_bytes;
            DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
        	DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
            DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
        	DiskDriver_writeBlock(f->sfs->disk, &fb_tmp, next_block);
			return written_bytes;
		}
		else if(off < max_free_space_fb && to_write > max_free_space_fb-off){ // if byte to be written are greater of space available
			memcpy(fb_tmp.data+off, (char*)data+written_bytes, max_free_space_fb-off); // then we write the last bytes and exit from cycle
			written_bytes += max_free_space_fb-off;	// update changes
			to_write = size - written_bytes;
            DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
        	DiskDriver_writeBlock(f->sfs->disk, &fb_tmp, next_block);
			off = 0;
		}
		else off-=max_free_space_fb;

		block_in_disk = next_block;	// update index of current_block
		next_block = fb_tmp.header.next_block;
		block_in_file = fb_tmp.header.block_in_file; // update index of next_block
	}

	return written_bytes;
}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){
	FirstFileBlock* ffb = f->fcb;

	int off = f->pos_in_file; // position in file
	int written_bytes = ffb->fcb.written_bytes;	// bytes written in file

	if(size+off > written_bytes){ // invalid size
		printf("INVALID SIZE: choose a smaller size\n");
		bzero(data, size);
		return -1;
	}

	int bytes_read = 0;
	int to_read = size;
    int max_free_space_ffb = BLOCK_SIZE - sizeof(FileControlBlock) - sizeof(BlockHeader)
    int max_free_space_fb = BLOCK_SIZE - sizeof(BlockHeader)

	if(off < max_free_space_ffb && to_read <= max_free_space_ffb-off){ // If byte to be read are smaller or equal of space available
		memcpy(data, ffb->data+off, to_read); // read the bytes
		bytes_read += to_read;
		to_read = size - bytes_read;
		f->pos_in_file += bytes_read;
		return bytes_read;
	}
	else if(off < max_free_space_ffb && to_read > max_free_space_ffb-off){ // If byte to be read are greater of space available
		memcpy(data, ffb->data+off, max_free_space_ffb-off);
		bytes_read += max_free_space_ffb-off;
		to_read = size - bytes_read;
		off = 0;
	}
	else off -= max_free_space_ffb;

	int next_block = ffb->header.next_block; // index on disk of next block
	FileBlock fb_tmp;	// aux

	while(bytes_read < size && next_block != -1){ // reading all the blocks till bytes_read < size
		if(DiskDriver_readBlock(f->sfs->disk, &fb_tmp, next_block) == -1) return -1;

		if(off < max_free_space_fb && to_read <= max_free_space_fb-off){	// if byte to be read are smaller of space available
			memcpy(data+bytes_read, fb_tmp.data+off, to_read);	// read the bytes and terminate
			bytes_read += to_read;
			to_read = size - bytes_read;
			f->pos_in_file += bytes_read;
			return bytes_read;
		}
		else if(off < max_free_space_fb && to_read > max_free_space_fb-off){ // else read bytes and continue
			memcpy(data+bytes_read, fb_tmp.data+off, max_free_space_fb-off);
			bytes_read += max_free_space_fb-off;
			to_read = size - bytes_read;
			off = 0;
		}
		else off -= max_free_space_fb;

		next_block = fb_tmp.header.next_block;
	}

	return bytes_read;
}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){
	FirstFileBlock* ffb = f->fcb;

	if(pos > ffb->fcb.written_bytes){ // pos is invalid
		printf("INVALID POS: choose a smaller pos\n");
		return -1;
	}

	f->pos_in_file = pos;
	return pos;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){
	if (d == NULL || dirname == NULL)
		ERROR_HELPER(-1, "Impossible to change dir: Bad Parameters\n");

	int ret = 0;

	if (!strncmp(dirname, "..", 2)){ // go one level up
		if (d->dcb->fcb.block_in_disk == 0){ // check if i'm in root
			printf("Impossible to read parent directory, this is root directory\n");
			return -1;
		}
		d->pos_in_block = 0; // reset directory
		d->dcb = d->directory; 	// this directory become the parent directory

        // search parent directory
		int parent_block =  d->dcb->fcb.directory_block; // save first block of parent directory
		if (parent_block == -1){ // first block of parent directory is root directory
			d->directory = NULL;
			return 0;
		}
		FirstDirectoryBlock* parent = malloc(sizeof(FirstDirectoryBlock));
		ret = DiskDriver_readBlock(d->sfs->disk, parent, parent_block);	 // read the parent directory to save it
		if (ret == -1){
			printf("Impossible to read parent directory during go level up ('cd ..')\n");
			d->directory = NULL; // problems to read, handle will nor have the parent
		} else{
			d->directory = parent; // save the correct parent
		}
		return 0;
	} else if (d->dcb->num_entries < 0){ // directory is empty
		printf("Impossible to change directory, this directory is empty\n");
		return -1;
	}

    // if not go level up and directory is not empty
	FirstDirectoryBlock *fdb = d->dcb;
	DiskDriver* disk = d->sfs->disk;

	FirstDirectoryBlock* to_check = malloc(sizeof(FirstDirectoryBlock));

    for (i = 0; i < max_free_space_fdb; i++){ //checks every block indicator in the directory block
       if (fdb->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, fdb->file_blocks[i]) != -1){  //read the FirstFileBlock of the file to check the name (if to_check block is not empty, block[i]>0)
           if (!strncmp(to_check.fcb.name, dirname, 128)){ // string compare name strings, return 0 if s1 == s2
                DiskDriver_readBlock(disk, to_check, fdb->file_blocks[i]); // read again the correct directory to save it
                d->pos_in_block = 0; // reset directory
           		d->directory = fdb;	//parent directory become this directory
           		d->dcb = to_check;  // this directory become the read directory
           		return 0;
           }
       }
   }

   int next = fdb->header.next_block;
   DirectoryBlock db;

   while (next != -1){	// to check in other bocks
        ret = DiskDriver_readBlock(disk, &db, next); // read new directory block
        if (ret == -1){
            printf("Impossible to read all direcory, problems on next block\n");
            return -1;
        }

        for (i = 0; i < max_free_space_db; i++){ // checks every block indicator in the directory block
            if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, db->file_blocks[i]) != -1){  // read the FirstFileBlock of the file to check the name (if to_check block is not empty, block[i]>0)
               if (!strncmp(to_check.fcb.name, dirname, 128)){ // string compare name strings, return 0 if s1 == s2
                    DiskDriver_readBlock(disk, to_check, db->file_blocks[i]); // read again the correct directory to save it
                    d->pos_in_block = 0; // reset directory
               		d->directory = fdb;	// parent directory become this directory
               		d->dcb = to_check;  // this directory become the read directory
               		return 0;
               }
            }
        }
        next = db.header.next_block; // goto next directoryBlock
   }

   printf("Impossible to change directory, it doesn't exist\n");
   return -1;
}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	if (d == NULL || dirname == NULL)
		ERROR_HELPER(-1, "Impossible to create directory: Bad Parameters\n");

	int ret = 0;
	FirstDirectoryBlock *fdb = d->dcb;	// save pointers, used for performance
	DiskDriver* disk = d->sfs->disk;

	if (fdb->num_entries > 0){	// directory not empty, needs to check if dir with dirname already exist
        for (i = 0; i < max_free_space_fdb; i++){ // checks every block indicator in the directory block
            if (fdb->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, fdb->file_blocks[i]) != -1){  // read the FirstFileBlock of the file to check the name (if to_check block is not empty, block[i]>0)
               if (strncmp(to_check.fcb.name, dirname, 128) == 0){ // string compare name strings, return 0 if s1 == s2
                   printf("Impossible to create directory: file already exist\n");
               	   return -1;
               }
            }
        }

		int next = fdb->header.next_block;
		DirectoryBlock db;

		while (next != -1){	// to check if dir is in another block of the directory
			ret = DiskDriver_readBlock(disk, &db, next); // read new directory block
			if (ret == -1){
				printf("Impossible to create directory: problem on readBlock to read next block and find already made same file\n");
				return -1;
			}
            for (i = 0; i < max_free_space_db; i++){ // checks every block indicator in the directory block
                if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &to_check, db->file_blocks[i]) != -1){  // read the FirstFileBlock of the file to check the name (if to_check block is not empty, block[i]>0)
                   if (strncmp(to_check.fcb.name, dirname, 128)){ // string compare name strings, return 0 if s1 == s2
                       printf("Impossible to create directory: file already exist\n");
                   	   return -1;
                   }
                }
            }

			next = db.header.next_block; // goto next directoryBlock
		}
	}

	int new_block = DiskDriver_getFreeBlock(disk, disk->header->first_free_block); // search a free block in the disk for the directory
	if (new_block == -1){
		printf("Impossible to create directory: impossible to find free block\n%s");
		return -1;
	}
	// create new directory
	FirstDirectoryBlock * new_directory = calloc(1, sizeof(FirstFileBlock)); //calloc fills buffer with 0 on the directory
	new_directory->header.block_in_file = 0; // populate header of the directory
	new_directory->header.next_block = -1;
	new_directory->header.previous_block = -1;

	new_directory->fcb.directory_block = fdb->fcb.block_in_disk; // populate FileControlBlock
	new_directory->fcb.block_in_disk = new_block;
	new_directory->fcb.is_dir = 1;
	strcpy(new_directory->fcb.name, dirname);

	ret = DiskDriver_writeBlock(disk, new_directory, new_block); // write directory on disk
	if (ret == -1){
		printf("Impossible to create directory: problem on writeBlock to write directory on disk\n%s");
		return -1;
	}

	int i = 0;
	int found = 0;	// to check if there is space in already created blocks of the directory, '1' ok | '0' no space
	int block_number = fdb->fcb.block_in_disk; // number of the current block inside the disk
	DirectoryBlock db_last;	 // in case of no space in firstDirectoryBlock this will save the current block

	int entry = 0;	// to save the number of the entry in file_blocks of the directoryBlock / firstDirectoryBlock
	int blockInFile = 0; // indicate the number of the block inside the file (directory)
	int fdb_or_db_save = 0;	 //if 0 save the next block after fdb, if 1 after db_last
    int fdb_or_max_free_space_db = 0; // if 0 there is space in firstDirectoryBlock, if 1 space in another block

	if (fdb->num_entries < max_free_space_db){	 // check if free space in FirstDirectoryBlock (implicit if num_entry == 0)
		int* blocks = fdb->file_blocks;
		for(i=0; i<max_free_space_db; i++){  // loop to find the position
			if (blocks[i] == 0){ // free space in fdb->blocks[i]
				found = 1;
				entry = i;
				break;
			}
		}
	} else{	// there's space in firstDirectoryBlock => check in directoryBlock
		fdb_or_max_free_space_db = 1;
		int next = fdb->header.next_block;

		while (next != -1 && !found){ // loop to find position in another DirectoryBlock
			DiskDriver_readBlock(disk, &db_last, next); // read a new directory block
			if (ret == -1){
				printf("Impossible to create directory: problem on readBlock to read directory and change status\n");
				DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// needs to free the block already written because it's imposssible to complete operations
				return -1;
			}
			int* blocks = db_last.file_blocks;
			blockInFile++;
			block_number = next;
			for(i=0; i<max_free_space_db; i++){	// loop to find a free space
				if (blocks[i] == 0){ // block[i] == 0 ==> free block found
					found = 1;
					entry = i;
					break;
				}
			}
			fdb_or_db_save = 1;
			next = db_last.header.next_block;
		}
	}

	if (!found){ // in case there's no space in any block
		DirectoryBlock new_db = {0}; // create new directoryBlock
		new_db.header.next_block = -1;
		new_db.header.block_in_file = blockInFile;
		new_db.header.previous_block = block_number;
		new_db.file_blocks[0] = new_block;

		int new_dir_block = DiskDriver_getFreeBlock(disk, disk->header->first_free_block); // search a free block in the disk
		if (new_block == -1){
			printf("Impossible to create directory: impossible to find free block to create a new block for directory\n");
			DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// to complete operations
			return -1;
		}

		ret = DiskDriver_writeBlock(disk, &new_db, new_dir_block); // write block on the disk
		if (ret == -1){
			printf("Impossible to create directory: problem on writeBlock to write directory on disk\n");
			DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);	// to complete operations
			return -1;
		}

		if (fdb_or_db_save == 0){
			fdb->header.next_block = new_dir_block; // update header of the current block
		} else{
			db_last.header.next_block = new_dir_block;
		}
		db_last = new_db;
		block_number = new_dir_block;

	}

	if (fdb_or_max_free_space_db == 0){	// space in firstDirectoryBlock
		fdb->num_entries++;
		fdb->file_blocks[entry] = new_block; // save new_block position in firstDirectoryBlock
        // update (free then write)
        DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(disk, fdb, fdb->fcb.block_in_disk);
	} else{
		fdb->num_entries++;
        // update (free then write)
        DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(disk, fdb, fdb->fcb.block_in_disk);
		db_last.file_blocks[entry] = new_block;
        DiskDriver_freeBlock(disk, block_number);
		DiskDriver_writeBlock(disk, &db_last, block_number);
	}

	return 0;
}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(DirectoryHandle* d, char* filename){
	if (d == NULL || filename == NULL)
		ERROR_HELPER(-1, "Impossible to remove directory: Bad Parameters\n");

	FirstDirectoryBlock* fdb = d->dcb;

    FirstFileBlock to_check;
    int i,id = -1;
    for(i = 0; i < max_free_space_fdb; i++){
        if(fdb->file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,&to_check,fdb->file_blocks[i]) != -1)){ //check if block is free
            if(strncmp(to_check.fdb.name,filename,128) == 0){
                id = i;
                printf("createFile: File already exists!\n");
                return NULL;
            }
        }
    }

	int first = 1;

	DirectoryBlock* db_tmp = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
	int next_block = fdb->header.next_block; // variable with next block of fdb
	int block_in_disk = fdb->fcb.block_in_disk;

	while(id == -1){ // if file isn't in fdb contiunue looping in DirectoryBlock
		if(next_block != -1){
			first = 0;	// on the other blocks of current directory
			if(DiskDriver_readBlock(d->sfs->disk, db_tmp, next_block) == -1) return -1;
            for(i = 0; i < max_free_space_db; i++){
                if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk_driver,&to_check,db->file_blocks[i]) != -1)){ //check if block is free
                    if(strncmp(to_check.fdb.name,filename,128) == 0){
                        id = i;
                        printf("createFile: File already exists!\n");
                        return NULL;
                    }
                }
            }

			block_in_disk = next_block;
			next_block = db_tmp->header.next_block;
		}
		else{ // if blocks of current directory are terminated there is an error
			printf("INVALID FILENAME: file not existent or is a directory\n");
			return -1;
		}
	}

	int idf;
	int ret;
	if(first == 0) idf = db_tmp->file_blocks[id];
	else idf = fdb->file_blocks[id];

	FirstFileBlock ffb_rm; // ffb to remove
	if(DiskDriver_readBlock(d->sfs->disk, &ffb_rm, idf) == -1) return -1;
	if(ffb_rm.fcb.is_dir == 0){	// if filename is a file, we free its block (is_dir == 0 => is a file, is_dir == -1 => is a directory)
		FileBlock fb_tmp;
		int next = ffb_rm.header.next_block;
		int block_in_disk = idf;
		while(next != -1){
			if(DiskDriver_readBlock(d->sfs->disk, &fb_tmp, next) == -1) return -1;
			block_in_disk = next;
			next = fb_tmp.header.next_block;
			DiskDriver_freeBlock(d->sfs->disk, block_in_disk);
		}
		DiskDriver_freeBlock(d->sfs->disk, idf);
		d->dcb = fdb;
		ret = 0;
	}
	else{ // else if is a directory we recursively free all contained blocks
		FirstDirectoryBlock fdb_rm;
		if(DiskDriver_readBlock(d->sfs->disk, &fdb_rm, idf) == -1) return -1;
		if(fdb_rm.num_entries > 0){ // if directory is not empty
			if(SimpleFS_changeDir(d, fdb_rm.fcb.name) == -1) return -1;
			int i;
			for(i = 0; i < max_free_space_db; i++){
				FirstFileBlock ffb;
				if(fdb_rm.file_blocks[i] > 0 && DiskDriver_readBlock(d->sfs->disk, &ffb, fdb_rm.file_blocks[i]) != -1)
					SimpleFS_remove(d, ffb.fcb.name); // recursive remove
			}
			int next = fdb_rm.header.next_block;
			int block_in_disk = idf;
			DirectoryBlock db_tmp;
			while(next != -1){ // loop in all directory blocks
				if(DiskDriver_readBlock(d->sfs->disk, &db_tmp, next) == -1) return -1;
				int j;
				for(j = 0; j < max_free_space_db; j++){
					FirstFileBlock ffb;
					if(DiskDriver_readBlock(d->sfs->disk, &ffb, db_tmp.file_blocks[i]) == -1) return -1;
					SimpleFS_remove(d, ffb.fcb.name);
				}
				block_in_disk = next;
				next = db_tmp.header.next_block;
				DiskDriver_FreeBlock(d->sfs->disk, block_in_disk);
			}
			DiskDriver_freeBlock(d->sfs->disk, idf);
			d->dcb = fdb;
			ret = 0;
		}
		else{
			DiskDriver_freeBlock(d->sfs->disk, idf);
			d->dcb = fdb;
			ret = 0;
		}
	}

	if(first == 0){
		db_tmp->file_blocks[id] = -1;
		fdb->num_entries-=1;
        // update blocks
        DiskDriver_freeBlock(d->sfs->disk, block_in_disk);
		DiskDriver_writeBlock(d->sfs->disk, db_tmp, block_in_disk);
        DiskDriver_freeBlock(d->sfs->disk, fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->fcb.block_in_disk);
		return ret;
	}
	else{
		fdb->file_blocks[id] = -1;
		fdb->num_entries-=1;
        // update blocks
        DiskDriver_freeBlock(d->sfs->disk, fdb->fcb.block_in_disk);
		DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->fcb.block_in_disk);
		return ret;
	}

	return -1;
}
