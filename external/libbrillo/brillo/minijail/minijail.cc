// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/minijail/minijail.h"

#include <sys/types.h>
#include <sys/wait.h>

using std::vector;

namespace brillo {

static base::LazyInstance<Minijail> g_minijail = LAZY_INSTANCE_INITIALIZER;

Minijail::Minijail() {}

Minijail::~Minijail() {}

// static
Minijail* Minijail::GetInstance() {
  return g_minijail.Pointer();
}

struct minijail* Minijail::New() {
  return minijail_new();
}

void Minijail::Destroy(struct minijail* jail) {
  minijail_destroy(jail);
}

void Minijail::DropRoot(struct minijail* jail, uid_t uid, gid_t gid) {
  minijail_change_uid(jail, uid);
  minijail_change_gid(jail, gid);
}

bool Minijail::DropRoot(struct minijail* jail,
                        const char* user,
                        const char* group) {
  // |user| and |group| are copied so the only reason either of these
  // calls can fail is ENOMEM.
  return !minijail_change_user(jail, user) &&
         !minijail_change_group(jail, group);
}

void Minijail::EnterNewPidNamespace(struct minijail* jail) {
  minijail_namespace_pids(jail);
}

void Minijail::MountTmp(struct minijail* jail) {
  minijail_mount_tmp(jail);
}

void Minijail::UseSeccompFilter(struct minijail* jail, const char* path) {
  minijail_no_new_privs(jail);
  minijail_use_seccomp_filter(jail);
  minijail_parse_seccomp_filters(jail, path);
}

void Minijail::UseCapabilities(struct minijail* jail, uint64_t capmask) {
  minijail_use_caps(jail, capmask);
}

void Minijail::ResetSignalMask(struct minijail* jail) {
  minijail_reset_signal_mask(jail);
}

void Minijail::Enter(struct minijail* jail) {
  minijail_enter(jail);
}

bool Minijail::Run(struct minijail* jail, vector<char*> args, pid_t* pid) {
  return minijail_run_pid(jail, args[0], args.data(), pid) == 0;
}

bool Minijail::RunSync(struct minijail* jail, vector<char*> args, int* status) {
  pid_t pid;
  if (Run(jail, args, &pid) && waitpid(pid, status, 0) == pid) {
    return true;
  }

  return false;
}

bool Minijail::RunPipe(struct minijail* jail,
                       vector<char*> args,
                       pid_t* pid,
                       int* stdin) {
#if defined(__ANDROID__)
  return minijail_run_pid_pipes_no_preload(jail, args[0], args.data(), pid,
                                           stdin, NULL, NULL) == 0;
#else
  return minijail_run_pid_pipes(jail, args[0], args.data(), pid, stdin, NULL,
                                NULL) == 0;
#endif  // __ANDROID__
}

bool Minijail::RunPipes(struct minijail* jail,
                        vector<char*> args,
                        pid_t* pid,
                        int* stdin,
                        int* stdout,
                        int* stderr) {
#if defined(__ANDROID__)
  return minijail_run_pid_pipes_no_preload(jail, args[0], args.data(), pid,
                                           stdin, stdout, stderr) == 0;
#else
  return minijail_run_pid_pipes(jail, args[0], args.data(), pid, stdin, stdout,
                                stderr) == 0;
#endif  // __ANDROID__
}

bool Minijail::RunAndDestroy(struct minijail* jail,
                             vector<char*> args,
                             pid_t* pid) {
  bool res = Run(jail, args, pid);
  Destroy(jail);
  return res;
}

bool Minijail::RunSyncAndDestroy(struct minijail* jail,
                                 vector<char*> args,
                                 int* status) {
  bool res = RunSync(jail, args, status);
  Destroy(jail);
  return res;
}

bool Minijail::RunPipeAndDestroy(struct minijail* jail,
                                 vector<char*> args,
                                 pid_t* pid,
                                 int* stdin) {
  bool res = RunPipe(jail, args, pid, stdin);
  Destroy(jail);
  return res;
}

bool Minijail::RunPipesAndDestroy(struct minijail* jail,
                                  vector<char*> args,
                                  pid_t* pid,
                                  int* stdin,
                                  int* stdout,
                                  int* stderr) {
  bool res = RunPipes(jail, args, pid, stdin, stdout, stderr);
  Destroy(jail);
  return res;
}

}  // namespace brillo
