#include <stdio.h>
#include <string.h>
#include "svc.h"

// Check if a string given is invalid for branch name.
int check_invalid(char* str) {
  while (*str) {
    // If str contains at least one of these ascii. it is invalid.
    if (*str == 45 ||
    *str == 47 ||
    (*str >= 48 && *str <= 57) ||
    (*str >= 65 && *str <= 90) ||
    (*str >= 97 && *str <= 122) ||
    *str == 95) {
      str++;
    } else {
      return 1;
    }
  }
  return 0;
}

// Initialise the data structures and return a ptr to the memory.
void *svc_init(void) {
    // Branches will be stored as a circular linked list.
    struct branch* master = malloc(sizeof(struct branch));
    master->name = malloc(51);
    strcpy(master->name, "master");
    master->active_commit = NULL;
    // Master initially points back to itself.
    master->next_branch = master;

    struct head* helper = malloc(sizeof(struct head));
    helper->cur_branch = master;
    helper->tracked_files = NULL;

    return helper;
}

// Free the helper data structure.
void cleanup(void *helper) {
    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    struct branch** all_branches = malloc(sizeof(struct branch*));
    int num_branches = 1;

    do {
      all_branches[num_branches - 1] = cur;
      num_branches += 1;
      all_branches = realloc(all_branches, num_branches * sizeof(struct branch*));

      cur = cur->next_branch;
    } while (cur != h->cur_branch);

    for (int i = 0; i < (num_branches - 1); i++) {
      free(all_branches[i]->name);
      free(all_branches[i]);
    }
    free(all_branches);
    free(h);
}

// Calculate the hash value of a file.
int hash_file(void *helper, char *file_path) {
    if (file_path == NULL || helper == NULL) {
      return -1;
    }

    FILE* fp = fopen(file_path, "r");

    // Check if the file exists.
    if (fp == NULL) {
      return -2;
    }

    fseek(fp, 0L, SEEK_END);

    // Get the num_bytes.
    int num_bytes = ftell(fp);

    // Reset the file.
    fseek(fp, 0, SEEK_SET);

    int hash = 0;

    // Calculate hash of file name.
    for (unsigned long i = 0; i < strlen(file_path); i++) {
      hash = (hash + file_path[i]) % 1000;
    }

    // Calculate the hash of file contents.
    for (int i = 0; i < num_bytes; i++) {
      int byte = fgetc(fp);
      hash = (hash + byte) % 2000000000;
    }

    fclose(fp);

    return hash;
}

char *svc_commit(void *helper, char *message) {
    // TODO: Implement
    return NULL;
}

void *get_commit(void *helper, char *commit_id) {
    // TODO: Implement
    return NULL;
}

char **get_prev_commits(void *helper, void *commit, int *n_prev) {
    // TODO: Implement
    return NULL;
}

void print_commit(void *helper, char *commit_id) {
    // TODO: Implement
}

// Creates a new branch.
int svc_branch(void *helper, char *branch_name) {
    // Do some initial checks.
    if (branch_name == NULL || check_invalid(branch_name)) {
      return -1;
    }

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    // Check if there are uncommited changes.
    if (h->tracked_files != NULL) {
      return -3;
    }

    // traverse the circular linked list until we reach the start again.
    // If we reach back to start_branch break out of loop.
    do {
      if (strcmp(cur->name, branch_name) == 0) {
        return -2;
      }
      // Check if the one after this is at the start. If it is we insert new branch.
      if (cur->next_branch == h->cur_branch) {
        // Create a new branch
        struct branch* new_branch = malloc(sizeof(struct branch));
        new_branch->name = malloc(51);
        strcpy(new_branch->name, branch_name);
        new_branch->active_commit = NULL;
        new_branch->next_branch = h->cur_branch;

        // Set cur's next_branch to new_branch.
        cur->next_branch = new_branch;
        break;
      }

      // If we are at master with no other branches this returns itself.
      cur = cur->next_branch;

    } while (cur != h->cur_branch);

    return 0;
}

int svc_checkout(void *helper, char *branch_name) {
    // TODO: Implement
    return 0;
}

char **list_branches(void *helper, int *n_branches) {
    // n_branches is null.
    if (n_branches == NULL) {
      return NULL;
    }

    int n = 1;
    char** branch_list = malloc(sizeof(char*));

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    do {
      printf("%s\n", cur->name);
      // branch_list[n - 1] = malloc(51);
      // strcpy(branch_list[n - 1], cur->name);
      branch_list[n - 1] = cur->name;

      n += 1;
      branch_list = realloc(branch_list, n * sizeof(char*));

      cur = cur->next_branch;
    } while (cur != h->cur_branch);

    *n_branches = n - 1;

    return branch_list;
}

int svc_add(void *helper, char *file_name) {
    // TODO: Implement
    return 0;
}

int svc_rm(void *helper, char *file_name) {
    // TODO: Implement
    return 0;
}

int svc_reset(void *helper, char *commit_id) {
    // TODO: Implement
    return 0;
}

char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {
    // TODO: Implement
    return NULL;
}
