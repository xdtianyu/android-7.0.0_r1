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

#include "webservd/log_manager.h"

#include <arpa/inet.h>
#include <cinttypes>
#include <netinet/in.h>
#include <set>

#include <base/bind.h>
#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/lazy_instance.h>
#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>

namespace webservd {

namespace {

// Singleton instance of LogManager class.
base::LazyInstance<LogManager> g_log_manager = LAZY_INSTANCE_INITIALIZER;

// The number of files to keep in the log directory. Since there is one log
// file per day of logging, this is essentially how many days' worth of logs
// to keep. This also controls the total maximum size of the log data, which
// is (kLogFilesToKeep * kMaxLogFileSize).
const size_t kLogFilesToKeep = 7;

// Maximum log file size.
const int64_t kMaxLogFileSize = 1024 * 1024;  // 1 MB

// Obtain an IP address as a human-readable string for logging.
std::string GetIPAddress(const sockaddr* addr) {
  static_assert(INET6_ADDRSTRLEN > INET_ADDRSTRLEN, "Unexpected IP addr len.");
  char buf[INET6_ADDRSTRLEN] = "-";
  if (!addr)
    return buf;

  switch (addr->sa_family) {
    case AF_INET: {
      auto addr_in = reinterpret_cast<const sockaddr_in*>(addr);
      if (!inet_ntop(AF_INET, &addr_in->sin_addr, buf, sizeof(buf)))
        PLOG(ERROR) << "Unable to get IP address string (IPv4)";
    }
    break;

    case AF_INET6: {
      auto addr_in6 = reinterpret_cast<const sockaddr_in6*>(addr);

      // Note that inet_ntop(3) doesn't handle IPv4-mapped IPv6
      // addresses [1] the way you'd expect .. for example, it returns
      // "::ffff:172.22.72.163" instead of the more traditional IPv4
      // notation "172.22.72.163". Fortunately, this is pretty easy to
      // fix ourselves.
      //
      // [1] : see RFC 4291, section 2.5.5.2 for what that means
      //       http://tools.ietf.org/html/rfc4291#section-2.5.5
      //
      auto dwords = reinterpret_cast<const uint32_t*>(&addr_in6->sin6_addr);
      if (dwords[0] == 0x00000000 && dwords[1] == 0x00000000 &&
          dwords[2] == htonl(0x0000ffff)) {
        auto bytes = reinterpret_cast<const uint8_t*>(&addr_in6->sin6_addr);
        return base::StringPrintf("%d.%d.%d.%d",
                                  bytes[12], bytes[13], bytes[14], bytes[15]);
      } else if (!inet_ntop(AF_INET6, &addr_in6->sin6_addr, buf, sizeof(buf))) {
        PLOG(ERROR) << "Unable to get IP address string (IPv6)";
      }
    }
    break;

    default:
      LOG(ERROR) << "Unsupported address family " << addr->sa_family;
      break;
  }
  return buf;
}

}  // Anonymous namespace

// Logger class to write the log data to a log file.
class FileLogger final : public LogManager::LoggerInterface {
 public:
  FileLogger(const base::FilePath& log_directory, LogManager* log_manager)
      : log_directory_(log_directory), log_manager_{log_manager} {}

  // Write the log entry to today's log file.
  void Log(const base::Time& timestamp, const std::string& entry) override {
    tm time_buf = {};
    char file_name[32] = {};
    // Create the file name in year-month-day format so that string sort would
    // correspond to date sort.
    time_t time = timestamp.ToTimeT();
    strftime(file_name, sizeof(file_name), "%Y-%m-%d.log",
             localtime_r(&time, &time_buf));
    base::FilePath file_path = log_directory_.Append(file_name);
    bool success = false;
    bool exists = base::PathExists(file_path);
    // If the file already exists, check its size. If it is going to be larger
    // then the maximum allowed log size, archive the current log file and
    // create a new, empty one.
    if (exists) {
      int64_t file_size = 0;
      bool got_size = base::GetFileSize(file_path, &file_size);
      if (got_size && (file_size + entry.size() > kMaxLogFileSize))
        exists = !ArchiveLogFile(file_name);
    }

    if (exists) {
      success = base::AppendToFile(file_path, entry.data(), entry.size());
    } else {
      int size = static_cast<int>(entry.size());
      success = (base::WriteFile(file_path, entry.data(), size) == size);
      if (success) {
        // We just created a new file, see if we need to purge old ones.
        log_manager_->PerformLogMaintenance();
      }
    }
    PLOG_IF(ERROR, !success) << "Failed to append a log entry to log file at "
                             << file_path.value();
  }

 private:
  // Renames the log file to a next available suffix-appended archive when
  // the log file size starts to exceed the pre-defined maximum size.
  // The existing log file is renamed by changing the original |file_name| to
  // YYYY-MM-DD-<suffix>.log where suffix is characters 'a', 'b', ...
  // Since '-' comes before '.', "2015-02-25-a.log" will come before
  // "2015-02-25.log" in sort order and the previously-renamed files will be
  // considered "older" than the current one, which is what we need.
  // Returns true if the file has been successfully renamed.
  bool ArchiveLogFile(const std::string& file_name) {
    char suffix = 'a';
    auto pair = brillo::string_utils::SplitAtFirst(file_name, ".");
    // If we try all the suffixes from 'a' to 'z' and still can't find a name,
    // abandon this strategy and keep appending to the current file.
    while (suffix <= 'z') {
      base::FilePath archive_file_path = log_directory_.Append(
          base::StringPrintf("%s-%c.%s", pair.first.c_str(),
                              suffix, pair.second.c_str()));
      if (!base::PathExists(archive_file_path)) {
        base::FilePath file_path = log_directory_.Append(file_name);
        if (base::Move(file_path, archive_file_path)) {
          // Successfully renamed, start a new log file.
          return true;
        } else {
          PLOG(ERROR) << "Failed to rename log file from "
                      << file_path.value() << " to "
                      << archive_file_path.value();
        }
        break;
      }
      suffix++;
    }
    return false;
  }

  base::FilePath log_directory_;
  LogManager* log_manager_{nullptr};
};

void LogManager::Init(const base::FilePath& log_directory) {
  LogManager* inst = GetInstance();
  inst->log_directory_ = log_directory;
  inst->SetLogger(
      std::unique_ptr<LoggerInterface>{new FileLogger{log_directory, inst}});
  inst->PerformLogMaintenance();
}

void LogManager::OnRequestCompleted(const base::Time& timestamp,
                                    const sockaddr* client_addr,
                                    const std::string& method,
                                    const std::string& url,
                                    const std::string& version,
                                    int status_code,
                                    int64_t response_size) {
  std::string ip_address = GetIPAddress(client_addr);
  tm time_buf = {};
  char str_buf[32] = {};
  // Format the date/time as "25/Feb/2015:03:29:12 -0800".
  time_t time = timestamp.ToTimeT();
  strftime(str_buf, sizeof(str_buf), "%d/%b/%Y:%H:%M:%S %z",
           localtime_r(&time, &time_buf));

  // Log file entry for one HTTP request looking like this:
  // 127.0.0.1 - - [25/Feb/2015:03:29:12 -0800] "GET /test HTTP/1.1" 200 2326
  std::string size_string{"-"};
  if (response_size >= 0)
    size_string = std::to_string(response_size);
  std::string log_entry = base::StringPrintf(
      "%s - - [%s] \"%s %s %s\" %d %s\n", ip_address.c_str(), str_buf,
      method.c_str(), url.c_str(), version.c_str(), status_code,
      size_string.c_str());
  GetInstance()->logger_->Log(timestamp, log_entry);
}

void LogManager::SetLogger(std::unique_ptr<LoggerInterface> logger) {
  GetInstance()->logger_ = std::move(logger);
}

LogManager* LogManager::GetInstance() {
  return g_log_manager.Pointer();
}

void LogManager::PerformLogMaintenance() {
  // Get the list of all the log files in the log directory and put them into
  // a set which will sort the files by name (and effectively by the date since
  // we chose the file naming scheme deliberately to guarantee proper sorting
  // order).
  std::set<base::FilePath> log_files;
  base::FileEnumerator enumerator{log_directory_,
                                  false,
                                  base::FileEnumerator::FILES,
                                  "*.log"};
  base::FilePath file = enumerator.Next();
  while (!file.empty()) {
    log_files.insert(file);
    file = enumerator.Next();
  }

  // Now, if we have more files than we want to keep, purge the old files.
  while (log_files.size() > kLogFilesToKeep) {
    auto front_it = log_files.begin();
    PLOG_IF(WARNING, !base::DeleteFile(*front_it, false))
        << "Failed to delete an old log file: " << front_it->value();
    log_files.erase(front_it);
  }
}

}  // namespace webservd
