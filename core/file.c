#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "../utils/helper.h"
#include "file.h"

char* get_curb_path(void* helper) {
  char* cur_branch = current_branch(helper);

  char* path = malloc(14 + strlen(cur_branch) + 1);
  sprintf(path, "%s/%s", ".svc/branches", cur_branch);

  free(cur_branch);
  return path;
}

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

void create_dir(char* dir_name, mode_t mode) {
  int ret_val = mkdir(dir_name, mode);
  if (ret_val < 0) {
    if (errno == EEXIST) {
      return;
    } else {
      fprintf(stderr, "Could not create directory\n");
    }
  }

  return;
}

char** ls_dir(char* dir_name, int* ls_len) {
  DIR* dirp;
  struct dirent* dir;

  // Array of strings containing all the files under directory
  char** list = malloc(sizeof(char*));

  dirp = opendir(dir_name);
  if (dirp == NULL) {
    perror("Error opening the directory");
    return NULL;
  }

  // Iter count to keep track of the number of file
  int iter = 0;

  // Read each entry in the directory.
  while ((dir = readdir(dirp)) != NULL) {
    if (dir->d_type == DT_REG) {
      list[iter] = malloc(strlen(dir->d_name) + 1);
      strcpy(list[iter], dir->d_name);
      iter += 1;

      // Set dir_count + 1 so we can realloc a larger array by one.
      list = realloc(list, (iter + 1) * sizeof(char*));
    }
  }

  // Store the length of the list
  *ls_len = iter;

  return list;
}
