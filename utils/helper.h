#ifndef helper_h
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#define helper_h

int check_invalid(char* str);

int branch_exist(void* helper, char* branch_name);

// Return the current active branch as a string
char* current_branch(void* helper);

char **list_branches_noout(void *helper, int *n_branches);

int check_modified(void* helper);

int files_cmp(const void* a, const void* b);

char* get_commit_id(void* helper, char* message);

#endif
