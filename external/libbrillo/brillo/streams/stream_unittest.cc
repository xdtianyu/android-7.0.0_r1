// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/stream.h>

#include <limits>

#include <base/callback.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <brillo/bind_lambda.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/streams/stream_errors.h>

using testing::DoAll;
using testing::InSequence;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::_;

namespace brillo {

using AccessMode = Stream::AccessMode;
using Whence = Stream::Whence;

// To verify "non-trivial" methods implemented in Stream, mock out the
// "trivial" methods to make sure the ones we are interested in testing
// actually end up calling the expected methods with right parameters.
class MockStreamImpl : public Stream {
 public:
  MockStreamImpl() = default;

  MOCK_CONST_METHOD0(IsOpen, bool());
  MOCK_CONST_METHOD0(CanRead, bool());
  MOCK_CONST_METHOD0(CanWrite, bool());
  MOCK_CONST_METHOD0(CanSeek, bool());
  MOCK_CONST_METHOD0(CanGetSize, bool());

  MOCK_CONST_METHOD0(GetSize, uint64_t());
  MOCK_METHOD2(SetSizeBlocking, bool(uint64_t, ErrorPtr*));
  MOCK_CONST_METHOD0(GetRemainingSize, uint64_t());

  MOCK_CONST_METHOD0(GetPosition, uint64_t());
  MOCK_METHOD4(Seek, bool(int64_t, Whence, uint64_t*, ErrorPtr*));

  // Omitted: ReadAsync
  // Omitted: ReadAllAsync
  MOCK_METHOD5(ReadNonBlocking, bool(void*, size_t, size_t*, bool*, ErrorPtr*));
  // Omitted: ReadBlocking
  // Omitted: ReadAllBlocking

  // Omitted: WriteAsync
  // Omitted: WriteAllAsync
  MOCK_METHOD4(WriteNonBlocking, bool(const void*, size_t, size_t*, ErrorPtr*));
  // Omitted: WriteBlocking
  // Omitted: WriteAllBlocking

  MOCK_METHOD1(FlushBlocking, bool(ErrorPtr*));
  MOCK_METHOD1(CloseBlocking, bool(ErrorPtr*));

  MOCK_METHOD3(WaitForData, bool(AccessMode,
                                 const base::Callback<void(AccessMode)>&,
                                 ErrorPtr*));
  MOCK_METHOD4(WaitForDataBlocking,
               bool(AccessMode, base::TimeDelta, AccessMode*, ErrorPtr*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockStreamImpl);
};

TEST(Stream, TruncateBlocking) {
  MockStreamImpl stream_mock;
  EXPECT_CALL(stream_mock, GetPosition()).WillOnce(Return(123));
  EXPECT_CALL(stream_mock, SetSizeBlocking(123, _)).WillOnce(Return(true));
  EXPECT_TRUE(stream_mock.TruncateBlocking(nullptr));
}

TEST(Stream, SetPosition) {
  MockStreamImpl stream_mock;
  EXPECT_CALL(stream_mock, Seek(12345, Whence::FROM_BEGIN, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(stream_mock.SetPosition(12345, nullptr));

  // Test too large an offset (that doesn't fit in signed 64 bit value).
  ErrorPtr error;
  uint64_t max_offset = std::numeric_limits<int64_t>::max();
  EXPECT_CALL(stream_mock, Seek(max_offset, _, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(stream_mock.SetPosition(max_offset, nullptr));

  EXPECT_FALSE(stream_mock.SetPosition(max_offset + 1, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kInvalidParameter, error->GetCode());
}

TEST(Stream, ReadAsync) {
  size_t read_size = 0;
  bool succeeded = false;
  bool failed = false;
  auto success_callback = [&read_size, &succeeded](size_t size) {
    read_size = size;
    succeeded = true;
  };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };

  MockStreamImpl stream_mock;
  base::Callback<void(AccessMode)> data_callback;
  char buf[10];

  // This sets up an initial non blocking read that would block, so ReadAsync()
  // should wait for more data.
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(0), SetArgPointee<3>(false), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::READ, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_mock.ReadAsync(buf, sizeof(buf),
                                    base::Bind(success_callback),
                                    base::Bind(error_callback), nullptr));
  EXPECT_EQ(0u, read_size);
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);

  // Since the previous call is waiting for the data to be available, we can't
  // schedule another read.
  ErrorPtr error;
  EXPECT_FALSE(stream_mock.ReadAsync(buf, sizeof(buf),
                                     base::Bind(success_callback),
                                     base::Bind(error_callback), &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kOperationNotSupported, error->GetCode());
  EXPECT_EQ("Another asynchronous operation is still pending",
            error->GetMessage());

  // Making the data available via data_callback should not schedule the
  // success callback from the main loop and run it directly instead.
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(7),
                      SetArgPointee<3>(false),
                      Return(true)));
  data_callback.Run(AccessMode::READ);
  EXPECT_EQ(7u, read_size);
  EXPECT_FALSE(failed);
}

TEST(Stream, ReadAsync_DontWaitForData) {
  bool succeeded = false;
  bool failed = false;
  auto success_callback = [&succeeded](size_t /* size */) { succeeded = true; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };

  MockStreamImpl stream_mock;
  char buf[10];
  FakeMessageLoop fake_loop_{nullptr};
  fake_loop_.SetAsCurrent();

  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(5), SetArgPointee<3>(false), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(_, _, _)).Times(0);
  EXPECT_TRUE(stream_mock.ReadAsync(buf, sizeof(buf),
                                    base::Bind(success_callback),
                                    base::Bind(error_callback), nullptr));
  // Even if ReadNonBlocking() returned some data without waiting, the
  // |success_callback| should not run yet.
  EXPECT_TRUE(fake_loop_.PendingTasks());
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);

  // Since the previous callback is still waiting in the main loop, we can't
  // schedule another read yet.
  ErrorPtr error;
  EXPECT_FALSE(stream_mock.ReadAsync(buf, sizeof(buf),
                                     base::Bind(success_callback),
                                     base::Bind(error_callback), &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kOperationNotSupported, error->GetCode());
  EXPECT_EQ("Another asynchronous operation is still pending",
            error->GetMessage());

  fake_loop_.Run();
  EXPECT_TRUE(succeeded);
  EXPECT_FALSE(failed);
}

TEST(Stream, ReadAllAsync) {
  bool succeeded = false;
  bool failed = false;
  auto success_callback = [&succeeded]() { succeeded = true; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };

  MockStreamImpl stream_mock;
  base::Callback<void(AccessMode)> data_callback;
  char buf[10];

  // This sets up an initial non blocking read that would block, so
  // ReadAllAsync() should wait for more data.
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(0), SetArgPointee<3>(false), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::READ, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_mock.ReadAllAsync(buf, sizeof(buf),
                                       base::Bind(success_callback),
                                       base::Bind(error_callback),
                                       nullptr));
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);
  testing::Mock::VerifyAndClearExpectations(&stream_mock);

  // ReadAllAsync() will try to read non blocking until the read would block
  // before it waits for the data to be available again.
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(7),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf + 7, 3, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(0), SetArgPointee<3>(false), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::READ, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  data_callback.Run(AccessMode::READ);
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);
  testing::Mock::VerifyAndClearExpectations(&stream_mock);

  EXPECT_CALL(stream_mock, ReadNonBlocking(buf + 7, 3, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(3),
                      SetArgPointee<3>(true),
                      Return(true)));
  data_callback.Run(AccessMode::READ);
  EXPECT_TRUE(succeeded);
  EXPECT_FALSE(failed);
}

TEST(Stream, ReadAllAsync_EOS) {
  bool succeeded = false;
  bool failed = false;
  auto success_callback = [&succeeded]() { succeeded = true; };
  auto error_callback = [&failed](const Error* error) {
    ASSERT_EQ(errors::stream::kDomain, error->GetDomain());
    ASSERT_EQ(errors::stream::kPartialData, error->GetCode());
    failed = true;
  };

  MockStreamImpl stream_mock;
  base::Callback<void(AccessMode)> data_callback;
  char buf[10];

  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(0), SetArgPointee<3>(false), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::READ, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_mock.ReadAllAsync(buf, sizeof(buf),
                                       base::Bind(success_callback),
                                       base::Bind(error_callback),
                                       nullptr));

  // ReadAsyncAll() should finish and fail once ReadNonBlocking() returns an
  // end-of-stream condition.
  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(7),
                      SetArgPointee<3>(true),
                      Return(true)));
  data_callback.Run(AccessMode::READ);
  EXPECT_FALSE(succeeded);
  EXPECT_TRUE(failed);
}

TEST(Stream, ReadBlocking) {
  MockStreamImpl stream_mock;
  char buf[1024];
  size_t read = 0;

  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(24),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_TRUE(stream_mock.ReadBlocking(buf, sizeof(buf), &read, nullptr));
  EXPECT_EQ(24, read);

  EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(true),
                      Return(true)));
  EXPECT_TRUE(stream_mock.ReadBlocking(buf, sizeof(buf), &read, nullptr));
  EXPECT_EQ(0, read);

  {
    InSequence seq;
    EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(0),
                        SetArgPointee<3>(false),
                        Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::READ, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(0),
                        SetArgPointee<3>(false),
                        Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::READ, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(124),
                        SetArgPointee<3>(false),
                        Return(true)));
  }
  EXPECT_TRUE(stream_mock.ReadBlocking(buf, sizeof(buf), &read, nullptr));
  EXPECT_EQ(124, read);

  {
    InSequence seq;
    EXPECT_CALL(stream_mock, ReadNonBlocking(buf, 1024, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(0),
                        SetArgPointee<3>(false),
                        Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::READ, _, _, _))
        .WillOnce(Return(false));
  }
  EXPECT_FALSE(stream_mock.ReadBlocking(buf, sizeof(buf), &read, nullptr));
}

TEST(Stream, ReadAllBlocking) {
  class MockReadBlocking : public MockStreamImpl {
   public:
    MOCK_METHOD4(ReadBlocking, bool(void*, size_t, size_t*, ErrorPtr*));
  } stream_mock;

  char buf[1024];

  EXPECT_CALL(stream_mock, ReadBlocking(buf, 1024, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(24), Return(true)));
  EXPECT_CALL(stream_mock, ReadBlocking(buf + 24, 1000, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(1000), Return(true)));
  EXPECT_TRUE(stream_mock.ReadAllBlocking(buf, sizeof(buf), nullptr));

  ErrorPtr error;
  EXPECT_CALL(stream_mock, ReadBlocking(buf, 1024, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(24), Return(true)));
  EXPECT_CALL(stream_mock, ReadBlocking(buf + 24, 1000, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
  EXPECT_FALSE(stream_mock.ReadAllBlocking(buf, sizeof(buf), &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kPartialData, error->GetCode());
}

TEST(Stream, WriteAsync) {
  size_t write_size = 0;
  bool failed = false;
  auto success_callback = [&write_size](size_t size) { write_size = size; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };

  MockStreamImpl stream_mock;
  InSequence s;
  base::Callback<void(AccessMode)> data_callback;
  char buf[10] = {};

  // WriteNonBlocking returns a blocking situation (size_written = 0) so the
  // WaitForData() is run.
  EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::WRITE, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_mock.WriteAsync(buf, sizeof(buf),
                                     base::Bind(success_callback),
                                     base::Bind(error_callback), nullptr));
  EXPECT_EQ(0u, write_size);
  EXPECT_FALSE(failed);

  ErrorPtr error;
  EXPECT_FALSE(stream_mock.WriteAsync(buf, sizeof(buf),
                                      base::Bind(success_callback),
                                      base::Bind(error_callback), &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kOperationNotSupported, error->GetCode());
  EXPECT_EQ("Another asynchronous operation is still pending",
            error->GetMessage());

  EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(7), Return(true)));
  data_callback.Run(AccessMode::WRITE);
  EXPECT_EQ(7u, write_size);
  EXPECT_FALSE(failed);
}

TEST(Stream, WriteAllAsync) {
  bool succeeded = false;
  bool failed = false;
  auto success_callback = [&succeeded]() { succeeded = true; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };

  MockStreamImpl stream_mock;
  base::Callback<void(AccessMode)> data_callback;
  char buf[10] = {};

  EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::WRITE, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_mock.WriteAllAsync(buf, sizeof(buf),
                                        base::Bind(success_callback),
                                        base::Bind(error_callback),
                                        nullptr));
  testing::Mock::VerifyAndClearExpectations(&stream_mock);
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);

  EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(7), Return(true)));
  EXPECT_CALL(stream_mock, WriteNonBlocking(buf + 7, 3, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
  EXPECT_CALL(stream_mock, WaitForData(AccessMode::WRITE, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  data_callback.Run(AccessMode::WRITE);
  testing::Mock::VerifyAndClearExpectations(&stream_mock);
  EXPECT_FALSE(succeeded);
  EXPECT_FALSE(failed);

  EXPECT_CALL(stream_mock, WriteNonBlocking(buf + 7, 3, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(3), Return(true)));
  data_callback.Run(AccessMode::WRITE);
  EXPECT_TRUE(succeeded);
  EXPECT_FALSE(failed);
}

TEST(Stream, WriteBlocking) {
  MockStreamImpl stream_mock;
  char buf[1024];
  size_t written = 0;

  EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 1024, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(24), Return(true)));
  EXPECT_TRUE(stream_mock.WriteBlocking(buf, sizeof(buf), &written, nullptr));
  EXPECT_EQ(24, written);

  {
    InSequence seq;
    EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 1024, _, _))
          .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::WRITE, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 1024, _, _))
          .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::WRITE, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 1024, _, _))
          .WillOnce(DoAll(SetArgPointee<2>(124), Return(true)));
  }
  EXPECT_TRUE(stream_mock.WriteBlocking(buf, sizeof(buf), &written, nullptr));
  EXPECT_EQ(124, written);

  {
    InSequence seq;
    EXPECT_CALL(stream_mock, WriteNonBlocking(buf, 1024, _, _))
          .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
    EXPECT_CALL(stream_mock, WaitForDataBlocking(AccessMode::WRITE, _, _, _))
        .WillOnce(Return(false));
  }
  EXPECT_FALSE(stream_mock.WriteBlocking(buf, sizeof(buf), &written, nullptr));
}

TEST(Stream, WriteAllBlocking) {
  class MockWritelocking : public MockStreamImpl {
   public:
    MOCK_METHOD4(WriteBlocking, bool(const void*, size_t, size_t*, ErrorPtr*));
  } stream_mock;

  char buf[1024];

  EXPECT_CALL(stream_mock, WriteBlocking(buf, 1024, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(24), Return(true)));
  EXPECT_CALL(stream_mock, WriteBlocking(buf + 24, 1000, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(1000), Return(true)));
  EXPECT_TRUE(stream_mock.WriteAllBlocking(buf, sizeof(buf), nullptr));
}

}  // namespace brillo
