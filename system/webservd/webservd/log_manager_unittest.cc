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
#include <netinet/in.h>
#include <set>
#include <vector>

#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#include <gtest/gtest.h>

namespace webservd {

namespace {

struct TestLogger : public LogManager::LoggerInterface {
  explicit TestLogger(std::string& last_entry) : last_entry_{last_entry} {}
  void Log(const base::Time& timestamp, const std::string& entry) override {
    last_entry_ = entry;
  }
  std::string& last_entry_;
};

}  // Anonymous namespace

class LogManagerTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  }

  // Adds a test log entry to the file corresponding to the give |timestamp|.
  void LogEntry(const base::Time& timestamp) {
    sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = 80;
    inet_aton("10.11.12.13", &client_addr.sin_addr);
    LogManager::OnRequestCompleted(
        timestamp, reinterpret_cast<const sockaddr*>(&client_addr), "POST",
        "/test", "HTTP/1.0", 200, 123456);
  }

  // Get the list of current log files in the test log directory.
  std::set<std::string> GetLogFiles() const {
    std::set<std::string> log_files;
    base::FileEnumerator enumerator{temp_dir.path(),
                                    false,
                                    base::FileEnumerator::FILES,
                                    "*.log"};
    base::FilePath file = enumerator.Next();
    while (!file.empty()) {
      log_files.insert(file.BaseName().value());
      file = enumerator.Next();
    }
    return log_files;
  }

  base::ScopedTempDir temp_dir;
};

TEST_F(LogManagerTest, OnRequestCompleted) {
  std::string last_entry;
  LogManager::SetLogger(
      std::unique_ptr<LogManager::LoggerInterface>(new TestLogger{last_entry}));
  base::Time timestamp = base::Time::Now();
  LogEntry(timestamp);

  tm time_buf = {};
  char str_buf[32] = {};
  time_t time = timestamp.ToTimeT();
  strftime(str_buf, sizeof(str_buf), "%d/%b/%Y:%H:%M:%S %z",
           localtime_r(&time, &time_buf));
  std::string match = base::StringPrintf(
      "10.11.12.13 - - [%s] \"POST /test HTTP/1.0\" 200 123456\n",
      str_buf);
  EXPECT_EQ(match, last_entry);
}

TEST_F(LogManagerTest, LogFileManagement) {
  LogManager::Init(temp_dir.path());
  EXPECT_TRUE(GetLogFiles().empty());
  // Feb 25, 2015, 0:00:00 Local time
  tm date = {0, 0, 0, 25, 1, 115, 2, 55, 0};
  base::Time timestamp = base::Time::FromTimeT(mktime(&date));
  for (size_t i = 0; i < 10; i++) {
    LogEntry(timestamp);
    LogEntry(timestamp);
    LogEntry(timestamp);
    LogEntry(timestamp);
    timestamp += base::TimeDelta::FromDays(1);
  }
  std::set<std::string> expected_files{
    "2015-02-28.log",
    "2015-03-01.log",
    "2015-03-02.log",
    "2015-03-03.log",
    "2015-03-04.log",
    "2015-03-05.log",
    "2015-03-06.log",
  };
  EXPECT_EQ(expected_files, GetLogFiles());
}

TEST_F(LogManagerTest, LargeLogs) {
  const size_t log_line_len = 78;  // Test log entries are 78 chars long.
  LogManager::Init(temp_dir.path());
  EXPECT_TRUE(GetLogFiles().empty());
  // Feb 25, 2015, 0:00:00 Local time
  tm date = {0, 0, 0, 25, 1, 115, 2, 55, 0};
  base::Time timestamp = base::Time::FromTimeT(mktime(&date));
  // Create 2015-02-25.log
  LogEntry(timestamp);

  timestamp += base::TimeDelta::FromDays(1);
  // Write a large 2015-02-26.log but with enough room for one more log line.
  std::vector<char> data(1024 * 1024 - (log_line_len * 3 / 2), ' ');
  base::FilePath current_file = temp_dir.path().Append("2015-02-26.log");
  ASSERT_EQ(static_cast<int>(data.size()),
            base::WriteFile(current_file, data.data(), data.size()));
  // Add the line. Should still go to the same file.
  LogEntry(timestamp);
  std::set<std::string> expected_files{
    "2015-02-25.log",
    "2015-02-26.log",
  };
  EXPECT_EQ(expected_files, GetLogFiles());
  // Now this log entry will not fit and will end up creating a new file.
  LogEntry(timestamp);
  expected_files = {
    "2015-02-25.log",
    "2015-02-26-a.log",
    "2015-02-26.log",
  };
  EXPECT_EQ(expected_files, GetLogFiles());
  // Add some more data to the current file.
  ASSERT_TRUE(base::AppendToFile(current_file, data.data(), data.size()));
  LogEntry(timestamp);
  expected_files = {
    "2015-02-25.log",
    "2015-02-26-a.log",
    "2015-02-26-b.log",
    "2015-02-26.log",
  };
}
}  // namespace webservd
