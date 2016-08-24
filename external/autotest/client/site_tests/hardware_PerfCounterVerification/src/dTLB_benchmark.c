#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  unsigned long i, block_cnt = 100;
  char** blocks;
  long page_size;

  page_size = sysconf(_SC_PAGESIZE);
  if (page_size == -1) {
    page_size = (1 << 12); // 4Kb
  }

  if (argc > 1) {
    block_cnt = strtoul(argv[1], NULL, 10);
    if (block_cnt < 1) {
      block_cnt = 1;
    }
  }

  blocks = (char**) malloc(block_cnt * sizeof(char*));
  for (i = 0; i < block_cnt; i++) {
    char* dummy_ptr = (char*) malloc(page_size * sizeof(char)); // forcing fragmentation
    blocks[i] = (char*) malloc(page_size * sizeof(char));
    free(dummy_ptr);
  }

  for (i = 0; i < block_cnt; i++) {
    char dummy_char = blocks[i][0];
  }

  for(i = 0; i < block_cnt; i++) {
    free(blocks[i]);
  }

  free(blocks);

  return 0;
}
