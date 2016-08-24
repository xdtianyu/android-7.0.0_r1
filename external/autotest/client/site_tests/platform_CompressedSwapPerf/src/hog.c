// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define CMD_POKE 1
#define CMD_BALLOON 2
#define CMD_EXIT 3

#define TOUCH_LIMIT 1000
#define WRITE_MOD 10

// Allocate memory in 1 MiB chunks
#define CHUNK_SIZE (1 << 20)

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

// Hog's main buffer
char *global_buf = NULL;
size_t buf_size = 0;

// Stores a chunk of fake data that will give a target compression ratio
char *fake_data;

// Dummy global that forces compiler to perform the read
volatile char dummy;

struct PokeResult {
  uint64_t real_time;
  uint64_t user_time;
  uint64_t sys_time;
  uint64_t faults;
} __attribute__((packed));

// Reads and writes random pages in global_buf.
static void TouchMemory() {
  for (int i = 0; i < TOUCH_LIMIT; i++) {
    unsigned int index = (unsigned int)rand();

    // Randomly do a write instead of a read.
    if (rand() % WRITE_MOD == 0) {
      global_buf[index % buf_size] = 0x00;
    } else {
      dummy = global_buf[index % buf_size];
    }
  }
}

// Allocates memory and copies fake data in to ensure there's no copy-on-write
// business going on.
static void BalloonMemory(size_t balloon_size) {
  size_t new_buf_size = buf_size + balloon_size * CHUNK_SIZE;
  global_buf = realloc(global_buf, new_buf_size);

  // Copy fake data into every chunk that we allocate.
  for (unsigned int chunk = 0; chunk < balloon_size; chunk++) {
    char *new_chunk = global_buf + buf_size + chunk * CHUNK_SIZE;
    memcpy(new_chunk, fake_data, CHUNK_SIZE);
  }

  buf_size = new_buf_size;
}

// Calculates the difference between two timespecs in milliseconds.
static uint64_t DiffTimespec(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000 +
         (end.tv_nsec - start.tv_nsec) / 1000000;
}

// Calculates the difference between two timevals in milliseconds.
static uint64_t DiffTimeval(struct timeval start, struct timeval end) {
  return (end.tv_sec - start.tv_sec) * 1000 +
         (end.tv_usec - start.tv_usec) / 1000;
}

int main(int argc, char *argv[]) {
  int sockfd;
  struct sockaddr_un test_sock_addr;
  int compression_factor = 3;
  int random_fd = open("/dev/urandom", O_RDONLY);

  if (argc < 2) {
    fprintf(stderr, "Usage: %s SOCKETNAME COMPRESSION_FACTOR\n", argv[0]);
    return 1;
  }

  if (argc == 3) {
    compression_factor = atoi(argv[2]);
  }

  srand(getpid());

  test_sock_addr.sun_family = AF_UNIX;
  strncpy(test_sock_addr.sun_path, argv[1], strlen(argv[1]) + 1);

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("could not open socket");
    return 1;
  }

  // Unlink any existing socket with this name.
  struct stat file_stat;
  if (stat(argv[1], &file_stat) == 0) {
    if (S_ISSOCK(file_stat.st_mode)) {
      unlink(argv[1]);
    } else {
      fprintf(stderr,
             "there is a file with the given socket name already; aborting\n");
      return 1;
    }
  }

  if (bind(sockfd, (struct sockaddr *)&test_sock_addr, sizeof test_sock_addr)) {
    perror("could not bind to socket");
    return 1;
  }

  if (listen(sockfd, 1)) {
    perror("could not listen to socket");
    return 1;
  }

  int connfd;
  if ((connfd = accept(sockfd, NULL, NULL)) < 0) {
    perror("could not accept connection");
    return 1;
  }

  // Fill fake_data with fake data so that it compresses to roughly the desired
  // compression factor. Random data should be uncompressible, while long
  // sequences of ones are highly compressible.
  fake_data = malloc(CHUNK_SIZE);
  read(random_fd, fake_data, CHUNK_SIZE / compression_factor);

  memset(fake_data + CHUNK_SIZE / compression_factor, 1,
         CHUNK_SIZE - (CHUNK_SIZE / compression_factor));

  // Allocate one chunk worth of data to start with.
  BalloonMemory(1);

  while (true) {
    uint32_t command;
    uint32_t balloon_size;
    struct sockaddr src_addr;
    struct timespec time_start;
    struct timespec time_end;
    struct rusage usage_start;
    struct rusage usage_end;
    struct PokeResult result;

    ssize_t bytes_read = recv(connfd, &command, sizeof(command), 0);

    if (bytes_read < 0) {
      perror("error while reading from socket");
      return 1;
    } else if (bytes_read == 0) {
      // Remote socket closed early; clean up this hog.
      fprintf(stderr, "read 0 bytes from socket; terminating\n");
      return 0;
    } else if (bytes_read != sizeof(command)) {
      fprintf(stderr, "read %li bytes (expected %lu); aborting\n",
              bytes_read, sizeof(command));
      return 1;
    }

    switch(command) {
      case CMD_POKE:
        // Touch pages of memory while monitoring time and resource usage.
        getrusage(RUSAGE_SELF, &usage_start);
        clock_gettime(CLOCK_REALTIME, &time_start);

        TouchMemory();

        clock_gettime(CLOCK_REALTIME, &time_end);
        getrusage(RUSAGE_SELF, &usage_end);

        // Send stats back to monitor script.
        result.real_time = DiffTimespec(time_start, time_end);
        result.user_time = DiffTimeval(usage_start.ru_utime,
                                       usage_end.ru_utime);
        result.sys_time = DiffTimeval(usage_start.ru_stime,
                                      usage_end.ru_stime);
        result.faults = usage_end.ru_majflt - usage_start.ru_majflt;

        send(connfd, &result, sizeof(result), 0);
        break;
      case CMD_BALLOON:
        bytes_read = recv(connfd, &balloon_size, sizeof(balloon_size), 0);

        if (bytes_read < 0) {
          perror("error while reading from socket");
          return 1;
        } else if (bytes_read == 0) {
          fprintf(stderr, "read 0 bytes from socket; terminating\n");
          return 0;
        }

        BalloonMemory(balloon_size);
        send(connfd, &balloon_size, sizeof(balloon_size), 0);
        break;
      case CMD_EXIT:
        fprintf(stderr, "exiting\n");
        return 0;
      default:
        fprintf(stderr, "unexpected command: %d\n", command);
    }
  }

  return 0;
}
