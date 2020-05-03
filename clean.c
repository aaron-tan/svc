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

// Get all of the commits from all branches.
struct commit** all_commits(void* helper, int* n_commits) {
  struct head* h = (struct head*) helper;
  struct commit* cur_commit = h->cur_branch->active_commit;

  // We get all the branches first.
  int n_branches;
  // List branches noout does not output branch names for cleanup.
  char** b_list = list_branches_noout(helper, &n_branches);

  // Array to store all branches' commits in.
  struct commit** all_commits = malloc(sizeof(struct commit*));
  int commit_exists = 0;

  // Checkout all the branches and store its commits into the array all_commits.
  for (int i = 0; i < n_branches; i++) {
    svc_checkout(helper, b_list[i]);

    while (cur_commit != NULL) {
      commit_exists = 0;

      // Check if commit exists.
      for (int i = 0; i < (*n_commits - 1); i++) {
        if (all_commits[i] == cur_commit) {
          commit_exists = 1;
        }
      }

      if (!commit_exists) {
        all_commits[*n_commits - 1] = cur_commit;
        *n_commits += 1;
        all_commits = realloc(all_commits, *n_commits * sizeof(struct commit*));
      }

      cur_commit = cur_commit->prev_commit;
    }
  }

  free(b_list);

  return all_commits;
}
