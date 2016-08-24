//
// Copyright (C) 2011 The Android Open Source Project
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
//

#include "update_engine/payload_consumer/download_action.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/bind.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/location.h>
#include <base/strings/stringprintf.h>
#include <brillo/bind_lambda.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/common/action_pipe.h"
#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/mock_http_fetcher.h"
#include "update_engine/common/mock_prefs.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/fake_p2p_manager_configuration.h"
#include "update_engine/fake_system_state.h"
#include "update_engine/payload_consumer/mock_download_action.h"
#include "update_engine/update_manager/fake_update_manager.h"

namespace chromeos_update_engine {

using base::FilePath;
using base::ReadFileToString;
using base::WriteFile;
using std::string;
using std::unique_ptr;
using std::vector;
using test_utils::ScopedTempFile;
using testing::AtLeast;
using testing::InSequence;
using testing::Return;
using testing::_;

class DownloadActionTest : public ::testing::Test { };

namespace {

class DownloadActionTestProcessorDelegate : public ActionProcessorDelegate {
 public:
  explicit DownloadActionTestProcessorDelegate(ErrorCode expected_code)
      : processing_done_called_(false),
        expected_code_(expected_code) {}
  ~DownloadActionTestProcessorDelegate() override {
    EXPECT_TRUE(processing_done_called_);
  }
  void ProcessingDone(const ActionProcessor* processor,
                      ErrorCode code) override {
    brillo::MessageLoop::current()->BreakLoop();
    brillo::Blob found_data;
    ASSERT_TRUE(utils::ReadFile(path_, &found_data));
    if (expected_code_ != ErrorCode::kDownloadWriteError) {
      ASSERT_EQ(expected_data_.size(), found_data.size());
      for (unsigned i = 0; i < expected_data_.size(); i++) {
        EXPECT_EQ(expected_data_[i], found_data[i]);
      }
    }
    processing_done_called_ = true;
  }

  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) override {
    const string type = action->Type();
    if (type == DownloadAction::StaticType()) {
      EXPECT_EQ(expected_code_, code);
    } else {
      EXPECT_EQ(ErrorCode::kSuccess, code);
    }
  }

  string path_;
  brillo::Blob expected_data_;
  bool processing_done_called_;
  ErrorCode expected_code_;
};

class TestDirectFileWriter : public DirectFileWriter {
 public:
  TestDirectFileWriter() : fail_write_(0), current_write_(0) {}
  void set_fail_write(int fail_write) { fail_write_ = fail_write; }

  virtual bool Write(const void* bytes, size_t count) {
    if (++current_write_ == fail_write_) {
      return false;
    }
    return DirectFileWriter::Write(bytes, count);
  }

 private:
  // If positive, fail on the |fail_write_| call to Write.
  int fail_write_;
  int current_write_;
};

void StartProcessorInRunLoop(ActionProcessor* processor,
                             MockHttpFetcher* http_fetcher) {
  processor->StartProcessing();
  http_fetcher->SetOffset(1);
}

void TestWithData(const brillo::Blob& data,
                  int fail_write,
                  bool use_download_delegate) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();
  FakeSystemState fake_system_state;

  // TODO(adlr): see if we need a different file for build bots
  ScopedTempFile output_temp_file;
  TestDirectFileWriter writer;
  EXPECT_EQ(
      0, writer.Open(output_temp_file.path().c_str(), O_WRONLY | O_CREAT, 0));
  writer.set_fail_write(fail_write);

  // We pull off the first byte from data and seek past it.
  string hash = HashCalculator::HashOfBytes(&data[1], data.size() - 1);
  uint64_t size = data.size();
  InstallPlan install_plan;
  install_plan.payload_type = InstallPayloadType::kDelta;
  install_plan.payload_size = size;
  install_plan.payload_hash = hash;
  install_plan.source_slot = 0;
  install_plan.target_slot = 1;
  // We mark both slots as bootable. Only the target slot should be unbootable
  // after the download starts.
  fake_system_state.fake_boot_control()->SetSlotBootable(
      install_plan.source_slot, true);
  fake_system_state.fake_boot_control()->SetSlotBootable(
      install_plan.target_slot, true);
  ObjectFeederAction<InstallPlan> feeder_action;
  feeder_action.set_obj(install_plan);
  MockPrefs prefs;
  MockHttpFetcher* http_fetcher = new MockHttpFetcher(data.data(),
                                                      data.size(),
                                                      nullptr);
  // takes ownership of passed in HttpFetcher
  DownloadAction download_action(&prefs,
                                 fake_system_state.boot_control(),
                                 fake_system_state.hardware(),
                                 &fake_system_state,
                                 http_fetcher);
  download_action.SetTestFileWriter(&writer);
  BondActions(&feeder_action, &download_action);
  MockDownloadActionDelegate download_delegate;
  if (use_download_delegate) {
    InSequence s;
    download_action.set_delegate(&download_delegate);
    if (data.size() > kMockHttpFetcherChunkSize)
      EXPECT_CALL(download_delegate,
                  BytesReceived(_, 1 + kMockHttpFetcherChunkSize, _));
    EXPECT_CALL(download_delegate, BytesReceived(_, _, _)).Times(AtLeast(1));
  }
  ErrorCode expected_code = ErrorCode::kSuccess;
  if (fail_write > 0)
    expected_code = ErrorCode::kDownloadWriteError;
  DownloadActionTestProcessorDelegate delegate(expected_code);
  delegate.expected_data_ = brillo::Blob(data.begin() + 1, data.end());
  delegate.path_ = output_temp_file.path();
  ActionProcessor processor;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&download_action);

  loop.PostTask(FROM_HERE,
                base::Bind(&StartProcessorInRunLoop, &processor, http_fetcher));
  loop.Run();
  EXPECT_FALSE(loop.PendingTasks());

  EXPECT_TRUE(fake_system_state.fake_boot_control()->IsSlotBootable(
      install_plan.source_slot));
  EXPECT_FALSE(fake_system_state.fake_boot_control()->IsSlotBootable(
      install_plan.target_slot));
}
}  // namespace

TEST(DownloadActionTest, SimpleTest) {
  brillo::Blob small;
  const char* foo = "foo";
  small.insert(small.end(), foo, foo + strlen(foo));
  TestWithData(small,
               0,  // fail_write
               true);  // use_download_delegate
}

TEST(DownloadActionTest, LargeTest) {
  brillo::Blob big(5 * kMockHttpFetcherChunkSize);
  char c = '0';
  for (unsigned int i = 0; i < big.size(); i++) {
    big[i] = c;
    c = ('9' == c) ? '0' : c + 1;
  }
  TestWithData(big,
               0,  // fail_write
               true);  // use_download_delegate
}

TEST(DownloadActionTest, FailWriteTest) {
  brillo::Blob big(5 * kMockHttpFetcherChunkSize);
  char c = '0';
  for (unsigned int i = 0; i < big.size(); i++) {
    big[i] = c;
    c = ('9' == c) ? '0' : c + 1;
  }
  TestWithData(big,
               2,  // fail_write
               true);  // use_download_delegate
}

TEST(DownloadActionTest, NoDownloadDelegateTest) {
  brillo::Blob small;
  const char* foo = "foofoo";
  small.insert(small.end(), foo, foo + strlen(foo));
  TestWithData(small,
               0,  // fail_write
               false);  // use_download_delegate
}

namespace {
class TerminateEarlyTestProcessorDelegate : public ActionProcessorDelegate {
 public:
  void ProcessingStopped(const ActionProcessor* processor) {
    brillo::MessageLoop::current()->BreakLoop();
  }
};

void TerminateEarlyTestStarter(ActionProcessor* processor) {
  processor->StartProcessing();
  CHECK(processor->IsRunning());
  processor->StopProcessing();
}

void TestTerminateEarly(bool use_download_delegate) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();

  brillo::Blob data(kMockHttpFetcherChunkSize +
                      kMockHttpFetcherChunkSize / 2);
  memset(data.data(), 0, data.size());

  ScopedTempFile temp_file;
  {
    DirectFileWriter writer;
    EXPECT_EQ(0, writer.Open(temp_file.path().c_str(), O_WRONLY | O_CREAT, 0));

    // takes ownership of passed in HttpFetcher
    ObjectFeederAction<InstallPlan> feeder_action;
    InstallPlan install_plan;
    feeder_action.set_obj(install_plan);
    FakeSystemState fake_system_state_;
    MockPrefs prefs;
    DownloadAction download_action(
        &prefs,
        fake_system_state_.boot_control(),
        fake_system_state_.hardware(),
        &fake_system_state_,
        new MockHttpFetcher(data.data(), data.size(), nullptr));
    download_action.SetTestFileWriter(&writer);
    MockDownloadActionDelegate download_delegate;
    if (use_download_delegate) {
      download_action.set_delegate(&download_delegate);
      EXPECT_CALL(download_delegate, BytesReceived(_, _, _)).Times(0);
    }
    TerminateEarlyTestProcessorDelegate delegate;
    ActionProcessor processor;
    processor.set_delegate(&delegate);
    processor.EnqueueAction(&feeder_action);
    processor.EnqueueAction(&download_action);
    BondActions(&feeder_action, &download_action);

    loop.PostTask(FROM_HERE,
                  base::Bind(&TerminateEarlyTestStarter, &processor));
    loop.Run();
    EXPECT_FALSE(loop.PendingTasks());
  }

  // 1 or 0 chunks should have come through
  const off_t resulting_file_size(utils::FileSize(temp_file.path()));
  EXPECT_GE(resulting_file_size, 0);
  if (resulting_file_size != 0)
    EXPECT_EQ(kMockHttpFetcherChunkSize,
              static_cast<size_t>(resulting_file_size));
}

}  // namespace

TEST(DownloadActionTest, TerminateEarlyTest) {
  TestTerminateEarly(true);
}

TEST(DownloadActionTest, TerminateEarlyNoDownloadDelegateTest) {
  TestTerminateEarly(false);
}

class DownloadActionTestAction;

template<>
class ActionTraits<DownloadActionTestAction> {
 public:
  typedef InstallPlan OutputObjectType;
  typedef InstallPlan InputObjectType;
};

// This is a simple Action class for testing.
class DownloadActionTestAction : public Action<DownloadActionTestAction> {
 public:
  DownloadActionTestAction() : did_run_(false) {}
  typedef InstallPlan InputObjectType;
  typedef InstallPlan OutputObjectType;
  ActionPipe<InstallPlan>* in_pipe() { return in_pipe_.get(); }
  ActionPipe<InstallPlan>* out_pipe() { return out_pipe_.get(); }
  ActionProcessor* processor() { return processor_; }
  void PerformAction() {
    did_run_ = true;
    ASSERT_TRUE(HasInputObject());
    EXPECT_TRUE(expected_input_object_ == GetInputObject());
    ASSERT_TRUE(processor());
    processor()->ActionComplete(this, ErrorCode::kSuccess);
  }
  string Type() const { return "DownloadActionTestAction"; }
  InstallPlan expected_input_object_;
  bool did_run_;
};

namespace {
// This class is an ActionProcessorDelegate that simply terminates the
// run loop when the ActionProcessor has completed processing. It's used
// only by the test PassObjectOutTest.
class PassObjectOutTestProcessorDelegate : public ActionProcessorDelegate {
 public:
  void ProcessingDone(const ActionProcessor* processor, ErrorCode code) {
    brillo::MessageLoop::current()->BreakLoop();
  }
};

}  // namespace

TEST(DownloadActionTest, PassObjectOutTest) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();

  DirectFileWriter writer;
  EXPECT_EQ(0, writer.Open("/dev/null", O_WRONLY | O_CREAT, 0));

  // takes ownership of passed in HttpFetcher
  InstallPlan install_plan;
  install_plan.payload_size = 1;
  install_plan.payload_hash = HashCalculator::HashOfString("x");
  ObjectFeederAction<InstallPlan> feeder_action;
  feeder_action.set_obj(install_plan);
  MockPrefs prefs;
  FakeSystemState fake_system_state_;
  DownloadAction download_action(&prefs,
                                 fake_system_state_.boot_control(),
                                 fake_system_state_.hardware(),
                                 &fake_system_state_,
                                 new MockHttpFetcher("x", 1, nullptr));
  download_action.SetTestFileWriter(&writer);

  DownloadActionTestAction test_action;
  test_action.expected_input_object_ = install_plan;
  BondActions(&feeder_action, &download_action);
  BondActions(&download_action, &test_action);

  ActionProcessor processor;
  PassObjectOutTestProcessorDelegate delegate;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&download_action);
  processor.EnqueueAction(&test_action);

  loop.PostTask(FROM_HERE,
                base::Bind([&processor] { processor.StartProcessing(); }));
  loop.Run();
  EXPECT_FALSE(loop.PendingTasks());

  EXPECT_EQ(true, test_action.did_run_);
}

// Test fixture for P2P tests.
class P2PDownloadActionTest : public testing::Test {
 protected:
  P2PDownloadActionTest()
    : start_at_offset_(0),
      fake_um_(fake_system_state_.fake_clock()) {}

  ~P2PDownloadActionTest() override {}

  // Derived from testing::Test.
  void SetUp() override {
    loop_.SetAsCurrent();
  }

  // Derived from testing::Test.
  void TearDown() override {
    EXPECT_FALSE(loop_.PendingTasks());
  }

  // To be called by tests to setup the download. The
  // |starting_offset| parameter is for where to resume.
  void SetupDownload(off_t starting_offset) {
    start_at_offset_ = starting_offset;
    // Prepare data 10 kB of data.
    data_.clear();
    for (unsigned int i = 0; i < 10 * 1000; i++)
      data_ += 'a' + (i % 25);

    // Setup p2p.
    FakeP2PManagerConfiguration *test_conf = new FakeP2PManagerConfiguration();
    p2p_manager_.reset(P2PManager::Construct(
        test_conf, nullptr, &fake_um_, "cros_au", 3,
        base::TimeDelta::FromDays(5)));
    fake_system_state_.set_p2p_manager(p2p_manager_.get());
  }

  // To be called by tests to perform the download. The
  // |use_p2p_to_share| parameter is used to indicate whether the
  // payload should be shared via p2p.
  void StartDownload(bool use_p2p_to_share) {
    EXPECT_CALL(*fake_system_state_.mock_payload_state(),
                GetUsingP2PForSharing())
        .WillRepeatedly(Return(use_p2p_to_share));

    ScopedTempFile output_temp_file;
    TestDirectFileWriter writer;
    EXPECT_EQ(
        0, writer.Open(output_temp_file.path().c_str(), O_WRONLY | O_CREAT, 0));
    InstallPlan install_plan;
    install_plan.payload_size = data_.length();
    install_plan.payload_hash = "1234hash";
    ObjectFeederAction<InstallPlan> feeder_action;
    feeder_action.set_obj(install_plan);
    MockPrefs prefs;
    http_fetcher_ = new MockHttpFetcher(data_.c_str(),
                                        data_.length(),
                                        nullptr);
    // Note that DownloadAction takes ownership of the passed in HttpFetcher.
    download_action_.reset(new DownloadAction(&prefs,
                                              fake_system_state_.boot_control(),
                                              fake_system_state_.hardware(),
                                              &fake_system_state_,
                                              http_fetcher_));
    download_action_->SetTestFileWriter(&writer);
    BondActions(&feeder_action, download_action_.get());
    DownloadActionTestProcessorDelegate delegate(ErrorCode::kSuccess);
    delegate.expected_data_ = brillo::Blob(data_.begin() + start_at_offset_,
                                           data_.end());
    delegate.path_ = output_temp_file.path();
    processor_.set_delegate(&delegate);
    processor_.EnqueueAction(&feeder_action);
    processor_.EnqueueAction(download_action_.get());

    loop_.PostTask(FROM_HERE, base::Bind(
        &P2PDownloadActionTest::StartProcessorInRunLoopForP2P,
        base::Unretained(this)));
    loop_.Run();
  }

  // Mainloop used to make StartDownload() synchronous.
  brillo::FakeMessageLoop loop_{nullptr};

  // The DownloadAction instance under test.
  unique_ptr<DownloadAction> download_action_;

  // The HttpFetcher used in the test.
  MockHttpFetcher* http_fetcher_;

  // The P2PManager used in the test.
  unique_ptr<P2PManager> p2p_manager_;

  // The ActionProcessor used for running the actions.
  ActionProcessor processor_;

  // A fake system state.
  FakeSystemState fake_system_state_;

  // The data being downloaded.
  string data_;

 private:
  // Callback used in StartDownload() method.
  void StartProcessorInRunLoopForP2P() {
    processor_.StartProcessing();
    http_fetcher_->SetOffset(start_at_offset_);
  }

  // The requested starting offset passed to SetupDownload().
  off_t start_at_offset_;

  chromeos_update_manager::FakeUpdateManager fake_um_;
};

TEST_F(P2PDownloadActionTest, IsWrittenTo) {
  if (!test_utils::IsXAttrSupported(FilePath("/tmp"))) {
    LOG(WARNING) << "Skipping test because /tmp does not support xattr. "
                 << "Please update your system to support this feature.";
    return;
  }

  SetupDownload(0);     // starting_offset
  StartDownload(true);  // use_p2p_to_share

  // Check the p2p file and its content matches what was sent.
  string file_id = download_action_->p2p_file_id();
  EXPECT_NE("", file_id);
  EXPECT_EQ(static_cast<int>(data_.length()),
            p2p_manager_->FileGetSize(file_id));
  EXPECT_EQ(static_cast<int>(data_.length()),
            p2p_manager_->FileGetExpectedSize(file_id));
  string p2p_file_contents;
  EXPECT_TRUE(ReadFileToString(p2p_manager_->FileGetPath(file_id),
                               &p2p_file_contents));
  EXPECT_EQ(data_, p2p_file_contents);
}

TEST_F(P2PDownloadActionTest, DeleteIfHoleExists) {
  if (!test_utils::IsXAttrSupported(FilePath("/tmp"))) {
    LOG(WARNING) << "Skipping test because /tmp does not support xattr. "
                 << "Please update your system to support this feature.";
    return;
  }

  SetupDownload(1000);  // starting_offset
  StartDownload(true);  // use_p2p_to_share

  // DownloadAction should convey that the file is not being shared.
  // and that we don't have any p2p files.
  EXPECT_EQ(download_action_->p2p_file_id(), "");
  EXPECT_EQ(p2p_manager_->CountSharedFiles(), 0);
}

TEST_F(P2PDownloadActionTest, CanAppend) {
  if (!test_utils::IsXAttrSupported(FilePath("/tmp"))) {
    LOG(WARNING) << "Skipping test because /tmp does not support xattr. "
                 << "Please update your system to support this feature.";
    return;
  }

  SetupDownload(1000);  // starting_offset

  // Prepare the file with existing data before starting to write to
  // it via DownloadAction.
  string file_id = utils::CalculateP2PFileId("1234hash", data_.length());
  ASSERT_TRUE(p2p_manager_->FileShare(file_id, data_.length()));
  string existing_data;
  for (unsigned int i = 0; i < 1000; i++)
    existing_data += '0' + (i % 10);
  ASSERT_EQ(WriteFile(p2p_manager_->FileGetPath(file_id), existing_data.c_str(),
                      1000), 1000);

  StartDownload(true);  // use_p2p_to_share

  // DownloadAction should convey the same file_id and the file should
  // have the expected size.
  EXPECT_EQ(download_action_->p2p_file_id(), file_id);
  EXPECT_EQ(static_cast<ssize_t>(data_.length()),
            p2p_manager_->FileGetSize(file_id));
  EXPECT_EQ(static_cast<ssize_t>(data_.length()),
            p2p_manager_->FileGetExpectedSize(file_id));
  string p2p_file_contents;
  // Check that the first 1000 bytes wasn't touched and that we
  // appended the remaining as appropriate.
  EXPECT_TRUE(ReadFileToString(p2p_manager_->FileGetPath(file_id),
                               &p2p_file_contents));
  EXPECT_EQ(existing_data, p2p_file_contents.substr(0, 1000));
  EXPECT_EQ(data_.substr(1000), p2p_file_contents.substr(1000));
}

TEST_F(P2PDownloadActionTest, DeletePartialP2PFileIfResumingWithoutP2P) {
  if (!test_utils::IsXAttrSupported(FilePath("/tmp"))) {
    LOG(WARNING) << "Skipping test because /tmp does not support xattr. "
                 << "Please update your system to support this feature.";
    return;
  }

  SetupDownload(1000);  // starting_offset

  // Prepare the file with all existing data before starting to write
  // to it via DownloadAction.
  string file_id = utils::CalculateP2PFileId("1234hash", data_.length());
  ASSERT_TRUE(p2p_manager_->FileShare(file_id, data_.length()));
  string existing_data;
  for (unsigned int i = 0; i < 1000; i++)
    existing_data += '0' + (i % 10);
  ASSERT_EQ(WriteFile(p2p_manager_->FileGetPath(file_id), existing_data.c_str(),
                      1000), 1000);

  // Check that the file is there.
  EXPECT_EQ(1000, p2p_manager_->FileGetSize(file_id));
  EXPECT_EQ(1, p2p_manager_->CountSharedFiles());

  StartDownload(false);  // use_p2p_to_share

  // DownloadAction should have deleted the p2p file. Check that it's gone.
  EXPECT_EQ(-1, p2p_manager_->FileGetSize(file_id));
  EXPECT_EQ(0, p2p_manager_->CountSharedFiles());
}

}  // namespace chromeos_update_engine
