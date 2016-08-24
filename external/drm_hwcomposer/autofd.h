/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ANDROID_AUTO_FD_H_
#define ANDROID_AUTO_FD_H_

#include <unistd.h>

namespace android {

class UniqueFd {
 public:
  UniqueFd() = default;
  UniqueFd(int fd) : fd_(fd) {
  }
  UniqueFd(UniqueFd &&rhs) {
    fd_ = rhs.fd_;
    rhs.fd_ = -1;
  }

  UniqueFd &operator=(UniqueFd &&rhs) {
    Set(rhs.Release());
    return *this;
  }

  ~UniqueFd() {
    if (fd_ >= 0)
      close(fd_);
  }

  int Release() {
    int old_fd = fd_;
    fd_ = -1;
    return old_fd;
  }

  int Set(int fd) {
    if (fd_ >= 0)
      close(fd_);
    fd_ = fd;
    return fd_;
  }

  void Close() {
    if (fd_ >= 0)
      close(fd_);
    fd_ = -1;
  }

  int get() const {
    return fd_;
  }

 private:
  int fd_ = -1;
};

struct OutputFd {
  OutputFd() = default;
  OutputFd(int *fd) : fd_(fd) {
  }
  OutputFd(OutputFd &&rhs) {
    fd_ = rhs.fd_;
    rhs.fd_ = NULL;
  }

  OutputFd &operator=(OutputFd &&rhs) {
    fd_ = rhs.fd_;
    rhs.fd_ = NULL;
    return *this;
  }

  int Set(int fd) {
    if (*fd_ >= 0)
      close(*fd_);
    *fd_ = fd;
    return fd;
  }

  int get() {
    return *fd_;
  }

  operator bool() const {
    return fd_ != NULL;
  }

 private:
  int *fd_ = NULL;
};
}

#endif
