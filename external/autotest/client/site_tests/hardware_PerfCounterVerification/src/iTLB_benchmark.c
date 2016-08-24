#include <stdlib.h>
#include "iTLB_benchmark_function.h"

int main(int argc, char *argv[]) {
  unsigned long loops = 1000;
  if (argc > 1) {
    loops = strtoul(argv[1], NULL, 10);
    if (loops < 1) {
      loops = 1;
    }
  }

  while (--loops) {
    iTLB_bechmark_function();
  }

  return 0;
}
