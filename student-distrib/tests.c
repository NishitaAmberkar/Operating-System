#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesystem.h"
#include "terminal.h"
#include "rtc.h"

#define PASS 1
#define FAIL 0

// select tests to run
#define TEST_IDT        0
#define TEST_PAGING     0
#define TEST_FILESYSTEM 0
#define TEST_TERMINAL   0
#define TEST_RTC        0
#define TEST_SYSCALLS   0

#define BEFORE_VIDEO_MEM_ADDR	0xB7FFF
#define START_VIDEO_MEM_ADDR	0xB8000
#define MID_VIDEO_MEM_ADDR		START_VIDEO_MEM_ADDR + 12
#define END_VIDEO_MEM_ADDR		0xB8FFF
#define AFTER_VIDEO_MEM_ADDR	0xB9000
#define START_KERNEL_ADDR		0x400000
#define MID_KERNEL_ADDR			START_KERNEL_ADDR + 12
#define END_KERNEL_ADDR			0x7FFFFF
#define AFTER_KERNEL_ADDR		0x800000

#define FRAME0_FILE_LENGTH      187
#define VERY_LARGE_FILE_LENGTH  5277  
#define LS_EXE_FILE_LEN         5349         

#define READ_CHUNK_SIZE         2000
/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("\n[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/*
 *  print_buf_content_nulbytes_rspaced
 *   DESCRIPTION: helper fnct that prints content of buffer up to a specied size with NUL bytes printed as SPACES
 *   INPUTS: buf: pointer to buffer  -- buf_size: size of buffer or size to be printed starting from 0th char
 *   --------------PLEASE DO NOT EXCEEED BUFFER SIZE ---------------
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void print_buf_content_nulbytes_rspaced(uint8_t* buf, int buf_size){
    int i =0;
    for(i =0; i<buf_size; i++){
        if(buf[i] == '\0'){
            printf(" ");
        }else{
			putc((uint8_t)buf[i], active_terminals.current_showing_terminal);
        }
    }
}

/*
 *  print_buf_content_nulbytes_ignored
 *   DESCRIPTION: helper fnct that prints content of buffer up to a specied size EXCEPT NUL bytes
 *   INPUTS: buf: pointer to buffer  -- buf_size: size of buffer or size to be printed starting from 0th char
 *   --------------PLEASE DO NOT EXCEEED BUFFER SIZE ---------------
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void print_buf_content_nulbytes_ignored(uint8_t* buf, int buf_size){
    int i =0;
    for(i =0; i<buf_size; i++){

        if(buf[i] != '\0')
			putc((uint8_t)buf[i], active_terminals.current_showing_terminal);
        
    }
}
/* Checkpoint 1 tests  */

/* IDT Test - Example 
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */

int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	return result;
}


// check if system call is generated correctly
int idt_test2(){
    printf("\n  **IM GENERATING A SYSTEM CALL  ! ===> \n ");
	asm volatile("int $0x80"); // PLEASE CHANGE THIS VALUE TO CHECK OTHER INTERRUPTS
	return 0;
}

// check if a certain interrupt is correctly generated (edit index IDT for specific interrupts or exceptions)
int idt_test3(){
    printf("\n **IM GENERATING A SIMD FLOATING INTERRUPT! ===> \n ");
	asm volatile("int $19"); // PLEASE CHANGE THIS VALUE TO CHECK OTHER INTERRUPTS
	return 0;
}

//UNCOMMENT THIS TO TEST (COMMENTED SINCE IT CAUSES A WARNING)
//divide by zero exception should happen
// int divide_by_zero_test(){

//     int test = 5/0; 
// 	test = 12;
//     // should print DIVIDE BY ZERO EXCEPTION
// 	return FAIL;

// }

/***********       PAGING TESTS     *****************/
 
// used in paging tests to indentify different regions of physical memory to be tested
uint32_t physical_memory_addr_regions[9]= { BEFORE_VIDEO_MEM_ADDR, START_VIDEO_MEM_ADDR, MID_VIDEO_MEM_ADDR, END_VIDEO_MEM_ADDR, AFTER_VIDEO_MEM_ADDR, 
START_KERNEL_ADDR, MID_KERNEL_ADDR, END_KERNEL_ADDR, AFTER_KERNEL_ADDR};

/* paging_test_should_reference
 * 
 * Try to derefence a valid mapped physical address (SHOULD PASS! ==> NOT PAGE_FAULT)
 * Inputs: memory region described by above array 
 * -- PLEASE SELECT THE VALID REGION NBR ACCORDING TO PHYSICAL ADDRESSESEs IN ARRAY
 * Outputs: PASS (PRINTED)/ or PAGE FAULT-- inifite loop (MEANS IT FAILED)
 * Side Effects: None
 * Coverage: Load paging
 * Files: page.c, page.h
 */
int paging_test_should_reference(unsigned int region_num){
	TEST_HEADER;

	uint8_t* ptr = (uint8_t*)(physical_memory_addr_regions[region_num]); // PLEASE PASS A DIFFERENT REGION_NUM TO TEST OTHER REGIONS
	uint8_t test = *ptr;
	test = 0xFF; // to remove unused variable warning
	*ptr = 0xFF;
	return PASS;
}

/* paging_test_should_pagefault
 * 
 * Try to derefence a INvalid NON_mapped physical address (SHOULD NOT FAIL! ==> SHOULD PAGE_FAULT (infinite loop))
 * Inputs: memory region num described by above array "physical_memory_addr_regions"
 * -- PLEASE SELECT THE INVALID REGION NBR ACCORDING TO PHYSICAL ADDRESSESEs IN ARRAY
 * Outputs: PASS BY INFINITE LOOPING (PAGE FAULT)/ FAIL (PRINTED)
 * Side Effects: None
 * Coverage: Load paging
 * Files: page.c, page.h
 */
int paging_test_should_pagefault(int region_num){
	TEST_HEADER;

	uint8_t* ptr = (uint8_t*)(physical_memory_addr_regions[region_num]); // PLEASE PASS A DIFFERENT REGION_NUM TO TEST OTHER REGIONS
	uint8_t test = *ptr; //read
	test = 0xFF; // to remove unused variable warning
	*ptr = 0xFF;  //write
	return FAIL;
}

/* Checkpoint 2 tests */

/* DIRECTORY FUNCTIONS TESTS */


/* test_open_valid_directory
 * Asserts: opening a valid directory is succesfull
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_open_valid_directory(){
 TEST_HEADER;
 int32_t fd = open_dir ((uint8_t*)"."); // "." is a valid directory
 if (!fd){
	close_dir(fd); // do not leave directory open 
    return PASS; 
 }
return FAIL; 	
}

/* test_open_invalid_directory
 * Asserts: opening an invalid directory is failed
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_open_invalid_directory(){
 TEST_HEADER;
 int32_t fd = open_dir ((uint8_t*)".dsadas"); // "." is a valid directory
 if (!fd)
    return FAIL; 
return PASS; 	

}

/* test_read_directory
 * 
 * Asserts: * ALL directory entries can be read by displaying the filenames and their size 
 *          * # of dentries read is equal to # of dentries present (= nbr_dir_entries field in boot block of filesystem)
 *          * call to open_dir() and close_dir() are succesfull 
 *          * call to read_by_index() helper function in filesystem.h -- called by read_dir -- is succesfull
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: FS structure, FS initializaiton, FS utility functions, Directory file specific functions
 * Files: filesytem.c, filesytem.h
 */

int test_read_directory(){
 TEST_HEADER;

    int32_t fd, cnt; 
    uint8_t buf[MAX_FILENAME_LEN];
	int32_t dentries_count =0; //counts
    
    if (-1 == (fd = open_dir ((uint8_t*)"."))) {
        printf("\n ----- directory open failed               \n");
        return FAIL;
    }
    // printf("\n**********  LIST OF FILES IN DIRECTORY (.)   *************\n");
    while (0 != (cnt = read_dir (fd, buf, (int32_t)MAX_FILENAME_LEN))) { 
        if (-1 == cnt) {
	        printf("\n ----- directory entry read failed               \n");
	        return FAIL;
	    }
		printf("file_name: ");
		print_buf_content_nulbytes_rspaced(buf, (int)(MAX_FILENAME_LEN));
        printf("  file_type: %d, file_size: %d\n", get_file_type_bydentry_index(dentries_count), get_file_size_bydentry_index(dentries_count));
		dentries_count ++; 
    }
//  printf("\n..... ** END OF LIST ** =====> %u DENTRIES READ.               \n", (unsigned int)dentries_count);
 //assert that number of dentries read is equal to num of dentries currenly present in FS (nbr_dir_entries field in boot block) 
 if (dentries_count != get_num_dentries_present()) 
     return FAIL;

 close_dir(fd); // do not leave directory open
 return PASS; 
}

/* test_write_directory
 * Asserts: writing to a directory fails
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_write_directory(){
 TEST_HEADER;
 int32_t fd;
 uint8_t buf[MAX_FILENAME_LEN];
     if (-1 == (fd = open_dir ((uint8_t*)"."))) {
        printf("\n ----- directory open failed               \n");
        return FAIL;
    }

	if(-1 == write_dir (fd, buf, MAX_FILENAME_LEN)) //write to directory should fail
	    return PASS; 

 close_dir(fd); // do not leave directory open
 return FAIL; 
}

/* test_close_directory
 * Asserts: closing an OPEN directory is succesfull and closing a non-open directory is failed
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_close_directory(){
TEST_HEADER;
int32_t fd;

 if(close_dir(0) != -1) //trying to close a non-open direcory should return -1 (fail)
     return FAIL; 

 if (-1 == (fd = open_dir ((uint8_t*)"."))) {
        printf("\n ----- directory open failed               \n");
        return FAIL;
    }

if(0 == close_dir (fd)) //now closing an open directory should PASS
	    return PASS; 

return FAIL;
}

/* FILE FUNCTIONS TESTS */

/* test_open_existent_file
 * Asserts: opening an existing file is succesfull 
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_open_existent_file(){
 TEST_HEADER;
 int32_t fd = open_file ((uint8_t*)"pingpong"); // "pingpong" file exists
 if (!fd){
	close_file(fd); // do not leave file open
    return PASS; 
 }
return FAIL; 
}

/* test_open_nonexistent_file
 * Asserts: opening a NON-existing file is failed
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_open_nonexistent_file(){
 TEST_HEADER;
 int32_t fd = open_file ((uint8_t*)"justafilename"); // "justafilename" does not exist
 if (!fd){
	close_file(fd); // do not leave file open in case it was opened	 
    return FAIL; 
 }

return PASS; 	
}

/* test_open_nonregulartype_file
 * Asserts: opening a file that is is not of type (regular file) with regular file's open fnct --> is failed
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_open_nonregulartype_file(){
 TEST_HEADER;
 int32_t fd = open_file ((uint8_t*)"."); // "." exists but not a regular file, it is a directory
 if (!fd){
	close_file(fd); // do not leave file open in case it was opened	 
    return FAIL; 
 }
return PASS; 	
}

/* test_close_file
 * Asserts: closing an OPEN file is succesfull and closing a non-open file is failed
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_close_file(){
TEST_HEADER;
int32_t fd;

 if(close_file(0) != -1) //trying to close a non-open file should return -1 (fail)
     return FAIL; 

 if (-1 == (fd = open_file ((uint8_t*)"frame0.txt"))) {
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

if(0 == close_file (fd)) //now closing the open file should PASS
	    return PASS; 

return FAIL;
}

/* test_write_file
 * Asserts: write to a file is not possible for now (read only) 
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Filesytem
 * Files: filesytem.c, filesytem.h
 */
int test_write_file(){
 TEST_HEADER;
 int32_t fd;
 uint8_t buf[MAX_FILENAME_LEN];
     if (-1 == (fd = open_file ((uint8_t*)"verylargetextwithverylongname.txt"))) {
		//NOTE: "verylargetextwithverylongname.txt" is 33 char long so will be clipped to 32
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

	if(-1 == write_file (fd, buf, MAX_FILENAME_LEN)){ //write to file fails at least for now
	    close_file(fd); // do not leave file open
	    return PASS; }

 close_file(fd); 
 return FAIL; 
}

/* test_read_small_ file
 * 
 * Asserts: * specified number of bytes are read from a specified OPEN SMALL file (max: file length)
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: FS structure, FS initializaiton, FS utility functions, Regular file specific functions
 * Files: filesytem.c, filesytem.h
 */
int test_read_small_file(){
 TEST_HEADER;
 int32_t fd;
 uint8_t buf[FRAME0_FILE_LENGTH];
     if (-1 == (fd = open_file ((uint8_t*)"frame0.txt"))) {
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

if (read_file(fd, buf, (int32_t) FRAME0_FILE_LENGTH) != FRAME0_FILE_LENGTH){
    close_file(fd); 
    return FAIL; 
}
   print_buf_content_nulbytes_rspaced(buf, (int)(FRAME0_FILE_LENGTH));
   printf("\nfile_name: frame0.txt\n");
   close_file(fd);   
   return PASS;  
}

/* test_read_large_file
 * 
 * Asserts: * specified number of bytes are read from a specified OPEN LARGE file (max: file length)
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: FS structure, FS initializaiton, FS utility functions, Regular file specific functions
 * Files: filesytem.c, filesytem.h
 */
int test_read_large_file(){
 TEST_HEADER;
 int32_t fd;
 uint8_t buf[VERY_LARGE_FILE_LENGTH];
     if (-1 == (fd = open_file ((uint8_t*)"verylargetextwithverylongname.txt"))) { // the name gets clipped to 32 B when searched for 
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

if (read_file(fd, buf, (int32_t) VERY_LARGE_FILE_LENGTH) != VERY_LARGE_FILE_LENGTH){
    close_file(fd); 
    return FAIL; 
}
   print_buf_content_nulbytes_rspaced (buf, (int)(VERY_LARGE_FILE_LENGTH));
   printf("\nfile_name:  verylargetextwithverylongname.tx\n");
   close_file(fd);   
   return PASS;  
}

/* test_read_exe_file
 * 
 * Asserts: * specified number of bytes are read from a specified OPEN EXECUTABLE file (max: file length)
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: FS structure, FS initializaiton, FS utility functions, Regular file specific functions
 * Files: filesytem.c, filesytem.h
 */
int test_read_exe_file(){
 TEST_HEADER;
 int32_t fd;
 uint8_t buf[LS_EXE_FILE_LEN];
     if (-1 == (fd = open_file ((uint8_t*)"ls"))) {
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

if (read_file(fd, buf, (int32_t) LS_EXE_FILE_LEN) != LS_EXE_FILE_LEN){
    close_file(fd); 
    return FAIL; 
}
   print_buf_content_nulbytes_ignored(buf, (int)(LS_EXE_FILE_LEN)); //print exe file with nul bytes ignored(not printed) to see begining and end patterns
   printf("\nfile_name: ls\n");
   close_file(fd);   
   return PASS;  
}

/* test_read_file_by_chunks
 * 
 * Asserts: * file is read by chunks (size can be changed) until end of file is reached 
 * Inputs: filename: name of file to be read
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: FS structure, FS initializaiton, FS utility functions, Regular file specific functions
 * Files: filesytem.c, filesytem.h
 */
int test_read_file_by_chunks(uint8_t* filename){
 TEST_HEADER;
 int32_t fd, nbr_bytes_read , total_nbr_bytes_read =0;
 uint8_t buf[READ_CHUNK_SIZE];
     if (-1 == (fd = open_file ((uint8_t*)filename))) {
        printf("\n ----- file open failed               \n");
        return FAIL;
    }

while (0 != (nbr_bytes_read = read_file (fd, buf, READ_CHUNK_SIZE))){
   //printf("\nnbr_bytes_read: %d \n", nbr_bytes_read);
   print_buf_content_nulbytes_rspaced(buf, (int)(nbr_bytes_read));
   total_nbr_bytes_read += nbr_bytes_read;
}
   printf("\nfile_name: ");
   print_buf_content_nulbytes_rspaced(filename,(int) MAX_FILENAME_LEN);
   printf("\ntotal bytes read: %d \n", total_nbr_bytes_read );
   close_file(fd);   
   return PASS;  
}

/* test_terminal_rw
 * 
 * Asserts: termianal read and write functionality
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: terminal read retruns string, terminal write returns zero (success)
 */
int test_terminal_rw(){
	TEST_HEADER;

	char test_buf[128];
	char gold_buf[128] = "A TA needs to type this during demo :)\n";
	char test_pass[50] = "Good typin' tex, test pass\n";
	printf("Copy the following string exactly then press enter:\n");
	printf(gold_buf);
	terminal_read(0, test_buf, strlen(gold_buf));
	if(strncmp(gold_buf, test_buf, strlen(gold_buf)) == 0){
		if(terminal_write(0, test_pass, strlen(test_pass)) == 0);
			return PASS;
	}
	printf("buffer returned:\n");
	printf(test_buf);
	return FAIL;
}

int test_terminal_null(){
	TEST_HEADER;
	if(terminal_read(0, 0, 1) == -1)
		return PASS;
	return FAIL;
}

/* test_read_file
 * 
 * Asserts: termianal close disables cursor, should re-enable at exit
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: terminal close, terminal open
 */
int test_terminal_close(){
	TEST_HEADER;
	char result[1];
	terminal_close();
	printf("is the cursor still there? y/n\n");
	terminal_read(0, result, 1);
	if((result[0] == 'n') || (result[0] == 'N')){
		terminal_open();
		return PASS;
	}
	terminal_open();
	return FAIL;
}


/* TEST RTC */
int test_freq() {
    TEST_HEADER;
    int i;
    uint16_t frequency = 2;
    rtc_open(NULL);

    while (frequency <= 1024) {
        i = 0;
        while (i < 5) {
            rtc_read(NULL, NULL, 0);
            printf("frequency: %d \n", frequency);
			printf("index: %d \n", i);
            i++;
        }
        frequency = frequency * 2;
        rtc_write(NULL, (void*) &frequency, 4);
        printf("\n");
    }
    rtc_close(NULL);
    return PASS;
}

/* Checkpoint 3 tests */

/* Test suite entry point */
void launch_tests(){
	// CHECKPOINT 1
	if(TEST_IDT){
		TEST_OUTPUT("idt_test", idt_test());
		TEST_OUTPUT("idt_test2", idt_test2());
		TEST_OUTPUT("idt_test3", idt_test3());
		//TEST_OUTPUT("divide_by_zero_test", divide_by_zero_test());
	}

	if(TEST_PAGING){
     TEST_OUTPUT("paging_test_should_reference", paging_test_should_reference(7));  //change region tested by changing integer argument (index to "physical_memory_addr_regions" above)
	 TEST_OUTPUT("paging_test_should_pagefault", paging_test_should_pagefault(0));
	} 
    
	//CHECKPOINT 2
	if(TEST_FILESYSTEM){
	/* tests for functions of directories */
    // TEST_OUTPUT("test_open_valid_directory", test_open_valid_directory());
    // TEST_OUTPUT("test_open_invalid_directory", test_open_invalid_directory());
	// TEST_OUTPUT("test_close_directory", test_close_directory());
	// TEST_OUTPUT("test_write_directory", test_write_directory());
	// TEST_OUTPUT("test_read_directory", test_read_directory());

    /* tests for functions of files */
	// TEST_OUTPUT("test_open_existent_file", test_open_existent_file());
    // TEST_OUTPUT("test_open_nonexistent_file", test_open_nonexistent_file());
	// TEST_OUTPUT("test_open_nonregulartype_file", test_open_nonregulartype_file());
	// TEST_OUTPUT("test_close_file", test_close_file());
	// TEST_OUTPUT("test_write_file", test_write_file())
	// TEST_OUTPUT("test_read_small_file", test_read_small_file());
	// TEST_OUTPUT("test_read_large_file", test_read_large_file());
	// TEST_OUTPUT("test_read_exe_file", test_read_exe_file());

	uint8_t filename[MAX_FILENAME_LEN] = "frame1.txt";
	TEST_OUTPUT("test_read_file_by_chunks", test_read_file_by_chunks(filename));

    }

    if(TEST_TERMINAL){
	TEST_OUTPUT("test terminal null pointer", test_terminal_null());
	TEST_OUTPUT("test open/close", test_terminal_close());
	TEST_OUTPUT("test terminal w/r", test_terminal_rw());
	}

	if(TEST_RTC){
    TEST_OUTPUT("test rtc freq w/r", test_freq());
  
	}

    if(TEST_SYSCALLS){
       clear(); 
       printf("\n checking if shell is executable\n");
       if(is_file_executable((uint8_t*)"shell"))
	         printf("\n file is not executable\n");
	   else 
	        printf("\n file is executable");
  
	}

}

