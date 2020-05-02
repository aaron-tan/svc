#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "helper.h"
#include "svc.h"

/** This file contains all the helper functions I will be using in svc.c */

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

// Check if a branch exists.
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
