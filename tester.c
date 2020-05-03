#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "svc.h"
#include "helper.h"

int test_svc_init(void* helper) {
  struct head* h = (struct head*) helper;
  struct branch* master = h->cur_branch;
  assert(strcmp(master->name, "master") == 0);
  assert(master->active_commit == NULL);
  assert(master == master->next_branch);

  assert(((struct head*)helper)->cur_branch == master);
  assert(((struct head*)helper)->tracked_files == NULL);

  return 0;
}

int test_svc_branch(void* helper) {
  struct head* h = (struct head*) helper;
  struct branch* master = h->cur_branch;

  assert(svc_branch(helper, "example_branch/branch-10") == 0);
  assert(svc_branch(helper, "example") == 0);
  assert(svc_branch(helper, "good-branch") == 0);

  assert(svc_branch(helper, "example") == -2);
  assert(svc_branch(helper, "example_branch/branch-10") == -2);
  assert(svc_branch(helper, "good-branch") == -2);

  return 0;
}

int test_list_branches(void* helper) {
  svc_add(helper, "hello.py");
  svc_add(helper, "Tests/test1.in");
  svc_add(helper, "COMP2017/svc.c");

  int* num_branches = malloc(sizeof(int));
  char** list = list_branches(helper, num_branches);
  assert(*num_branches == 4);

  for (int i = 0; i < *num_branches; i++) {
    printf("From list: %s\n", list[i]);
    assert(strcmp(list[i], "master") == 0 ||
    strcmp(list[i], "example_branch/branch-10") == 0 ||
    strcmp(list[i], "example") == 0 ||
    strcmp(list[i], "good-branch") == 0);
  }

  return 0;
}

int test_svc_add_example_1(void* helper) {
  assert(svc_add(helper, "hello.py") == 2027);
  assert(svc_add(helper, "hello.py") == -2);

  // Do checks on helper itself.
  struct head* h = (struct head*) helper;
  assert(strcmp(h->tracked_files->name, "hello.py") == 0);
  printf("%s", h->tracked_files->contents);
  assert(h->tracked_files->stat == ADDED);
  assert(h->tracked_files->prev_file == NULL);
  assert(h->tracked_files->next_file == NULL);

  assert(svc_add(helper, "Tests/test1.in") == 564);

  // Do checks again after adding Tests/test1.in
  assert(strcmp(h->tracked_files->name, "Tests/test1.in") == 0);
  printf("%s", h->tracked_files->contents);
  assert(h->tracked_files->stat == ADDED);
  // assert(h->tracked_files->prev_file == h->tracked_files);
  assert(h->tracked_files->next_file == NULL);

  struct file* hello = h->tracked_files->prev_file;
  assert(strcmp(hello->name, "hello.py") == 0);
  printf("%s", hello->contents);
  assert(hello->stat == ADDED);
  assert(hello->prev_file == NULL);
  assert(hello->next_file == h->tracked_files);

  // assert(strcmp(h->tracked_files->name, "hello.py") == 0);
  // printf("%s", h->tracked_files->contents);
  // assert(h->tracked_files->stat == ADDED);
  // assert(h->tracked_files->prev_file == NULL);
  // assert(h->tracked_files->next_file != NULL);

  // Get Tests/test1.in
  // struct file* test1 = h->tracked_files->next_file;


  return 0;
}

int test_svc_add_example_2(void* helper) {
  assert(svc_add(helper, "COMP2017/svc.h") == 5007);
  assert(svc_add(helper, "COMP2017/svc.c") == 5217);

  struct head* h = (struct head*) helper;
  struct file* files = h->tracked_files;

  while (files != NULL) {
    printf("In test: %s\n", files->name);
    printf("In test: %s\n", files->contents);
    files = files->prev_file;
  }

  return 0;
}

int test_svc_remove(void* helper) {
  svc_add(helper, "hello.py");
  svc_add(helper, "Tests/test1.in");
  svc_add(helper, "COMP2017/svc.c");
  svc_add(helper, "COMP2017/svc.h");

  // assert(svc_rm(helper, "COMP2017/svc.h") == 5007);
  assert(svc_rm(helper, "Tests/test1.in") == 564);

  // Test file !being tracked.
  assert(svc_rm(helper, "whatever.c") == -2);

  struct head* h = (struct head*) helper;
  struct file* files = h->tracked_files;

  while (files != NULL) {
    printf("In test svc remove: %s\n", files->name);
    printf("In test svc remove: %s", files->contents);
    files = files->prev_file;
  }

  return 0;
}

int test_svc_checkout(void* helper) {
  assert(svc_checkout(helper, "example") == 0);

  struct head* h = (struct head*) helper;
  struct branch* b = h->cur_branch;

  assert(strcmp(b->name, "example") == 0);
  printf("Checkout branch: %s\n", b->name);

  return 0;
}

int test_example_1(void* helper) {
  // printf("%d\n", hash_file(helper, "hello.py"));
  assert(hash_file(helper, "hello.py") == 2027);

  assert(hash_file(helper, "fake.c") == -2);

  assert(svc_commit(helper, "No changes") == NULL);

  // Malloc hello and test strings.
  char* hello = malloc(20);
  strcpy(hello, "hello.py");

  char* test = malloc(20);
  strcpy(test, "Tests/test1.in");
  // End of malloc

  // Hello.py
  assert(svc_add(helper, hello) == 2027);

  strcpy(hello, "it");

  // Tests\test1.in
  assert(svc_add(helper, test) == 564);

  strcpy(test, "it");

  assert(svc_add(helper, test) == -3);

  char* id = svc_commit(helper, "Initial commit");

  assert(strcmp(id, "74cde7") == 0);

  void* commit = get_commit(helper, "74cde7");

  int n_prev;
  char** prev_commits = get_prev_commits(helper, commit, &n_prev);
  assert(prev_commits == NULL);
  assert(n_prev == 0);

  print_commit(helper, "74cde7");

  int n;
  char** branches = list_branches(helper, &n);

  assert(n == 1);

  printf("%s\n", branches[n - 1]);

  return 0;
}

int test_example_2(void* helper) {
  // Starting from a blank project, create two files and add them.
  // Hash files are correct.
  assert(svc_add(helper, "COMP2017/svc.c") == 5217);
  assert(svc_add(helper, "COMP2017/svc.h") == 5007);

  char* commid = svc_commit(helper, "Initial commit");
  printf("%s\n", commid);
  assert(strcmp(commid, "7b3e30") == 0);

  assert(svc_branch(helper, "random_branch") == 0);

  assert(svc_checkout(helper, "random_branch") == 0);

  FILE * f = fopen("COMP2017/svc.c", "w");
  fputs("#include \"svc.h\"\nvoid *svc_init(void) {\n    return NULL;\n}\n", f);
  fclose(f);

  assert(svc_rm(helper, "COMP2017/svc.h") == 5007);

  char* init_id = svc_commit(helper, "Implemented svc_init");
  printf("Init_id: %s\n", init_id);
  assert(strcmp(init_id, "73eacd") == 0);

  // You realise you accidentally deleted svc.h and want to revert to initial commit.
  remove("COMP2017/svc.h");

  assert(svc_reset(helper, "7b3e30") == 0);

  // Then the file COMP2017/svc.c is changed again to have the contents shown above.
  FILE * fileptr = fopen("COMP2017/svc.c", "w");
  fputs("#include \"svc.h\"\nvoid *svc_init(void) {\n    return NULL;\n}\n", fileptr);
  fclose(fileptr);

  char* after_reset = svc_commit(helper, "Implemented svc_init");
  printf("After reset: %s\n", after_reset);
  assert(strcmp(after_reset, "24829b") == 0);

  void* commit = get_commit(helper, "24829b");

  int n_prev;
  char** prev_commits = get_prev_commits(helper, commit, &n_prev);

  assert(n_prev == 1);
  for (int i = 0; i < n_prev; i++) {
    printf("Get prev commits: %s\n", prev_commits[i]);
    assert(strcmp(prev_commits[i], "7b3e30") == 0);
  }

  assert(svc_checkout(helper, "master") == 0);

  free(prev_commits);

  return 0;
}

int test_uncommitted(void* helper) {
  assert(svc_add(helper, "COMP2017/svc.c") == 5217);
  assert(svc_add(helper, "COMP2017/svc.h") == 5007);

  char* commid = svc_commit(helper, "Initial commit");
  printf("%s\n", commid);
  assert(strcmp(commid, "7b3e30") == 0);

  FILE * f = fopen("COMP2017/svc.c", "w");
  fputs("#include \"svc.h\"\nvoid *svc_init(void) {\n    return NULL;\n}\n", f);
  fclose(f);

  assert(svc_branch(helper, "random_branch") == -3);

  return 0;
}

void rewrite() {
  // Sleep before regenerating files.
  // sleep(10);
  FILE * svc_cfp = fopen("COMP2017/svc.c", "w");
  fputs("#include \"svc.h\"\nvoid *svc_init(void) {\n    // TODO: implement\n}\n", svc_cfp);
  fclose(svc_cfp);

  FILE* svc_hfp = fopen("COMP2017/svc.h", "w");
  fputs("#ifndef svc_h\n#define svc_h\nvoid *svc_init(void);\n#endif\n", svc_hfp);
  fclose(svc_hfp);
}

int main() {
    void *helper = svc_init();

    // assert(test_example_1(helper) == 0);
    assert(test_example_2(helper) == 0);
    // assert(test_uncommitted(helper) == 0);

    rewrite();

    cleanup(helper);

    return 0;
}
