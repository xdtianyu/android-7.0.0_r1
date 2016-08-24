/* Compile with:
 * i686-pc-linux-gnu-gcc hog.c -o hog
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MEGA (1 << 20)
#define CHUNK_SIZE MEGA                 /* one-megabyte chunks */
#define MAX_CHUNKS 4096
const int n_touches = CHUNK_SIZE >> 12;  /* average 1 per page */
char *chunks[MAX_CHUNKS];


long int estrtol(const char *s) {
  char *end;
  long int n;
  n = strtol(s, &end, 10);
  if (*end != '\0') {
    fprintf(stderr, "hog: malformed integer %s\n", s);
    exit(1);
  }
  return n;
}


int main(int ac, const char **av) {
  int chunk;
  int compression_factor = 3;
  unsigned int i, c, x;
  long int megabytes;
  int random_fd = open("/dev/urandom", O_RDONLY);
  char *fake_data = malloc(CHUNK_SIZE);
  char *p;

  if (ac != 2 && ac != 3) {
    fprintf(stderr,
            "usage: hog <megabytes> [<compression factor (default = 3)>]\n");
    exit(1);
  }

  megabytes = estrtol(av[1]);

  if (megabytes > MAX_CHUNKS) {
    fprintf(stderr, "hog: too many megabytes (%ld, max = %d)\n",
            megabytes, MAX_CHUNKS);
  }

  if (ac == 3) {
     compression_factor = estrtol(av[2]);
  }

  /* Fill fake_data with fake data so that it compresses to roughly the desired
   * compression factor.
   */
  read(random_fd, fake_data, CHUNK_SIZE / compression_factor);
  /* Fill the rest of the fake data with ones (compresses well). */
  memset(fake_data + CHUNK_SIZE / compression_factor, 1,
         CHUNK_SIZE - (CHUNK_SIZE / compression_factor));

  for (chunk = 0; chunk < megabytes; chunk++) {
    /* Allocate */
    p = malloc(CHUNK_SIZE);

    if (p == NULL) {
      printf("hog: out of memory at chunk %d\n", chunk);
      break;
    }

    /* Fill allocated memory with fake data */
    memcpy(p, fake_data, CHUNK_SIZE);

    /* Remember allocated data. */
    chunks[chunk] = p;
  }

  printf("hog: idling\n", chunk);
  while (1)
    sleep(10);
}
