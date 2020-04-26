#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

// Get the number of bytes of a file.
int get_num_bytes(char* file_name) {
  FILE* fp = fopen(file_name, "rb");

  if (fp == NULL) {
    return -3;
  }

  fseek(fp, 0L, SEEK_END);

  int num_bytes = ftell(fp);

  fseek(fp, 0, SEEK_SET);

  fclose(fp);

  return num_bytes;
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
    struct file* files = h->tracked_files;

    /** Clean up for if there is something staging.
    * After calling svc_add without svc_rm there is something
    * in tracked_files, so we clean it up.
    */
    struct file** all_files = malloc(sizeof(struct file*));
    int num_files = 1;
    if (files != NULL) {
      all_files[num_files - 1] = files;
      num_files += 1;

      while (files->next_file != NULL) {
        files = files->next_file;
        all_files = realloc(all_files, num_files * sizeof(struct file*));
        all_files[num_files - 1] = files;
        num_files += 1;
      }

      for (int i = 0; i < (num_files - 1); i++) {
        free(all_files[i]->contents);
        free(all_files[i]);
      }
    }
    // End of cleaning up if there is some files left tracked.

    // Clean up for all the branches.
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
    // End of cleaning up for all the branches.

    free(all_files);
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
      branch_list[n - 1] = cur->name;

      n += 1;
      branch_list = realloc(branch_list, n * sizeof(char*));

      cur = cur->next_branch;
    } while (cur != h->cur_branch);

    *n_branches = n - 1;

    return branch_list;
}

int svc_add(void *helper, char *file_name) {
    if (file_name == NULL) {
      return -1;
    }

    // Check if file exists.
    if (access(file_name, F_OK) == -1) {
      return -3;
    }

    FILE* fp = fopen(file_name, "rb");
    int bytes = get_num_bytes(file_name);

    // Add to the system.
    struct head* h = (struct head*) helper;

    // Check if the file has already been added.
    if (h->tracked_files != NULL) {
      struct file* check = h->tracked_files;

      while (check != NULL) {
        if (strcmp(check->name, file_name) == 0) {
          return -2;
        }
        check = check->next_file;
      }
    }

    // There are no tracked files.
    if (h->tracked_files == NULL) {
      // Remember to free this later.
      h->tracked_files = malloc(sizeof(struct file));

      // Populate the file with data.
      h->tracked_files->name = file_name;
      // Remember to free this later.
      h->tracked_files->contents = malloc((bytes + 1));

      // Read file contents into tracked_files contents.
      if (fread(h->tracked_files->contents, bytes, 1, fp) != 1) {
        // If something goes wrong we revert back to the original state.
        free(h->tracked_files->contents);
        free(h->tracked_files);
        h->tracked_files = NULL;
        fclose(fp);
        return -3;
      }
      h->tracked_files->contents[bytes] = 0;

      h->tracked_files->stat = ADDED;
      h->tracked_files->hash = hash_file(helper, file_name);
      h->tracked_files->prev_file = NULL;
      h->tracked_files->next_file = NULL;
    } else {
      // Traversal the doubly linked list until we get to a NULL.
      struct file* files = h->tracked_files;

      while (files->next_file != NULL) {
        files = files->next_file;
      }

      // Track this new file. Remember to free this later.
      struct file* new_file = malloc(sizeof(struct file));
      new_file->name = file_name;
      // Remember to free this later.
      new_file->contents = malloc((bytes + 1));

      // Read file contents into contents.
      if (fread(new_file->contents, bytes, 1, fp) != 1) {
        // If something goes wrong we revert back to the original state.
        free(new_file->contents);
        free(new_file);
        fclose(fp);
        return -3;
      }
      new_file->contents[bytes] = 0;

      new_file->stat = ADDED;
      new_file->hash = hash_file(helper, file_name);
      new_file->prev_file = files;
      new_file->next_file = NULL;

      // Set files->next_file to the new tracked file.
      files->next_file = new_file;
    }

    fclose(fp);

    return hash_file(helper, file_name);
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
