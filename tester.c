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

  assert(svc_branch(helper, "example_branch/branch-10") == 0);
  assert(svc_branch(helper, "example") == 0);
  assert(svc_branch(helper, "good-branch") == 0);

  assert(svc_branch(helper, "example") == -2);
  assert(svc_branch(helper, "example_branch/branch-10") == -2);
  assert(svc_branch(helper, "good-branch") == -2);

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

int test_svc_add_example_2(void* helper) {
  assert(svc_add(helper, "COMP2017/svc.h") == 5007);
  assert(svc_add(helper, "COMP2017/svc.c") == 5217);
  assert(svc_add(helper, "hello.py") == -2);
  assert(svc_add(helper, "Tests/test1.in") == -2);

  struct head* h = (struct head*) helper;
  struct file* files = h->tracked_files;

  while (files != NULL) {
    printf("%s\n", files->name);
    files = files->next_file;
  }

  return 0;
}

int test_svc_checkout(void* helper) {
  assert(svc_checkout(helper, "example") == -1);

  struct head* h = (struct head*) helper;
  struct branch* b = h->cur_branch;

  assert(strcmp(b->name, "example") == 0);
  printf("Checkout branch: %s\n", b->name);

  return 0;
}

int main() {
    void *helper = svc_init();

    assert(test_svc_branch(helper) == 0);

    assert(test_list_branches(helper) == 0);

    // Tests for svc_checkout.
    assert(test_svc_checkout(helper) == 0);

    // Tests for svc_add.
    assert(test_svc_add_example_1(helper) == 0);
    assert(test_svc_add_example_2(helper) == 0);


    cleanup(helper);

    return 0;
}
