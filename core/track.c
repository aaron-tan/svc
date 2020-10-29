#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../svc.h"
#include "../utils/helper.h"
#include "track.h"
#include "file.h"

void copy_file(int hash_file, char* file_path, char* file_name) {
  // Convert the hash into a string and also get the length
  // Append the .cpy to the end of the hash string.
  int len = snprintf(NULL, 0, "%d", hash_file);
  char* cpy = malloc(len + 5);
  snprintf(cpy, len + 5, "%d", hash_file);
  cpy = strcat(cpy, ".cpy");

  char* cpy_path = malloc(strlen(file_path) + 1 + strlen(cpy) + 1);
  sprintf(cpy_path, "%s/%s", file_path, cpy);

  // Open the cpy file and the file being copied from
  FILE* cpyfp = fopen(cpy_path, "w+");
  FILE* origfp = fopen(file_name, "r+");

  // Write the contents of the original file to the copy
  long fsize = get_num_bytes(file_name);
  char* file_contents = malloc((size_t) fsize);

  // Read the contents of the file being copied and write to the cpy file
  fread(file_contents, 1, fsize, origfp);
  fwrite(file_contents, 1, fsize, cpyfp);

  fclose(cpyfp);
  fclose(origfp);
  return;
}

FILE* create_diff_file(int hash_file, char* file_path, char* file_name) {

  // Create a diff file name
  int len = snprintf(NULL, 0, "%d", hash_file);
  char* hash = malloc(len + 7);
  snprintf(hash, len + 1, "%d", hash_file);

  // Create a directory for the diff file
  char* track_dir = malloc(10 + len + 1);
  sprintf(track_dir, "%s/%s", ".svc/HEAD", hash);

  // Create a diff file name.
  hash = strcat(hash, ".diff");

  // Create a path to the diff file for the file_name
  char* diff_path = malloc(strlen(file_path) + 1 + strlen(hash) + 1);
  sprintf(diff_path, "%s/%s", file_path, hash);

  // Create a diff file in the HEAD directory and return the ptr
  FILE* pfp = fopen(diff_path, "w+");

  free(diff_path);
  free(hash);
  return pfp;
}
