#!/bin/bash
# This script generates an assembly function that jumps over blocks of size
# determined by its first argument. Number of such blocks is determined
# by the second argument.

PAGE_SIZE=$(( $1 ))
TLB_ENTRY_CNT=$(( $2 ))

function instruction_block {
  for (( c=0; c < PAGE_SIZE ; c++ )) ; do
    echo '    "nop\n\t"'
  done
}

echo 'void iTLB_bechmark_function() {'
echo '  __asm__ ('

for (( i=0; i < TLB_ENTRY_CNT; i++ )) ; do
   echo '    "1:jmp 1f\n\t"'
   instruction_block
done
echo '    "1:nop\n\t"'
echo '  );'
echo '}'
