#!/bin/bash
# This script generates large function intended to
# cause as many iTLB misses as possible.

# Number of instructions:
# 4k - page size
# x 64 - supposed number of TLB entires
# x 2 - executing a function sized page_size * tlb_entry_count multiple
# times would cause tlb misses only on the first call and tlb entries
# would be valid for each next call. Doubling the size of the function
# guarantees invalidating tlb entires and thus causing tlb misses.

echo "void iTLB_bechmark_function() {"
echo "  int a = 0, b = 0;"

for (( c=0; c < (1 << 18) ; c++ )) ; do
  echo "  a = b + 1;"
  echo "  b = a + 1;"
done

echo "}"
