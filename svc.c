#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "svc.h"
#include "utils/helper.h"
#include "utils/clean.h"
#include "core/track.h"
#include "core/file.h"

// Initialise the data structures and return a ptr to the memory.
void *svc_init(void) {
    // Create a directory containing commit, branch and head information.
    create_dir(".svc/", S_IRWXU);

    // Directory contains commits made by each branch.
    create_dir(".svc/branches", S_IRWXU);

    // Create a directory for the master branch.
    create_dir(".svc/branches/master", S_IRWXU);

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
    helper->n_tracked = 0;
    helper->head_fp = ".svc/HEAD";

    FILE* headp = fopen(helper->head_fp, "w+");
    fwrite("master", 1, 7, headp);
    fclose(headp);

    return helper;
}

// Free the helper data structure.
void cleanup(void *helper) {
    struct head* h = (struct head*) helper;

    // Array to store all branches' commits in.
    int num_commits = 1;
    struct commit** commits_arr = all_commits(helper, &num_commits);

    // Traverse the array and free the commits.
    for (int i = 0; i < (num_commits - 1); i++) {
      free(commits_arr[i]->commit_id);
      free(commits_arr[i]->commit_msg);
      free(commits_arr[i]->branch_name);

      // Free all the commit files.
      struct file* tempfile = NULL;
      struct file* cur_file = commits_arr[i]->files;

      while (cur_file != NULL) {
        free(cur_file->name);
        free(cur_file->contents);
        tempfile = cur_file;
        cur_file = cur_file->prev_file;
        free(tempfile);
      }

      free(commits_arr[i]);
    }
    // End of clean up for all commits for all branches.

    // Clean up all the files in staging.
    int num_files = 1;
    struct file** files_arr = all_files(helper, &num_files);

    for (int i = 0; i < (num_files - 1); i++) {
      free(files_arr[i]->name);
      free(files_arr[i]->contents);
      free(files_arr[i]);
    }
    // End of cleaning up if there is some files left tracked.

    // Clean up for all the branches.
    int num_branches = 1;
    struct branch** branches_arr = all_branches(helper, &num_branches);

    for (int i = 0; i < (num_branches - 1); i++) {
      free(branches_arr[i]->name);
      free(branches_arr[i]);
    }
    // End of cleaning up for all the branches.

    free(commits_arr);
    free(files_arr);
    free(branches_arr);
    free(h);
}

// Calculate the hash value of a file.
int hash_file(void *helper, char *file_path) {
    if (file_path == NULL || helper == NULL) {
      return -1;
    }

    FILE* fp = fopen(file_path, "rb");

    // Check if the file exists.
    if (fp == NULL) {
      return -2;
    }

    int hash = 0;

    // Get the num_bytes.
    int num_bytes = get_num_bytes(file_path);

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
    // Check whether there are no changes since the last commit.
    if (message == NULL || !check_modified(helper)) {
      return NULL;
    }

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;
    struct file* t_files = h->tracked_files;

    // Get the hexadecimal commit id.
    char* hex_id = get_commit_id(helper, message);

    // Create the commit.
    struct commit* new_commit = malloc(sizeof(struct commit));
    new_commit->commit_id = hex_id;

    new_commit->branch_name = malloc(51);
    strcpy(new_commit->branch_name, cur->name);

    new_commit->commit_msg = malloc(strlen(message) + 1);
    strcpy(new_commit->commit_msg, message);

    // Commit the files into the commit.
    new_commit->files = malloc(sizeof(struct file));

    // Use the head and tail to point to first and last of new_commit files.
    struct file* head = NULL;
    struct file* tail = new_commit->files;

    // Deep copy into new_commit files from t_files using head and tail.
    while (t_files != NULL) {
      tail->name = malloc(261);
      strcpy(tail->name, t_files->name);

      tail->contents = malloc(strlen(t_files->contents) + 1);
      strcpy(tail->contents, t_files->contents);

      tail->stat = t_files->stat;
      tail->hash = t_files->hash;

      if (t_files->prev_file == NULL) {
        tail->prev_file = NULL;
        tail->next_file = head;
      } else {
        tail->prev_file = malloc(sizeof(struct file));
        tail->next_file = head;
        head = tail;
      }

      t_files = t_files->prev_file;
      tail = tail->prev_file;
    }

    // The previous for the new commit is the one pointed to by active_commit. Null if this is the first commit.
    new_commit->prev_commit = cur->active_commit;
    new_commit->next_commit = NULL;

    // We want the previous commit's next_commit to point to new_commit but not the first one.
    if (cur->active_commit != NULL) {
      // Free the next_commit first next_commit is not null.
      if (cur->active_commit->next_commit != NULL) {
        struct commit* tobfreed = cur->active_commit->next_commit;
        free(tobfreed->commit_id);
        free(tobfreed->commit_msg);
        free(tobfreed->branch_name);

        // Temp variable used to free the files.
        struct file* tempfile = NULL;
        struct file* cur_file = tobfreed->files;

        while (cur_file != NULL) {
          free(cur_file->name);
          free(cur_file->contents);
          tempfile = cur_file;
          cur_file = cur_file->prev_file;
          free(tempfile);
        }

        free(tobfreed);
      }

      cur->active_commit->next_commit = new_commit;
    }

    // Set the active_commit to be the new commit.
    cur->active_commit = new_commit;

    return hex_id;
}

void *get_commit(void *helper, char *commit_id) {
    if (commit_id == NULL) {
      return NULL;
    }

    struct head* h = (struct head*) helper;
    struct commit* cur_commit = h->cur_branch->active_commit;

    while (cur_commit != NULL) {
      if (strcmp(cur_commit->commit_id, commit_id) == 0) {
        return cur_commit;
      }

      // Keep traversing the list.
      cur_commit = cur_commit->prev_commit;
    }

    return NULL;
}

char **get_prev_commits(void *helper, void *commit, int *n_prev) {
    if (n_prev == NULL || commit == NULL) {
      if (n_prev == NULL) {
        return NULL;
      } else {
        *n_prev = 0;
        return NULL;
      }
    }

    struct commit* cur_com = ((struct commit*)commit)->prev_commit;
    char** id_list = NULL;
    *n_prev = 0;

    while (cur_com != NULL) {
      // We add this to the list.
      // If id_list is null we start a new list.
      if (id_list == NULL) {
        id_list = malloc(sizeof(char*));
      }

      id_list[*n_prev] = cur_com->commit_id;
      // strcpy(id_list[*n_prev], cur_com->commit_id);

      *n_prev += 1;
      id_list = realloc(id_list, (*n_prev + 1) * sizeof(char*));

      cur_com = cur_com->prev_commit;
    }

    return id_list;
}

void print_commit(void *helper, char *commit_id) {
    if (commit_id == NULL) {
      puts("Invalid commit id");
      return;
    }

    struct head* h = (struct head*) helper;
    struct commit* cur_commit = h->cur_branch->active_commit;
    struct file* t_file = h->tracked_files;

    while (cur_commit != NULL) {
      // Commit exists, print commit.
      if (strcmp(cur_commit->commit_id, commit_id) == 0) {
        struct file* com_file = cur_commit->files;
        int n_files = 0;

        // Print the first line.
        printf("%s [%s]: %s\n", cur_commit->commit_id, cur_commit->branch_name, cur_commit->commit_msg);

        while (com_file != NULL) {
          if (com_file->stat == ADDED || com_file->stat == ALRDY_ADD) {
            printf("    + %s\n", com_file->name);
          } else if (com_file->stat == REMOVED) {
            printf("    - %s\n", com_file->name);
          } else if (com_file->stat == MODIFIED) {
            printf("    / %s [%d]\n", com_file->name, com_file->hash);
          }

          n_files += 1;
          com_file = com_file->prev_file;
        }

        printf("\n    Tracked files (%d):\n", h->n_tracked);

        // Get the hash length and hash as a string.
        int hash_len;
        char* hash_str = NULL;

        // Traverse all the tracked files.
        while (t_file != NULL) {
          // Get the length of the hash so we can malloc memory.
          hash_len = snprintf(NULL, 0, "%d", t_file->hash);
          hash_str = realloc(hash_str, hash_len + 1);

          // Convert the int hash into str hash.
          snprintf(hash_str, hash_len + 1, "%d", t_file->hash);

          printf("    [%10s] %s\n", hash_str, t_file->name);

          t_file = t_file->prev_file;
        }

        free(hash_str);

        return;
      }

      cur_commit = cur_commit->prev_commit;
    }

    puts("Invalid commit id");
    return;
}

// Creates a new branch.
int svc_branch(void *helper, char *branch_name) {
    // Do some initial checks and check if branch name is invalid.
    if (branch_name == NULL || check_invalid(branch_name)) {
      return -1;
    }

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    // Check if there are uncommited changes.
    if (check_modified(helper) == 1 || check_modified(helper) == 2) {
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
        new_branch->active_commit = h->cur_branch->active_commit;
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
    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    if (branch_name == NULL || !branch_exist(helper, branch_name)) {
      return -1;
    }

    // If there are uncommitted changes return -2.
    if (check_modified(helper) == 1 || check_modified(helper) == 2) {
      return -2;
    }

    do {
      if (strcmp(cur->name, branch_name) == 0) {
        h->cur_branch = cur;
        break;
      }
      cur = cur->next_branch;
    } while (cur != h->cur_branch);

    return 0;
}

char **list_branches(void *helper, int *n_branches) {
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

    // Get the current branch.
    char* curb = current_branch(helper);
    char* file_path = malloc(14 + strlen(curb) + 1);
    // file_path = ".svc/branches";
    sprintf(file_path, "%s/%s", ".svc/branches", curb);

    // Track the file
    int hash = hash_file(helper, file_name);

    // Check if file has already been added
    /** First we convert the hash into a string so we can compare it
    *   as a substring in the list of file names */
    char* str_hash = malloc(snprintf(NULL, 0, "%d", hash) + 1);
    snprintf(str_hash, snprintf(NULL, 0, "%d", hash) + 1, "%d", hash);
    int ls_len = 0;
    char** file_list = ls_dir(file_path, &ls_len);

    for (int i = 0; i < ls_len; i++) {
      if (strstr(file_list[i], str_hash) != NULL) {
        // The file has been added before, return -2
        free(file_path);
        free(str_hash);
        return -2;
      }
    }

    FILE* tracker = create_diff_file(hash, file_path, file_name);

    // Create a copy of the file
    copy_file(hash, file_path, file_name);

    FILE* fp = fopen(file_name, "rb");
    int bytes = get_num_bytes(file_name);

    // Add to the system.
    struct head* h = (struct head*) helper;
    struct file* files = h->tracked_files;

    // Get the hash of the file.
    int h_file = hash_file(helper, file_name);

    // Check if the file has already been added.
    while (files != NULL) {
      // Use the hashes to determine if a file has been modified.
      if (files->hash == h_file) {
        return -2;
      }
      files = files->prev_file;
    }

    // Track this new file. Remember to free this later.
    struct file* new_file = malloc(sizeof(struct file));
    // Remember to free these later.
    new_file->name = malloc(261);
    strcpy(new_file->name, file_name);
    new_file->contents = malloc((bytes + 1));

    fread(new_file->contents, bytes, 1, fp);

    new_file->contents[bytes] = 0;

    new_file->stat = ADDED;
    new_file->hash = hash_file(helper, file_name);
    new_file->prev_file = h->tracked_files;
    new_file->next_file = NULL;

    // We want the previous file's next_file to also point to new_file. But not the first added file.
    if (h->tracked_files != NULL) {
      h->tracked_files->next_file = new_file;
    }

    // tracked_files will always be pointing to the latest new tracked file.
    h->tracked_files = new_file;
    h->n_tracked += 1;

    fclose(fp);

    return hash_file(helper, file_name);
}

int svc_rm(void *helper, char *file_name) {
    if (file_name == NULL) {
      return -1;
    }
    int h_file = hash_file(helper, file_name);
    // Get the hash of the file and convert it to a string
    char* str_hash = hash2str(hash_file(helper, file_name));

    // Get the path of the current branch.
    char* curb_path = get_curb_path(helper);
    int ls_len = 0;
    char** file_list = ls_dir(curb_path, &ls_len);

    for (int i = 0; i < ls_len; i++) {
      if (strstr(file_list[i], str_hash) != NULL) {
        // We have found the diff and cpy files,
        // Append file name to the current branch path
        char* rem_filepath = malloc(strlen(curb_path) + strlen(file_list[i]) + 2);
        sprintf(rem_filepath, "%s/%s", curb_path, file_list[i]);

        // Remove the file
        remove(rem_filepath);

        free(curb_path);
        free(str_hash);
        return hash_file(helper, file_name);
      }
    }

    struct head* h = (struct head*) helper;
    struct file* files = h->tracked_files;

    while (files != NULL) {
      // The file is tracked by the system.
      if (files->hash == h_file && files->stat != REMOVED) {
        // We've gotta get stat to be removed somehow.
        files->stat = REMOVED;
        return files->hash;
      }

      files = files->prev_file;
    }

    // File with the given name is not being tracked.
    return -2;
}

int svc_reset(void *helper, char *commit_id) {
    if (commit_id == NULL) {
      return -1;
    }

    struct head* h = (struct head*) helper;
    struct commit* cur_commit = h->cur_branch->active_commit;

    while (cur_commit != NULL) {
      // If the cur commit id is what we want to find reset to this.
      if (strcmp(cur_commit->commit_id, commit_id) == 0) {
        // Reset the current branch to this commit.
        h->cur_branch->active_commit = cur_commit;

        // We free the previously tracked files first.
        if (h->tracked_files != NULL) {
          struct file* tempfile = NULL;
          struct file* cur_file = h->tracked_files;

          while (cur_file != NULL) {

            free(cur_file->name);
            free(cur_file->contents);
            tempfile = cur_file;
            cur_file = cur_file->prev_file;
            free(tempfile);

          }
        }

        // Set the tracked files to be the same as this commit.
        h->tracked_files = malloc(sizeof(struct file));

        // Use the head and tail to point to first and last of files.
        struct file* head = NULL;
        struct file* tail = h->tracked_files;

        struct file* cur_file = cur_commit->files;
        while (cur_file != NULL) {
          // Write the file contents of this commit to disk.
          FILE* fp = fopen(cur_file->name, "w");
          fputs(cur_file->contents, fp);
          fclose(fp);

          // Deep copy into tracked files.
          tail->name = malloc(261);
          strcpy(tail->name, cur_file->name);

          tail->contents = malloc(strlen(cur_file->contents) + 1);
          strcpy(tail->contents, cur_file->contents);

          tail->stat = cur_file->stat;
          tail->hash = cur_file->hash;

          if (cur_file->prev_file == NULL) {
            tail->prev_file = NULL;
            tail->next_file = head;
          } else {
            tail->prev_file = malloc(sizeof(struct file));
            tail->next_file = head;
            head = tail;
          }

          cur_file = cur_file->prev_file;
          tail = tail->prev_file;
        }

        return 0;
      }

      cur_commit = cur_commit->prev_commit;
    }

    return -2;
}

char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {
    struct head* h = (struct head*) helper;
    struct branch* cur_branch = h->cur_branch;

    if (branch_name == NULL) {
      puts("Invalid branch name");
      return NULL;
    } else if (!branch_exist(helper, branch_name)) {
      puts("Branch not found");
      return NULL;
    } else if (strcmp(cur_branch->name, branch_name) == 0) {
      puts("Cannot merge a branch with itself");
      return NULL;
    } else if (check_modified(helper) == 1 || check_modified(helper) == 2) {
      puts("Changes must be committed");
      return NULL;
    }
    // Had no time left to complete this function. So only the bare checks were performed.

    return NULL;
}
