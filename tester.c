#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "svc.h"

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

  do {
    // assert(strcmp(master->name, "master") == 0 || strcmp(master->name, "example") == 0);
    // printf("%s\n", master->name);
    master = master->next_branch;
  } while (master != h->cur_branch);

  return 0;
}

int test_list_branches(void* helper) {
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

int test_svc_add(void* helper) {
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
  assert(strcmp(h->tracked_files->name, "hello.py") == 0);
  printf("%s", h->tracked_files->contents);
  assert(h->tracked_files->stat == ADDED);
  assert(h->tracked_files->prev_file == NULL);
  assert(h->tracked_files->next_file != NULL);

  // Get Tests/test1.in
  struct file* test1 = h->tracked_files->next_file;

  assert(strcmp(test1->name, "Tests/test1.in") == 0);
  printf("%s", test1->contents);
  assert(test1->stat == ADDED);
  assert(test1->prev_file == h->tracked_files);
  assert(test1->next_file == NULL);

  return 0;
}

int main() {
    void *helper = svc_init();

    assert(svc_branch(helper, "example_branch/branch-10") == 0);
    assert(svc_branch(helper, "example") == 0);
    assert(svc_branch(helper, "good-branch") == 0);

    assert(svc_branch(helper, "example") == -2);
    assert(svc_branch(helper, "example_branch/branch-10") == -2);
    assert(svc_branch(helper, "good-branch") == -2);
    assert(test_svc_branch(helper) == 0);

    // assert(test_list_branches(helper) == 0);

    // Tests for svc_add.
    assert(test_svc_add(helper) == 0);
    // Tests for svc_checkout.

    cleanup(helper);

    return 0;
}
