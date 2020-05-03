#ifndef clean_h
#define clean_h

struct file** all_files(void* helper, int* n_files);

struct branch** all_branches(void* helper, int* n_branches);

struct commit** all_commits(void* helper, int* n_commits);

#endif
