/*
This is the interface to the virtual disk.
You should read it carefully but don't modify it.
Make all of your changes in main.c instead.
*/ 

#ifndef DISK_H
#define DISK_H

#define BLOCK_SIZE 4096

/*
Create a new virtual disk in the file "filename", with the given number of blocks.
Returns a pointer to a new disk object, or null on failure.
*/

struct disk * disk_open( const char *filename, int blocks );

/*
Write exactly BLOCK_SIZE bytes to a given block on the virtual disk.
"d" must be a pointer to a virtual disk, "block" is the block number,
and "data" is a pointer to the data to write.
*/

void disk_write( struct disk *d, int block, const unsigned char *data );

/*
Read exactly BLOCK_SIZE bytes from a given block on the virtual disk.
"d" must be a pointer to a virtual disk, "block" is the block number,
and "data" is a pointer to where the data will be placed.
*/

void disk_read( struct disk *d, int block, unsigned char *data );

/*
Return the number of blocks in the virtual disk.
*/

int disk_nblocks( struct disk *d );

/*
Close the virtual disk.
*/

void disk_close( struct disk *d );

#endif
