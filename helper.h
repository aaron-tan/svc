#ifndef helper_h
#define helper_h

int check_invalid(char* str);

int get_num_bytes(char* file_name);

int branch_exist(void* helper, char* branch_name);

int check_modified(void* helper);

int check_uncommitted(void* helper);

char* get_commit_id(void* helper, char* message);

#endif
