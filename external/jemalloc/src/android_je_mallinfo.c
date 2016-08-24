/*
 * Copyright (C) 2014 The Android Open Source Project
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

/* This is an implementation that uses the same arena access pattern found
 * in the arena_stats_merge function from src/arena.c.
 */
struct mallinfo je_mallinfo() {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(&arenas_lock);
  for (unsigned i = 0; i < narenas_auto; i++) {
    if (arenas[i] != NULL) {
      malloc_mutex_lock(&arenas[i]->lock);
      mi.hblkhd += arenas[i]->stats.mapped;
      mi.uordblks += arenas[i]->stats.allocated_large;
      mi.uordblks += arenas[i]->stats.allocated_huge;
      malloc_mutex_unlock(&arenas[i]->lock);

      for (unsigned j = 0; j < NBINS; j++) {
        arena_bin_t* bin = &arenas[i]->bins[j];

        malloc_mutex_lock(&bin->lock);
        mi.uordblks += arena_bin_info[j].reg_size * bin->stats.curregs;
        malloc_mutex_unlock(&bin->lock);
      }
    }
  }
  malloc_mutex_unlock(&arenas_lock);
  mi.fordblks = mi.hblkhd - mi.uordblks;
  mi.usmblks = mi.hblkhd;
  return mi;
}

size_t __mallinfo_narenas() {
  return narenas_auto;
}

size_t __mallinfo_nbins() {
  return NBINS;
}

struct mallinfo __mallinfo_arena_info(size_t aidx) {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(&arenas_lock);
  if (aidx < narenas_auto) {
    if (arenas[aidx] != NULL) {
      malloc_mutex_lock(&arenas[aidx]->lock);
      mi.hblkhd = arenas[aidx]->stats.mapped;
      mi.ordblks = arenas[aidx]->stats.allocated_large;
      mi.uordblks = arenas[aidx]->stats.allocated_huge;
      malloc_mutex_unlock(&arenas[aidx]->lock);

      for (unsigned j = 0; j < NBINS; j++) {
        arena_bin_t* bin = &arenas[aidx]->bins[j];

        malloc_mutex_lock(&bin->lock);
        mi.fsmblks += arena_bin_info[j].reg_size * bin->stats.curregs;
        malloc_mutex_unlock(&bin->lock);
      }
    }
  }
  malloc_mutex_unlock(&arenas_lock);
  return mi;
}

struct mallinfo __mallinfo_bin_info(size_t aidx, size_t bidx) {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(&arenas_lock);
  if (aidx < narenas_auto && bidx < NBINS) {
    if (arenas[aidx] != NULL) {
      arena_bin_t* bin = &arenas[aidx]->bins[bidx];

      malloc_mutex_lock(&bin->lock);
      mi.ordblks = arena_bin_info[bidx].reg_size * bin->stats.curregs;
      mi.uordblks = bin->stats.nmalloc;
      mi.fordblks = bin->stats.ndalloc;
      malloc_mutex_unlock(&bin->lock);
    }
  }
  malloc_mutex_unlock(&arenas_lock);
  return mi;
}
