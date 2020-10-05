#ifndef helper_h
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#define helper_h

int check_invalid(char* str);

int get_num_bytes(char* file_name);

int branch_exist(void* helper, char* branch_name);

char **list_branches_noout(void *helper, int *n_branches);

int check_modified(void* helper);

int files_cmp(const void* a, const void* b);

char* get_commit_id(void* helper, char* message);

// Create a directory with name of dir_name and mode.
void create_dir(char* dir_name, mode_t mode);

// Get the size of a file
long get_file_size(char* file_name);

#endif
