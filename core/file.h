#ifndef file_h
#define file_h

// Return the current branch path in .svc as a string
char* get_curb_path(void* helper);

// Get the number of bytes of a file
int get_num_bytes(char* file_name);

// Create a directory with dir_name
void create_dir(char* dir_name, mode_t mode);

// Return a list of all files un the directory dir_name
char** ls_dir(char* dir_name, int* ls_len);

#endif