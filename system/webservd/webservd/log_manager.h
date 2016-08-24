// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef WEBSERVER_WEBSERVD_LOG_MANAGER_H_
#define WEBSERVER_WEBSERVD_LOG_MANAGER_H_

#include <memory>
#include <string>

#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/time.h>

struct sockaddr;

namespace webservd {

// A class that manages web server log files and helps with logging web request
// information.
class LogManager final {
 public:
  // Abstract interface for writing a log entry to a storage medium.
  // LogManager provides its own implementation for writing to a log file,
  // while tests can do something different.
  struct LoggerInterface {
    virtual ~LoggerInterface() = default;
    virtual void Log(const base::Time& timestamp, const std::string& entry) = 0;
  };

  LogManager() = default;

  // Initializes the logger and sets the log output directory.
  static void Init(const base::FilePath& log_directory);

  // Called when a request completes, so a new log entry can be added to log.
  static void OnRequestCompleted(const base::Time& timestamp,
                                 const sockaddr* client_addr,
                                 const std::string& method,
                                 const std::string& url,
                                 const std::string& version,
                                 int status_code,
                                 int64_t response_size);

  // Set a custom logger interface to do stuff other than log to a file.
  static void SetLogger(std::unique_ptr<LoggerInterface> logger);

 private:
  friend class FileLogger;

  // Returns the singleton instance of this class.
  static LogManager* GetInstance();

  // Keeps the last several days' worth of logs and purges the rest, to make
  // sure the log size is kept at bay.
  void PerformLogMaintenance();

  // Directory to write the logs to.
  base::FilePath log_directory_;

  // Logger interface (can be replaced for testing).
  std::unique_ptr<LoggerInterface> logger_;

  base::WeakPtrFactory<LogManager> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(LogManager);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_LOG_MANAGER_H_
