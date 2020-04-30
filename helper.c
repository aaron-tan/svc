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
    // Open the file to get ready to read it if necessary.
    FILE* fp = fopen(files->name, "rb");

    // Check if the file exists.
    if (fp == NULL) {
      return -2;
    }

    // Get the hash of the tracked file.
    int hash = hash_file(helper, files->name);

    // Could it be possible because this is outside the if it causes problems?
    // Get the number of bytes of said file.
    // int bytes = get_num_bytes(files->name);
    // printf("%d\n", bytes);

    // Compare hashes to see if the file with the same name has been modified.
    if (files->hash != hash) {

      // Keep a temp array of contents first in case something goes wrong.
      char* temp = malloc(100);

      // Re-read the modified contents.
      if (fread(temp, 100, 1, fp) != 1) {
        // If something goes wrong we just return an error.
        free(temp);
        fclose(fp);
        return -3;
      }
      temp[100] = 0;

      files->contents = temp;
      files->stat = MODIFIED;
      files->hash = hash_file(helper, files->name);

      // This file is modified.
      is_modified = 1;
      fclose(fp);
    }

    // We also check if there are files added. Return 1 if yes.
    if (files->stat == ADDED) {
      // There has been an addition.
      is_modified = 1;
    }

    files = files->prev_file;
  }

  // Return whether there is at least one modified tracked file.
  return is_modified;
}

// Check if there are uncommitted changes.
int check_uncommitted(void* helper) {
  struct head* h = (struct head*) helper;
  struct file* track_files = h->tracked_files;
  struct branch* cur = h->cur_branch;
  struct commit* cur_com = cur->active_commit;

  // If track_files is null, there are no commits at all. return 1
  if (track_files == NULL) {
    return 0;
  }

  // If active commit is null, then everything in tracked files is an uncommitted change.
  if (cur_com == NULL) {
    return 1;
  } else {

  }
  struct file* comm_files = cur_com->files;

  while (comm_files != NULL) {
    // Get the hash of the file.
    int h_file = hash_file(helper, comm_files->name);

    // If the hash of the commit is different to the hash of the file there are uncommitted changes.
    if (comm_files->hash != h_file) {
      return 1;
    }

    comm_files = comm_files->prev_file;
  }

  return 0;
}

// Used to calculate the commit id as per the algorithm in section 3.2
char* get_commit_id(void* helper, char* message) {
  struct head* h = (struct head*) helper;
  struct file* files = h->tracked_files;

  // Remember to free this, we have to free the commit_id.
  char* hex = malloc(7);
  int id = 0;

  // Get unsigned byte from commit msg.
  for (unsigned long i = 0; i < strlen(message); i++) {
    id = (id + message[i]) % 1000;
  }
  printf("Id: %d\n", id);

  // Traverse backwards, so that we can traverse forwards again to get commit id.
  while (files->prev_file != NULL) {
    files = files->prev_file;
  }

  // Get the commit changes.
  while (files != NULL) {
    if (files->stat == ADDED) {
      puts("In added");
      id = id + 376591;
    } else if (files->stat == MODIFIED) {
      id = id + 85973;
    } else {
      id = id + 9573681;
    }
    printf("In while: %d\n", id);

    // Get unsigned byte from file name. Increasing alphabetical order.
    for (unsigned long i = 0; i < strlen(files->name); i++) {
      printf("%c\n", files->name[i]);
      id = (id * (files->name[i] % 37)) % 15485863 + 1;
      printf("Loop the file name: %d\n", id);
    }

    files = files->next_file;
  }
  sprintf(hex, "%x", id);
  printf("End: %d\n", id);

  return hex;
}
