#include <stdlib.h>

int main(int argc, char *argv[]) {
  unsigned long loops = 10000000; // 10 million
  if (argc > 1) {
    loops = strtoul(argv[1], NULL, 10);
    if (loops < 1) {
      loops = 1;
    }
  }

  asm (".local the_loop_start\nthe_loop_start:\n\t");
  while (--loops) {
    /* nop */
    asm (".local the_loop_body\nthe_loop_body:\n\t");
  }
  asm (".local the_loop_end\nthe_loop_end:\n\t");

  return 0;
}
