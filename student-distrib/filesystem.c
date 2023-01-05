/* filesystem.c - Defines the MP3 File System and its utilities 
 */

#include "lib.h"
#include "filesystem.h"
#include "syscall_handlers.h"
#include "terminal.h"

//define pointers that map filesystem structs to their corresponding memory location in the given filesystem memory region
boot_blk_t* boot_blk_addr;  //address of boot block (first block in filesystem memory)
dentry_t* dentry_arr_addr ; 
inode_blk_t* inodes_arr_addr;  //address of first inode in memory
uint8_t* datablks_arr_addr; //address of first byte of first data block

uint8_t exe_magic_nbrs[NBR_EXE_MAGIC_NBRS] = {EXE_MAGIC_1, EXE_MAGIC_2, EXE_MAGIC_3, EXE_MAGIC_4}; // ELF magic number found in first 4 bytes of exe file

/*
 * filesystem_init
 *   DESCRIPTION: initializes the file system structures (set pointers to correct memory addresses of inside file system allocated memory)
 *   INPUTS: filesystem_base_addr: starting address of pre_filled memory space for filesystem structs
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: none
 */
void filesystem_init(int32_t* filesystem_base_addr){
 boot_blk_addr = (boot_blk_t*)(*filesystem_base_addr) ; // fs base address is retrieved and defined as var in kernel.c
 dentry_arr_addr = (dentry_t*)(boot_blk_addr-> dir_entries); // index 0 of dentries array of boot block
 inodes_arr_addr = (inode_blk_t*)(boot_blk_addr + 1); // inode is the second block in filesystem memory 
 datablks_arr_addr = (uint8_t*)(inodes_arr_addr + boot_blk_addr->nbr_inodes);  // first data block comes after nbr_inodes 
}



/* necessary functions for the KERNEL to interface (so far, only read!) with the filesystem  */ 

/*
 * read_dentry_by_name
 *   DESCRIPTION: reads the dentry corresponding to fname provided (if valid) to dentry struct passed as argument
 *   INPUTS: fname: pointer to file name -- dentry: pointer to dentry struct to be filled
 *   OUTPUTS: dentry: pointer to dentry struct to be filled
 *   SIDE EFFECTS: calls "read_dentry_by_index"
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){

if (fname == NULL || dentry == NULL)//invalid arguments
    return -1; 
uint32_t fname_len =  strlen((int8_t*) fname) ; 
if ( fname_len > MAX_FILENAME_LEN) //fname provided exceeds allowed filename length
     return -1;

uint32_t dentry_i; // index of dentries array in boot block 
uint32_t zero_pad_i;
uint32_t nbr_dentries_present = boot_blk_addr->nbr_dir_entries; // nbr of directory entries
uint8_t found =0;
for (dentry_i = 0; dentry_i < nbr_dentries_present; dentry_i ++){

   if( !(strncmp((int8_t*) fname, (int8_t*)dentry_arr_addr[dentry_i].filename, fname_len))){  //if fname matches with filename in dentry
   found =1;
   //check is filename is zero padded after "fname_len" of characters to confirm that fname matches with filename
   for (zero_pad_i = fname_len; zero_pad_i <MAX_FILENAME_LEN; zero_pad_i++ ){
        if (dentry_arr_addr[dentry_i].filename[zero_pad_i] != '\0'){
            found =0;
            break; 
        }
   } }
   if (found){
       if (!read_dentry_by_index(dentry_i, dentry)) 
             return 0;   // only return 0 if read was succesfull 
   }
}
return -1; //failure: filename not found in directory or no copy was successful
}

/*
 * read_dentry_by_index
 *   DESCRIPTION: reads the dentry at an index (if valid) of dentries arr located in boot lock to dentry struct passed as argument
 *   INPUTS: index: index of dentry in dentries arr located in boot block -- dentry: pointer to dentry struct to be filled
 *   OUTPUTS: dentry: pointer to dentry struct to be filled
 *   SIDE EFFECTS: none
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){
// do range checks on index    
     if( index >= NBR_DENTRIES_IN_BOOTBLOCK || index >= boot_blk_addr->nbr_dir_entries)
         return -1; 
     if( dentry == NULL ) 
         return -1; 
// do range check on inode num in dentry at index     
     if( dentry_arr_addr[index].inode_num >= boot_blk_addr->nbr_inodes )
         return -1;   

    //make a deep copy of dentry of boot block to dentry arg provided
    strncpy((int8_t*)(dentry->filename), (int8_t*)(dentry_arr_addr[index].filename), MAX_FILENAME_LEN);
    dentry->filetype = dentry_arr_addr[index].filetype;
    dentry->inode_num = dentry_arr_addr[index].inode_num;
    return 0;
}

/*
 * read_data
 *   DESCRIPTION: reading up to length bytes starting from position offset in the file with inode number inode and 
 *   returning the number of bytes read and placed in the buffer
 *   INPUTS: inode: inode number of file to be read -- offset: offset position in file to start reading from 
 *   -- buf: pointer to buffer to be filled with bytes read -- length: number of bytes requested to be read
 *   OUTPUTS: buf: pointer to buffer to be filled with bytes read
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1: fail or number of bytes read (0 means that end of file is reached or no reading is done)
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    //sanity checks
    if( inode >= boot_blk_addr->nbr_inodes || buf == NULL )// inode num exceed max inode num
            return -1; 

    int32_t nbr_bytes_read = 0; 
    int32_t file_length = inodes_arr_addr[inode].file_len_inB; //actual length of file described by inode
    
    if(offset > file_length) return 0;  //offset exceeds file length (nothing to be copied from file) 
    
    int32_t* data_blocks_inode = inodes_arr_addr[inode].data_block_num; 
    int32_t max_datablk_num = boot_blk_addr->nbr_data_blocks; // max valid data_block num
    int32_t loop_i, buf_i =0; // index of buffer
    int32_t blk_byte_off =  offset % FILESYSTEM_BLOCK_SIZE; //start copy from this byte offset of a certain block
    int32_t curr_datablk_inode_i =  (offset - blk_byte_off) / FILESYSTEM_BLOCK_SIZE ; // first data block index in inode 
    int32_t max_num_bytes_read = ((file_length - offset) > length)? length:  (file_length - offset);
   
   //start copy
    while(1){
            //do sanity checks on current data block read from 
        if (curr_datablk_inode_i >= MAX_NUM_DATA_BLOCKS) return -1; // a bad data block index is encountered
        int32_t curr_datablk_i = data_blocks_inode[curr_datablk_inode_i];
        if (curr_datablk_i >= max_datablk_num) return -1; // a bad block is encountered
        uint8_t* start_curr_datablk = datablks_arr_addr + curr_datablk_i * FILESYSTEM_BLOCK_SIZE; 

        //copy all of curr data block if allowed
        if( (nbr_bytes_read + FILESYSTEM_BLOCK_SIZE - blk_byte_off) <= max_num_bytes_read ){
            for(loop_i = blk_byte_off ; loop_i< FILESYSTEM_BLOCK_SIZE; loop_i ++){
                buf[buf_i++] =  *(start_curr_datablk + loop_i);
            } 
            nbr_bytes_read += FILESYSTEM_BLOCK_SIZE - blk_byte_off; 
            blk_byte_off =0;
            curr_datablk_inode_i++; //go to next data block in inode
        }
        else{ // we are copying from the final data block
            for(loop_i = blk_byte_off; loop_i< (max_num_bytes_read + blk_byte_off- nbr_bytes_read); loop_i++){
                buf[buf_i++] =  *(start_curr_datablk + loop_i );
            }
            nbr_bytes_read = max_num_bytes_read;  
            break; 
        }
        //loop end
    }

    return nbr_bytes_read;
}


/* regular file functions : SUBJECT TO CHANGE AFTER ADDING FILE DESCIPTORS*/ 

/*
 * open_file
 *   DESCRIPTION: opens a regular file based on file name provided
 *   INPUTS: fname: pointer file name
 *   SIDE EFFECTS: calls "read_dentry_by_name"
 *   RETURN VALUE: 0 (success) or -1(failure)
 */

int32_t open_file(const uint8_t* fname){
    dentry_t open_file_dentry; 
    if (read_dentry_by_name(fname, &open_file_dentry) == -1) //invalid read of filename
        return -1;
    if (open_file_dentry.filetype != REGULAR_FILE_TYPE)   // this file is not a regular file name ==> confusion!
        return -1;

    return 0;
}

/*
 * close_file
 *   DESCRIPTION:  closes a regular file based on file descriptor, undoes what is in the open function
 *   INPUTS: fd: file descriptor
 *   SIDE EFFECTS: none 
 *   RETURN VALUE: 0 (success) or -1(failure)
 */
int32_t close_file (int32_t fd){
    if (fd <= 1 || fd>= MAX_OPEN_FILES)  //can only close fd = [2,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED)
         return -1; // file is not open
     if (get_file_type_byinode_num(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].inode_num) != REGULAR_FILE_TYPE)
         return -1; // file type not matching

     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags = UNUSED;
     return 0; 
}
/*
 * read_file
 *   DESCRIPTION:  read nbytes of data from file into the buf
 *   INPUTS: fd: file descriptor -- buf: pointer to buffer to be filled --  nbytes: num of bytes to be read
 *   SIDE EFFECTS: calls "read_data"
 *   RETURN VALUE: number of bytes read (success) or -1(failure)
 */
int32_t read_file(int32_t fd, void* buf, int32_t nbytes){
   if (fd <= 1 || fd>= MAX_OPEN_FILES)  //can only close fd = [2,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED)
         return -1; // file is not open
     if (get_file_type_byinode_num(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].inode_num) != REGULAR_FILE_TYPE)
         return -1; // file type not matching

 int32_t nbrbytes_read = read_data(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].inode_num, active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_position, buf, nbytes);
 if(nbrbytes_read == -1)
      return -1;  //failed read 

active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_position += (uint32_t)nbrbytes_read; //update read cursor in file
return nbrbytes_read;
}

/*
 * write_file
 *   DESCRIPTION:  rite to file: should do nothing (for now)
 *   INPUTS: fd: file descriptor -- buf: pointer to buffer to be filled --  nbytes: num of bytes to be written
 *   SIDE EFFECTS: none
 *   RETURN VALUE:  0(success) or -1(failure)
 */
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}


/* directory file functions: SUBJECT TO CHANGE AFTER ADDING FILE DESCIPTORS*/ 

/*
 * open_dir
 *   DESCRIPTION: opens a directory file based on directory file name provided
 *   INPUTS: fname: pointer file name
 *   SIDE EFFECTS: calls "read_dentry_by_name"
 *   RETURN VALUE: 0 (success) or -1(failure)
 */
int32_t open_dir(const uint8_t* fname){   // fname should be "."  <== just a point
    dentry_t open_dir_dentry; 
    if (read_dentry_by_name(fname, &open_dir_dentry) == -1) //invalid read of filename
        return -1;
    if (open_dir_dentry.filetype != DIRECTORY_FILE_TYPE)    // this file is not a regular file name ==> confusion!
        return -1;
  
    return 0;    
}

/*
 * close_dir
 *   DESCRIPTION:  closes a directory file based on file descriptor, undoes what is in the open function
 *   INPUTS: fd: file descriptor
 *   SIDE EFFECTS: none
 *   RETURN VALUE: 0 (success) or -1(failure)
 */
int32_t close_dir(int32_t fd){
    if (fd <= 1 || fd>= MAX_OPEN_FILES)  //can only close fd = [2,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED)
         return -1; // file is not open
     if (get_file_type_byinode_num(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].inode_num) != DIRECTORY_FILE_TYPE)
         return -1; // file type not matching
             
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags = UNUSED;
     return 0; 
}

/*
 * close_dir
 *   DESCRIPTION:  ccannot write to directory ==> does nothing
 *   INPUTS: fd: fd: file descriptor -- buf: pointer to buffer to be filled --  nbytes: num of bytes to be written
 *   SIDE EFFECTS: none
 *   RETURN VALUE: 0 (success) or -1(failure)
 */
int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

/*
 * read_dir
 *   DESCRIPTION:  read files containted in directory filename by filename, including “.” 
 *   INPUTS: fd: file descriptor -- buf: pointer to buffer to be filled --  nbytes: num of bytes to be read at a time
 *   SIDE EFFECTS: calls "read_dentry_by_index"
 *   RETURN VALUE: number of bytes read (success) (0 indicates end of reading (last dentry)) or -1(failure)
 */
int32_t read_dir(int32_t fd, void* buf, int32_t nbytes){
   
  dentry_t dentry;   
  int32_t read_cursor = active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_position;

  if( read_dentry_by_index(read_cursor, &dentry) == -1){
    return 0;
  }
  strncpy((int8_t*)buf, (int8_t*)&(dentry.filename), MAX_FILENAME_LEN);
  int32_t nbr_bytes_read= strlen((int8_t*)&(dentry.filename)); 
  read_cursor ++;  
  active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_position = read_cursor; //update position
 if(nbr_bytes_read > MAX_FILENAME_LEN)
    return MAX_FILENAME_LEN;

  return nbr_bytes_read;
}


/* EXTRA HELPER FUNCTIONS */  

/*
 * get_num_dentries_present
 *   DESCRIPTION: get number of valid dentries currently present in filesystem ==> nbr_dir_entries of boot block
 *   INPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: number of valid dentries currently present in filesystem
 */
int32_t get_num_dentries_present(void){
    return boot_blk_addr->nbr_dir_entries;
}

/*
 * get_file_type_bydentry_index
 *   DESCRIPTION: get filetype corresponding to a dentry specified by index to dentries arr of boot block
 *   INPUTS: dentry_i: index to dentries arr of boot block
 *   SIDE EFFECTS: none
 *   RETURN VALUE: filetype
 */
int32_t get_file_type_bydentry_index(int32_t dentry_i){
    if (dentry_i >= boot_blk_addr->nbr_dir_entries || dentry_i >= NBR_DENTRIES_IN_BOOTBLOCK)
        return -1; //fail if dentry index is invalid
    return boot_blk_addr->dir_entries[dentry_i].filetype; 
}

/*
 * get_file_size_bydentry_index
 *   DESCRIPTION: get filesize corresponding to a dentry specified by index to dentries arr of boot block
 *   INPUTS: dentry_i: index to dentries arr of boot block
 *   SIDE EFFECTS: none
 *   RETURN VALUE: filesize
 */
int32_t get_file_size_bydentry_index(int32_t dentry_i){
    if (dentry_i >= boot_blk_addr->nbr_dir_entries || dentry_i >= NBR_DENTRIES_IN_BOOTBLOCK)
        return -1; //fail if dentry index is invalid
    return inodes_arr_addr[boot_blk_addr->dir_entries[dentry_i].inode_num].file_len_inB;
}

/*
 * get_file_size_byinode_num
 *   DESCRIPTION: get filesize corresponding to an inode specified by inode_number = index to arr of inodes
 *   INPUTS: inode_num: inode mumber = index to arr of inodes
 *   SIDE EFFECTS: none
 *   RETURN VALUE: filesize in bytes or -1: invalid inode number
 */
int32_t get_file_size_byinode_num (int32_t inode_num){
    if (inode_num >= boot_blk_addr->nbr_inodes || inode_num < 0)
        return -1; //fail if inode_num is invalid
    return inodes_arr_addr[inode_num].file_len_inB ;
}

/*
 * get_file_type_byinode_num
 *   DESCRIPTION: get filetype corresponding to an inode specified by inode_number = index to arr of inodes
 *   INPUTS: inode_num: inode mumber = index to arr of inodes
 *   SIDE EFFECTS: none
 *   RETURN VALUE: filetype or -1: invalid inode number
 */
int32_t get_file_type_byinode_num (int32_t inode_number){
    if (inode_number >= boot_blk_addr->nbr_inodes || inode_number < 0)
        return -1; //fail if inode_num is invalid

    uint32_t nbr_dentries_present = boot_blk_addr->nbr_dir_entries; // nbr of directory entries
    uint8_t dentry_i;
    for (dentry_i = 0; dentry_i < nbr_dentries_present; dentry_i ++){

        if(dentry_arr_addr[dentry_i].inode_num == inode_number)
            return dentry_arr_addr[dentry_i].filetype;  
    }
    return -1; //inode number not found in any dentry
}

/*
 * is_file_executable
 *   DESCRIPTION: check if a requested file (by filename) is valid: present and executable 
 *   INPUTS: filename: name of file 
 *   SIDE EFFECTS: none
 *   RETURN VALUE: 0 (file is valid) 
 *                 -1 (file is not present) 
 *                 -2 (file present but not regular file)
 *                 -3 (file is a regular file but not executable)                
 */

int8_t is_file_executable(uint8_t* filename){

    dentry_t exe_file_dentry; // holds dentry for file if it exists

    if ( read_dentry_by_name (filename , &exe_file_dentry)) 
        return -1;  //-1: file is not even present
    if (exe_file_dentry.filetype != REGULAR_FILE_TYPE)    
        return -2;// -2: file present but not a regular file  

    uint8_t magic_nbrs_buff[NBR_EXE_MAGIC_NBRS]; // this buffer will hold first 4 bytes read from file
    //read first 4 bytes of the file
    if( read_data (exe_file_dentry.inode_num, 0, magic_nbrs_buff, NBR_EXE_MAGIC_NBRS) != NBR_EXE_MAGIC_NBRS)
        return -3; // -3: smthing went wrong but we know file is regular type

    //check if first 4 bytes read match magic nbrs identifying executable file
    uint8_t index; 
    for (index =0; index <NBR_EXE_MAGIC_NBRS; index ++){

        if(magic_nbrs_buff[index] != exe_magic_nbrs[index])
                return -3;  //-3: file is not executable
    }

    return 0; //all magic numbers match ==> file is executable
}
