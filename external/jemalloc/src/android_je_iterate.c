/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

static pthread_mutex_t malloc_disabled_lock = PTHREAD_MUTEX_INITIALIZER;
static bool malloc_disabled_tcache;

static void je_iterate_chunk(arena_chunk_t *chunk,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg);
static void je_iterate_small(arena_run_t *run,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg);

/* je_iterate calls callback for each allocation found in the memory region
 * between [base, base+size).  base will be rounded down to by the jemalloc
 * chunk size, and base+size will be rounded up to the chunk size.  If no memory
 * managed by jemalloc is found in the requested region, je_iterate returns -1
 * and sets errno to EINVAL.
 *
 * je_iterate must be called when no allocations are in progress, either
 * when single-threaded (for example just after a fork), or between
 * jemalloc_prefork() and jemalloc_postfork_parent().  The callback must
 * not attempt to allocate with jemalloc.
 */
int je_iterate(uintptr_t base, size_t size,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg) {

  int error = EINVAL;
  uintptr_t ptr = (uintptr_t)CHUNK_ADDR2BASE(base);
  uintptr_t end = CHUNK_CEILING(base + size);

  while (ptr < end) {
    assert(ptr == (uintptr_t)CHUNK_ADDR2BASE(ptr));
    extent_node_t *node;

    node = chunk_lookup((void *)ptr, false);
    if (node == NULL) {
      ptr += chunksize;
      continue;
    }

    assert(extent_node_achunk_get(node) ||
        (uintptr_t)extent_node_addr_get(node) == ptr);

    error = 0;
    if (extent_node_achunk_get(node)) {
      /* Chunk */
      arena_chunk_t *chunk = (arena_chunk_t *)ptr;
      ptr += chunksize;

      if (&chunk->node != node) {
          /* Empty retained chunk */
          continue;
      }

      je_iterate_chunk(chunk, callback, arg);
    } else if ((uintptr_t)extent_node_addr_get(node) == ptr) {
      /* Huge allocation */
      callback(ptr, extent_node_size_get(node), arg);
      ptr = CHUNK_CEILING(ptr + extent_node_size_get(node));
    }
  }

  if (error) {
    set_errno(error);
    return -1;
  }

  return 0;
}

/* Iterate over a valid jemalloc chunk, calling callback for each large
 * allocation run, and calling je_iterate_small for each small allocation run */
static void je_iterate_chunk(arena_chunk_t *chunk,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg) {
  size_t pageind;

  pageind = map_bias;

  while (pageind < chunk_npages) {
    size_t mapbits;
    size_t size;

    mapbits = arena_mapbits_get(chunk, pageind);
    if (!arena_mapbits_allocated_get(chunk, pageind)) {
      /* Unallocated run */
      size = arena_mapbits_unallocated_size_get(chunk, pageind);
    } else if (arena_mapbits_large_get(chunk, pageind)) {
      /* Large allocation run */
      void *rpages;

      size = arena_mapbits_large_size_get(chunk, pageind);
      rpages = arena_miscelm_to_rpages(arena_miscelm_get(chunk, pageind));
      callback((uintptr_t)rpages, size, arg);
    } else {
      /* Run of small allocations */
      szind_t binind;
      arena_run_t *run;

      assert(arena_mapbits_small_runind_get(chunk, pageind) == pageind);
      binind = arena_mapbits_binind_get(chunk, pageind);
      run = &arena_miscelm_get(chunk, pageind)->run;
      assert(run->binind == binind);
      size = arena_bin_info[binind].run_size;

      je_iterate_small(run, callback, arg);
    }
    assert(size == PAGE_CEILING(size));
    assert(size > 0);
    pageind += size >> LG_PAGE;
  }

}

/* Iterate over a valid jemalloc small allocation run, calling callback for each
 * active allocation. */
static void je_iterate_small(arena_run_t *run,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg) {
  szind_t binind;
  const arena_bin_info_t *bin_info;
  uint32_t regind;
  uintptr_t ptr;
  void *rpages;

  binind = run->binind;
  bin_info = &arena_bin_info[binind];
  rpages = arena_miscelm_to_rpages(arena_run_to_miscelm(run));
  ptr = (uintptr_t)rpages + bin_info->reg0_offset;

  for (regind = 0; regind < bin_info->nregs; regind++) {
    if (bitmap_get(run->bitmap, &bin_info->bitmap_info, regind)) {
      callback(ptr, bin_info->reg_size, arg);
    }
    ptr += bin_info->reg_interval;
  }
}

static void je_malloc_disable_prefork() {
  pthread_mutex_lock(&malloc_disabled_lock);
}

static void je_malloc_disable_postfork_parent() {
  pthread_mutex_unlock(&malloc_disabled_lock);
}

static void je_malloc_disable_postfork_child() {
  pthread_mutex_init(&malloc_disabled_lock, NULL);
}

void je_malloc_disable_init() {
  if (pthread_atfork(je_malloc_disable_prefork,
      je_malloc_disable_postfork_parent, je_malloc_disable_postfork_child) != 0) {
    malloc_write("<jemalloc>: Error in pthread_atfork()\n");
    if (opt_abort)
      abort();
  }
}

void je_malloc_disable() {
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, je_malloc_disable_init);

  pthread_mutex_lock(&malloc_disabled_lock);
  bool new_tcache = false;
  size_t old_len = sizeof(malloc_disabled_tcache);
  je_mallctl("thread.tcache.enabled",
      &malloc_disabled_tcache, &old_len,
      &new_tcache, sizeof(new_tcache));
  jemalloc_prefork();
}

void je_malloc_enable() {
  jemalloc_postfork_parent();
  if (malloc_disabled_tcache) {
    je_mallctl("thread.tcache.enabled", NULL, NULL,
        &malloc_disabled_tcache, sizeof(malloc_disabled_tcache));
  }
  pthread_mutex_unlock(&malloc_disabled_lock);
}
