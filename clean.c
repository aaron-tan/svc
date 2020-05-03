#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "helper.h"
#include "svc.h"

// Get all of the tracked files.
struct file** all_files(void* helper, int* n_files) {
  struct head* h = (struct head*) helper;
  struct file* t_files = h->tracked_files;

  struct file** all_files = malloc(sizeof(struct file*));

  while (t_files != NULL) {
    all_files[*n_files - 1] = t_files;

    *n_files += 1;
    all_files = realloc(all_files, *n_files * sizeof(char*));

    t_files = t_files->prev_file;
  }

  return all_files;
}

// Get all of the branches.
struct branch** all_branches(void* helper, int* n_branches) {
  struct head* h = (struct head*) helper;
  struct branch* cur_branch = h->cur_branch;

  struct branch** all_branches = malloc(sizeof(struct branch*));

  do {
    all_branches[*n_branches - 1] = cur_branch;
    *n_branches += 1;
    all_branches = realloc(all_branches, *n_branches * sizeof(struct branch*));

    cur_branch = cur_branch->next_branch;
  } while (cur_branch != h->cur_branch);

  return all_branches;
}
