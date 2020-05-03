#ifndef helper_h
#define helper_h

int check_invalid(char* str);

int get_num_bytes(char* file_name);

int branch_exist(void* helper, char* branch_name);

char **list_branches_noout(void *helper, int *n_branches);

int check_modified(void* helper);

int files_cmp(const void* a, const void* b);

char* get_commit_id(void* helper, char* message);

#endif
