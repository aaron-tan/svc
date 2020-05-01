#ifndef svc_h
#define svc_h

#include <stdlib.h>

typedef enum {ADDED, MODIFIED, REMOVED, MERGED} STATUS;

struct head {
  struct branch* cur_branch;
  struct file* tracked_files;
  int n_tracked;
};

struct branch {
  char* name;
  struct commit* active_commit;
  struct branch* next_branch;
};

struct commit {
  char* commit_id;
  char* branch_name;
  char* commit_msg;
  struct file* files;
  struct commit* prev_commit;
  struct commit* next_commit;
};

struct file {
  char* name;
  char* contents;
  STATUS stat;
  int hash;
  struct file* prev_file;
  struct file* next_file;
};

typedef struct resolution {
    // NOTE: DO NOT MODIFY THIS STRUCT
    char *file_name;
    char *resolved_file;
} resolution;

void *svc_init(void);

void cleanup(void *helper);

int hash_file(void *helper, char *file_path);

char *svc_commit(void *helper, char *message);

void *get_commit(void *helper, char *commit_id);

char **get_prev_commits(void *helper, void *commit, int *n_prev);

void print_commit(void *helper, char *commit_id);

int svc_branch(void *helper, char *branch_name);

int svc_checkout(void *helper, char *branch_name);

char **list_branches(void *helper, int *n_branches);

int svc_add(void *helper, char *file_name);

int svc_rm(void *helper, char *file_name);

int svc_reset(void *helper, char *commit_id);

char *svc_merge(void *helper, char *branch_name, resolution *resolutions, int n_resolutions);

#endif
