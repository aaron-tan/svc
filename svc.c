#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "svc.h"
#include "helper.h"

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
    helper->n_tracked = 0;

    return helper;
}

// Free the helper data structure.
void cleanup(void *helper) {
    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;
    struct file* files = h->tracked_files;
    struct commit* commits = cur->active_commit;

    // Clean up all the commits in the current branch.
    struct commit** all_commits = malloc(sizeof(struct commit*));
    int num_commits = 1;
    if (commits != NULL) {
      all_commits[num_commits - 1] = commits;
      num_commits += 1;

      while (commits->prev_commit != NULL) {
        commits = commits->prev_commit;
        all_commits = realloc(all_commits, num_commits * sizeof(struct commit*));
        all_commits[num_commits - 1] = commits;
        num_commits += 1;
      }

      for (int i = 0; i < (num_commits - 1); i++) {
        free(all_commits[i]->commit_id);
        free(all_commits[i]->commit_msg);

        // Free all the commit files.
        // Temp variable used to free the files.
        struct file* tempfile = NULL;
        struct file* cur_file = all_commits[i]->files;

        while (cur_file != NULL) {
          free(cur_file->name);
          free(cur_file->contents);
          tempfile = cur_file;
          cur_file = cur_file->prev_file;
          free(tempfile);
        }

        free(all_commits[i]);
      }
    }
    // End of cleaning up for all commits.

    /** Clean up for if there is something staging.
    * After calling svc_add without svc_rm there is something
    * in tracked_files, so we clean it up.
    */
    struct file** all_files = malloc(sizeof(struct file*));
    int num_files = 1;

    if (files != NULL) {
      all_files[num_files - 1] = files;
      num_files += 1;

      while (files->prev_file != NULL) {
        files = files->prev_file;
        all_files = realloc(all_files, num_files * sizeof(struct file*));
        all_files[num_files - 1] = files;
        num_files += 1;
      }

      for (int i = 0; i < (num_files - 1); i++) {
        free(all_files[i]->name);
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

    free(all_commits);
    free(all_files);
    free(all_branches);
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
    new_commit->branch_name = cur->name;
    new_commit->commit_msg = malloc(strlen(message) + 1);
    strcpy(new_commit->commit_msg, message);
    // new_commit->files = h->tracked_files;
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

      id_list[*n_prev] = malloc(7);
      strcpy(id_list[*n_prev], cur_com->commit_id);

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

        // Free hash_str.
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
    // Do some initial checks.
    if (branch_name == NULL || check_invalid(branch_name)) {
      return -1;
    }

    struct head* h = (struct head*) helper;
    struct branch* cur = h->cur_branch;

    // Check if there are uncommited changes.
    if (check_modified(helper) == 1) {
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
    if (check_modified(helper) == 1) {
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
    struct file* files = h->tracked_files;

    // Get the hash of the file.
    int h_file = hash_file(helper, file_name);

    // Check if the file has already been added.
    while (files != NULL) {
      // Compare hashes is perhaps more accurate.
      // Use the hashes to determine if a file has been modified.
      if (files->hash == h_file) {
        return -2;
      }
      files = files->prev_file;
    }

    // Think about rewriting this.
    // There are no tracked files.
    // if (h->tracked_files == NULL) {
    //   // Remember to free this later.
    //   h->tracked_files = malloc(sizeof(struct file));
    //
    //   // Populate the file with data.
    //   h->tracked_files->name = file_name;
    //   // Remember to free this later.
    //   h->tracked_files->contents = malloc((bytes + 1));
    //
    //   // Read file contents into tracked_files contents.
    //   if (fread(h->tracked_files->contents, bytes, 1, fp) != 1) {
    //     // If something goes wrong we revert back to the original state.
    //     free(h->tracked_files->contents);
    //     free(h->tracked_files);
    //     h->tracked_files = NULL;
    //     fclose(fp);
    //     return -3;
    //   }
    //   h->tracked_files->contents[bytes] = 0;
    //
    //   h->tracked_files->stat = ADDED;
    //   h->tracked_files->hash = hash_file(helper, file_name);
    //   h->tracked_files->prev_file = NULL;
    //   h->tracked_files->next_file = NULL;
    // } else {
    //   files = h->tracked_files;
    //
    //   // Traversal the doubly linked list until we get to a NULL.
    //   while (files->next_file != NULL) {
    //     files = files->next_file;
    //   }
    //
    // }

    // Track this new file. Remember to free this later.
    struct file* new_file = malloc(sizeof(struct file));
    // Remember to free these later.
    new_file->name = malloc(261);
    strcpy(new_file->name, file_name);
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

    // Get the hash of the file.
    int h_file = hash_file(helper, file_name);

    struct head* h = (struct head*) helper;
    struct file* files = h->tracked_files;

    while (files != NULL) {
      // The file is tracked by the system.
      if (files->hash == h_file && files->stat != REMOVED) {
        // We've gotta get stat to be removed somehow.
        files->stat = REMOVED;
        return files->hash;
        /** Three cases: either file is at the front of the list.
        * middle of the list or
        * rear of the list.
        */
        // if (files->prev_file == NULL) {
        //   // file is at the front of the list.
        //   // Set the next file to be at the front.
        //   if (files->next_file == NULL) {
        //     h->tracked_files = files->next_file;
        //   } else {
        //     files->next_file->prev_file = NULL;
        //   }
        //
        //   // Get its last known hash value.
        //   int last_hash = files->hash;
        //
        //   // Free the file.
        //   free(files->name);
        //   free(files->contents);
        //   free(files);
        //
        //   return last_hash;
        // } else if (files->next_file == NULL) {
        //   // file is at the rear of the list.
        //   // set the prev file to be at the rear.
        //   files->prev_file->next_file = NULL;
        //
        //   // Set current tracked files to the prev file.
        //   h->tracked_files = files->prev_file;
        //
        //   // Get its last known hash value.
        //   int last_hash = files->hash;
        //
        //   // Free the file.
        //   free(files->name);
        //   free(files->contents);
        //   free(files);
        //
        //   return last_hash;
        // } else {
        //   // file is in the middle of the list.
        //   // set the prev file's next file to be next_file.
        //   files->prev_file->next_file = files->next_file;
        //   // set the next file's prev file to be prev_file.
        //   files->next_file->prev_file = files->prev_file;
        //
        //   // Get its last known hash value.
        //   int last_hash = files->hash;
        //
        //   // Free the file.
        //   free(files->name);
        //   free(files->contents);
        //   free(files);
        //
        //   return last_hash;
        // }
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

        // Set the tracked files to be the same as this commit. Possibly have to free previous.
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
    // TODO: Implement
    return NULL;
}
