#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "helper.h"
#include "clean.h"
#include "../svc.h"
#include "../core/file.h"

/** This file contains all the helper functions used in svc.c */

// Check if a string given is invalid for a given branch name.
// Utilised in svc_branch.
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

// Check if a branch exists. Utilised in svc_branch.
int branch_exist(void* helper, char* branch_name) {
  struct head* h = (struct head*) helper;
  struct branch* b = h->cur_branch;

  do {
    if (strcmp(b->name, branch_name) == 0) {
      return 1;
    }
    b = b->next_branch;
  } while (b != h->cur_branch);

  return 0;
}

char* current_branch(void* helper) {
  struct head* h = (struct head*) helper;
  FILE* headp = fopen(h->head_fp, "r");

  long fsize = get_num_bytes(h->head_fp);
  char* cur_branch = malloc(fsize + 1);
  fread(cur_branch, 1, fsize, headp);

  fclose(headp);
  return cur_branch;
}

char* hash2str(int hash) {
  char* strh = malloc(snprintf(NULL, 0, "%d", hash) + 1);
  sprintf(strh, "%d", hash);

  return strh;
}

/** List all branches without printing name. Used in the cleanup function.
* Identical to actual list_branches except it does not print branch names.
*/
char **list_branches_noout(void *helper, int *n_branches) {
    // n_branches is null.
    if (n_branches == NULL) {
      return NULL;
    }

    int n = 1;
    char** branch_list = malloc(sizeof(char*));

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    do {
      branch_list[n - 1] = cur->name;

      n += 1;
      branch_list = realloc(branch_list, n * sizeof(char*));

      cur = cur->next_branch;
    } while (cur != h->cur_branch);

    *n_branches = n - 1;

    return branch_list;
}

/** Check all the tracked files and see if there is a modified file.
* If there is at least one modified file then we return 1
* otherwise return 0.
*/
int check_modified(void* helper) {
  struct head* h = (struct head*) helper;
  struct file* files = h->tracked_files;

  // We return this.
  int is_modified = 0;

  // Traverse the doubly linked list to check for modifications.
  while (files != NULL) {

    // Get the hash of the tracked file.
    int hash = hash_file(helper, files->name);

    // If hash equals to -2 the file does not exist return 0.
    if (hash == -2) {
      return 0;
    }

    // Get the number of bytes of said file.
    int bytes = get_num_bytes(files->name);

    // Compare hashes to see if the file with the same name has been modified.
    if (files->hash != hash) {
      // We don't have to check if it exists or not since we already did above.
      FILE* fp = fopen(files->name, "rb");

      // Keep a temp array of contents first in case something goes wrong.
      char* temp = malloc(bytes + 1);

      // Re-read the modified contents.
      if (fread(temp, bytes, 1, fp) != 1) {
        // If something goes wrong we just return an error.
        free(temp);
        fclose(fp);
        return -3;
      }
      temp[bytes] = 0;

      files->contents = realloc(files->contents, bytes + 1);
      strcpy(files->contents, temp);
      files->stat = MODIFIED;
      files->hash = hash_file(helper, files->name);

      // This file is modified.
      is_modified = 1;

      free(temp);
      fclose(fp);
    }

    // We want to check if the files are added. Return 2 if yes to differentiate from modification.
    if (files->stat == ADDED) {
      // There has been an addition.
      is_modified = 2;
    }

    files = files->prev_file;
  }

  // Return whether there is at least one modified tracked file.
  return is_modified;
}

// Comparator used to sort file names alphabetically.
int files_cmp(const void* a, const void* b) {
  struct file* file_a = *(struct file**)a;
  struct file* file_b = *(struct file**)b;
  return strcasecmp(file_a->name, file_b->name);
}

// Used to calculate the commit id as per the algorithm in section 3.2
char* get_commit_id(void* helper, char* message) {

  // Remember to free this, we have to free the commit_id.
  char* hex = malloc(7);
  int id = 0;

  // Get unsigned byte from commit msg.
  for (unsigned long i = 0; i < strlen(message); i++) {
    id = (id + message[i]) % 1000;
  }

  // Get all the tracked files into an array and sort them.
  int n_files = 1;
  struct file** tracked = all_files(helper, &n_files);
  qsort(tracked, n_files - 1, sizeof(struct file*), files_cmp);

  // Traverse all files to calculate commit id.
  for (int i = 0; i < n_files - 1; i++) {
    if (tracked[i]->stat == ALRDY_ADD) {
      continue;
    }
    if (tracked[i]->stat == ADDED) {
      id = id + 376591;
      tracked[i]->stat = ALRDY_ADD;
    }
    if (tracked[i]->stat == REMOVED) {
      id = id + 85973;
    }
    if (tracked[i]->stat == MODIFIED) {
      id = id + 9573681;
    }

    for (unsigned long j = 0; j < strlen(tracked[i]->name); j++) {
      id = (id * (tracked[i]->name[j] % 37)) % 15485863 + 1;
    }
  }

  // Set tracked's contents to NULL since we don't want to free the tracked files yet. Just free tracked.
  for (int i = 0; i < n_files - 1; i++) {
    tracked[i] = NULL;
  }

  sprintf(hex, "%06x", id);
  free(tracked);

  return hex;
}
