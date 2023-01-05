/* filesystem.h - Defines the MP3 File System and its utilities 
 * vim:ts=4 noexpandtab
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#define FILESYSTEM_BLOCK_SIZE             4096      //each block in filesystem is 4k bytes in size
#define DENTRY_SIZE                       64        //each dentry struct should have 64 bytes
#define MAX_FILENAME_LEN                  32        //max file name length 
#define NBR_DENTRY_RESERVED_BYTES         24

#define NBR_BOOTBLOCK_RESERVED_BYTES      52        
#define NBR_DENTRIES_IN_BOOTBLOCK        ((FILESYSTEM_BLOCK_SIZE - 12 - NBR_BOOTBLOCK_RESERVED_BYTES)/DENTRY_SIZE)    

#define MAX_NUM_DATA_BLOCKS              (FILESYSTEM_BLOCK_SIZE -4 )/4 
  
#define REGULAR_FILE_TYPE                2
#define DIRECTORY_FILE_TYPE              1
#define RTC_FILE_TYPE                    0

#define NBR_EXE_MAGIC_NBRS               4
#define EXE_MAGIC_1                    0x7F
#define EXE_MAGIC_2                    0x45
#define EXE_MAGIC_3                    0x4C
#define EXE_MAGIC_4                    0x46

#include "types.h"
//define structures used in the filesystem: dentry, boot block and inode 
// NOTE: DATA BLOCKS DO NOT NEED A STRUCTURE BECAUSE THEY CAN BE FILLED WITH ANYTHIIING (IT IS UP TO USER PROGRAM TO INTERPRET CONTENT)

/* DENTRY STRUCTURE */
typedef struct __attribute__((packed)) dentry {
    uint8_t filename[MAX_FILENAME_LEN];
    int32_t filetype; 
    int32_t inode_num;
    int8_t reserved[NBR_DENTRY_RESERVED_BYTES]; 
} dentry_t;

/* BOOT BLOCK STRUCTURE */
typedef struct __attribute__((packed)) boot_blk {
    int32_t nbr_dir_entries;
    int32_t nbr_inodes;
    int32_t nbr_data_blocks;
    int8_t reserved[NBR_BOOTBLOCK_RESERVED_BYTES]; 
    dentry_t dir_entries[NBR_DENTRIES_IN_BOOTBLOCK];
} boot_blk_t;

/* INODE STRUCTURE */
typedef struct __attribute__((packed)) inode_blk {
    int32_t file_len_inB; 
    int32_t data_block_num[MAX_NUM_DATA_BLOCKS];
} inode_blk_t;

/* this funciton initializes the file system structures*/ 
void filesystem_init(int32_t* filesystem_base_addr);

/* necessary functions for the KERNEL to interface (so far, only read!) with the filesystem  */ 

/* reads the dentry corresponding to fname provided (if valid) to dentry struct passed as argument */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
/* reads the dentry at an index (if valid) of dentries arr located in boot lock to dentry struct passed as argument */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
/* reading up to length bytes starting from position offset in the file with inode number inode and 
 *   returning the number of bytes read and placed in the buffer */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


/* regular file functions */ 

/* opens a regular file based on file name provided */
int32_t open_file(const uint8_t* fname);
/* closes a regular file based on file descriptor, undoes what is in the open function */
int32_t close_file (int32_t fd);
/* read nbytes of data from file into the buf*/
int32_t read_file(int32_t fd, void* buf, int32_t nbytes);
/* write to file: should do nothing (for now)*/
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes);


/* directory file functions*/ 

/* opens a directory file based on directory file name provided */
int32_t open_dir(const uint8_t* fname);
/* closes a directory file based on file descriptor, undoes what is in the open function */
int32_t close_dir(int32_t fd);
/* cannot write to directory ==> does nothing*/
int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes);
/* read files containted in directory filename by filename, including “.”  */
int32_t read_dir(int32_t fd, void* buf, int32_t nbytes);

//EXTRA FUNCTIONS (FOR TESTING PURPOSES)
/* get number of valid dentries currently present in filesystem ==> nbr_dir_entries of boot block */
int32_t get_num_dentries_present(void);
/* get filetype corresponding to a dentry specified by index to dentries arr of boot block */
int32_t get_file_type_bydentry_index(int32_t dentry_i);
/* get filesize corresponding to a dentry specified by index to dentries arr of boot block */ 
int32_t get_file_size_bydentry_index(int32_t dentry_i);
/* get filesize corresponding to an inode specified by inode_number = index to arr of inodes */ 
int32_t get_file_size_byinode_num (int32_t inode_num);
/* get filetype corresponding to an inode specified by inode_number = index to arr of inodes */
int32_t get_file_type_byinode_num (int32_t inode_number);
/* FOR EXECUTE SYSTEM CALL: check if a requested file (by filename) is valid: present and executable */ 
int8_t is_file_executable(uint8_t* filename); 

#endif /* FILESYSTEM_H */
