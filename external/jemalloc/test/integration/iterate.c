#include "test/jemalloc_test.h"

/* Tests je_iterate added by src/android_je_iterate.c */

int je_iterate(uintptr_t, size_t, void (*)(uintptr_t, size_t, void*), void*);

static size_t alloc_count;
static size_t alloc_size;
static uintptr_t alloc_find;
static size_t alloc_find_size;
static bool alloc_found;

static void callback(uintptr_t ptr, size_t size, void* arg) {
  alloc_count++;
  alloc_size += size;
  if (ptr <= alloc_find && alloc_find < ptr + size) {
    assert(alloc_find + alloc_find_size <= ptr + size);
    alloc_found = true;
  }
}

TEST_BEGIN(test_iterate_alloc)
{

#define MAXSZ (((size_t)1) << 26)
  size_t sz;

  for (sz = 1; sz < MAXSZ; sz <<= 1) {
    void *ptr;
    ptr = malloc(sz);
    assert_ptr_not_null(ptr, "malloc() failed for size %zu", sz);

    alloc_count = 0;
    alloc_size = 0;
    alloc_find = (uintptr_t)ptr;
    alloc_find_size = sz;
    alloc_found = false;

    mallctl("thread.tcache.flush", NULL, NULL, NULL, 0);

    assert(je_iterate((uintptr_t)ptr, sz, callback, NULL) == 0);

    assert(alloc_found);

    free(ptr);
  }
#undef MAXSZ
}
TEST_END

TEST_BEGIN(test_iterate_dalloc)
{

#define MAXSZ (((size_t)1) << 26)
  size_t sz;

  for (sz = 1; sz < MAXSZ; sz <<= 1) {
    void *ptr;
    ptr = malloc(sz);
    free(ptr);
    assert_ptr_not_null(ptr, "malloc() failed for size %zu", sz);

    alloc_count = 0;
    alloc_size = 0;
    alloc_find = (uintptr_t)ptr;
    alloc_find_size = sz;
    alloc_found = false;

    mallctl("thread.tcache.flush", NULL, NULL, NULL, 0);

    je_iterate((uintptr_t)ptr, sz, callback, NULL);

    assert(!alloc_found);
  }
#undef MAXSZ
}
TEST_END

TEST_BEGIN(test_iterate_free_first)
{
#define MAXSZ (((size_t)1) << 26)
  size_t sz;

  for (sz = 1; sz < MAXSZ; sz <<= 1) {
    void *ptr;
    void *ptr2;
    ptr2 = malloc(sz);
    assert_ptr_not_null(ptr2, "malloc() failed for size %zu", sz);

    ptr = malloc(sz);
    assert_ptr_not_null(ptr, "malloc() failed for size %zu", sz);

    free(ptr2);

    alloc_count = 0;
    alloc_size = 0;
    alloc_find = (uintptr_t)ptr;
    alloc_find_size = sz;
    alloc_found = false;

    mallctl("thread.tcache.flush", NULL, NULL, NULL, 0);

    assert(je_iterate((uintptr_t)ptr, sz, callback, NULL) == 0);

    assert(alloc_found);

    free(ptr);
  }
#undef MAXSZ
}
TEST_END

int
main(void)
{

  return (test(
      test_iterate_alloc,
      test_iterate_dalloc,
      test_iterate_free_first));
}
