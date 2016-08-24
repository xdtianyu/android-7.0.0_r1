// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/file_stream.h>

#include <limits>
#include <numeric>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/message_loop/message_loop.h>
#include <base/rand_util.h>
#include <base/run_loop.h>
#include <brillo/bind_lambda.h>
#include <brillo/errors/error_codes.h>
#include <brillo/message_loops/base_message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>
#include <brillo/streams/stream_errors.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::InSequence;
using testing::Return;
using testing::ReturnArg;
using testing::SaveArg;
using testing::SetErrnoAndReturn;
using testing::_;

namespace brillo {

namespace {

// gmock action that would return a blocking situation from a read() or write().
ACTION(ReturnWouldBlock) {
  errno = EWOULDBLOCK;
  return -1;
}

// Helper function to read one byte from the stream.
inline int ReadByte(Stream* stream) {
  uint8_t byte = 0;
  return stream->ReadAllBlocking(&byte, sizeof(byte), nullptr) ? byte : -1;
}

// Helper function to write one byte from the stream.
inline bool WriteByte(Stream* stream, uint8_t byte) {
  return stream->WriteAllBlocking(&byte, sizeof(byte), nullptr);
}

// Helper function to test file stream workflow on newly created file.
void TestCreateFile(Stream* stream) {
  ASSERT_TRUE(stream->IsOpen());

  // Set up a sample data buffer.
  std::vector<uint8_t> in_buffer(256);
  std::iota(in_buffer.begin(), in_buffer.end(), 0);

  // Initial assumptions about empty file stream.
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(0, stream->GetSize());

  // Write sample data.
  EXPECT_TRUE(stream->WriteAllBlocking(in_buffer.data(), in_buffer.size(),
                                       nullptr));
  EXPECT_EQ(in_buffer.size(), stream->GetPosition());
  EXPECT_EQ(in_buffer.size(), stream->GetSize());

  // Rewind the stream to the beginning.
  uint64_t pos = 0;
  EXPECT_TRUE(stream->Seek(0, Stream::Whence::FROM_BEGIN, &pos, nullptr));
  EXPECT_EQ(0, pos);
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(in_buffer.size(), stream->GetSize());

  // Read the file contents back.
  std::vector<uint8_t> out_buffer(256);
  EXPECT_TRUE(stream->ReadAllBlocking(out_buffer.data(), out_buffer.size(),
                                      nullptr));
  EXPECT_EQ(out_buffer.size(), stream->GetPosition());
  EXPECT_EQ(out_buffer.size(), stream->GetSize());

  // Make sure the data read matches those written.
  EXPECT_EQ(in_buffer, out_buffer);

  // Random read/write
  EXPECT_TRUE(stream->Seek(10, Stream::Whence::FROM_BEGIN, &pos, nullptr));
  EXPECT_EQ(10, pos);

  // Since our data buffer contained values from 0 to 255, the byte at position
  // 10 will contain the value of 10.
  EXPECT_EQ(10, ReadByte(stream));
  EXPECT_EQ(11, ReadByte(stream));
  EXPECT_EQ(12, ReadByte(stream));
  EXPECT_TRUE(stream->Seek(7, Stream::Whence::FROM_CURRENT, nullptr, nullptr));
  EXPECT_EQ(20, ReadByte(stream));

  EXPECT_EQ(21, stream->GetPosition());
  EXPECT_TRUE(stream->Seek(-2, Stream::Whence::FROM_CURRENT, &pos, nullptr));
  EXPECT_EQ(19, pos);
  EXPECT_TRUE(WriteByte(stream, 100));
  EXPECT_EQ(20, ReadByte(stream));
  EXPECT_TRUE(stream->Seek(-2, Stream::Whence::FROM_CURRENT, nullptr, nullptr));
  EXPECT_EQ(100, ReadByte(stream));
  EXPECT_EQ(20, ReadByte(stream));
  EXPECT_TRUE(stream->Seek(-1, Stream::Whence::FROM_END, &pos, nullptr));
  EXPECT_EQ(255, pos);
  EXPECT_EQ(255, ReadByte(stream));
  EXPECT_EQ(-1, ReadByte(stream));
}

}  // anonymous namespace

// A mock file descriptor wrapper to test low-level file API used by FileStream.
class MockFileDescriptor : public FileStream::FileDescriptorInterface {
 public:
  MOCK_CONST_METHOD0(IsOpen, bool());
  MOCK_METHOD2(Read, ssize_t(void*, size_t));
  MOCK_METHOD2(Write, ssize_t(const void*, size_t));
  MOCK_METHOD2(Seek, off64_t(off64_t, int));
  MOCK_CONST_METHOD0(GetFileMode, mode_t());
  MOCK_CONST_METHOD0(GetSize, uint64_t());
  MOCK_CONST_METHOD1(Truncate, int(off64_t));
  MOCK_METHOD0(Flush, int());
  MOCK_METHOD0(Close, int());
  MOCK_METHOD3(WaitForData,
               bool(Stream::AccessMode, const DataCallback&, ErrorPtr*));
  MOCK_METHOD3(WaitForDataBlocking,
               int(Stream::AccessMode, base::TimeDelta, Stream::AccessMode*));
  MOCK_METHOD0(CancelPendingAsyncOperations, void());
};

class FileStreamTest : public testing::Test {
 public:
  void SetUp() override {
    CreateStream(S_IFREG, Stream::AccessMode::READ_WRITE);
  }

  MockFileDescriptor& fd_mock() {
    return *static_cast<MockFileDescriptor*>(stream_->fd_interface_.get());
  }

  void CreateStream(mode_t file_mode, Stream::AccessMode access_mode) {
    std::unique_ptr<MockFileDescriptor> fd{new MockFileDescriptor{}};
    EXPECT_CALL(*fd, GetFileMode()).WillOnce(Return(file_mode));
    stream_.reset(new FileStream(std::move(fd), access_mode));
    EXPECT_CALL(fd_mock(), IsOpen()).WillRepeatedly(Return(true));
  }

  void ExpectStreamClosed(const ErrorPtr& error) const {
    EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
    EXPECT_EQ(errors::stream::kStreamClosed, error->GetCode());
    EXPECT_EQ("Stream is closed", error->GetMessage());
  }

  void ExpectStreamOffsetTooLarge(const ErrorPtr& error) const {
    EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
    EXPECT_EQ(errors::stream::kInvalidParameter, error->GetCode());
    EXPECT_EQ("The stream offset value is out of range", error->GetMessage());
  }

  inline static char* IntToPtr(int addr) {
    return reinterpret_cast<char*>(addr);
  }

  inline static const char* IntToConstPtr(int addr) {
    return reinterpret_cast<const char*>(addr);
  }

  bool CallWaitForData(Stream::AccessMode mode, ErrorPtr* error) {
    return stream_->WaitForData(mode, {}, error);
  }

  std::unique_ptr<FileStream> stream_;

  const uint64_t kMaxSize = std::numeric_limits<int64_t>::max();
  const uint64_t kTooLargeSize = std::numeric_limits<uint64_t>::max();

  // Dummy buffer pointer values to make sure that input pointer values
  // are delegated to the file interface without a change.
  char* const test_read_buffer_ = IntToPtr(12345);
  const char* const test_write_buffer_ = IntToConstPtr(67890);
};

TEST_F(FileStreamTest, IsOpen) {
  EXPECT_TRUE(stream_->IsOpen());
  EXPECT_CALL(fd_mock(), IsOpen()).WillOnce(Return(false));
  EXPECT_FALSE(stream_->IsOpen());
}

TEST_F(FileStreamTest, CanRead) {
  CreateStream(S_IFREG, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanRead());
  EXPECT_CALL(fd_mock(), IsOpen()).WillRepeatedly(Return(false));
  EXPECT_FALSE(stream_->CanRead());
  CreateStream(S_IFREG, Stream::AccessMode::READ);
  EXPECT_TRUE(stream_->CanRead());
  CreateStream(S_IFREG, Stream::AccessMode::WRITE);
  EXPECT_FALSE(stream_->CanRead());
}

TEST_F(FileStreamTest, CanWrite) {
  CreateStream(S_IFREG, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanWrite());
  EXPECT_CALL(fd_mock(), IsOpen()).WillRepeatedly(Return(false));
  EXPECT_FALSE(stream_->CanWrite());
  CreateStream(S_IFREG, Stream::AccessMode::READ);
  EXPECT_FALSE(stream_->CanWrite());
  CreateStream(S_IFREG, Stream::AccessMode::WRITE);
  EXPECT_TRUE(stream_->CanWrite());
}

TEST_F(FileStreamTest, CanSeek) {
  CreateStream(S_IFBLK, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanSeek());
  CreateStream(S_IFDIR, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanSeek());
  CreateStream(S_IFREG, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanSeek());
  CreateStream(S_IFLNK, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanSeek());
  CreateStream(S_IFCHR, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanSeek());
  CreateStream(S_IFSOCK, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanSeek());
  CreateStream(S_IFIFO, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanSeek());

  CreateStream(S_IFREG, Stream::AccessMode::READ);
  EXPECT_TRUE(stream_->CanSeek());
  CreateStream(S_IFREG, Stream::AccessMode::WRITE);
  EXPECT_TRUE(stream_->CanSeek());
}

TEST_F(FileStreamTest, CanGetSize) {
  CreateStream(S_IFBLK, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanGetSize());
  CreateStream(S_IFDIR, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanGetSize());
  CreateStream(S_IFREG, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanGetSize());
  CreateStream(S_IFLNK, Stream::AccessMode::READ_WRITE);
  EXPECT_TRUE(stream_->CanGetSize());
  CreateStream(S_IFCHR, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanGetSize());
  CreateStream(S_IFSOCK, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanGetSize());
  CreateStream(S_IFIFO, Stream::AccessMode::READ_WRITE);
  EXPECT_FALSE(stream_->CanGetSize());

  CreateStream(S_IFREG, Stream::AccessMode::READ);
  EXPECT_TRUE(stream_->CanGetSize());
  CreateStream(S_IFREG, Stream::AccessMode::WRITE);
  EXPECT_TRUE(stream_->CanGetSize());
}

TEST_F(FileStreamTest, GetSize) {
  EXPECT_CALL(fd_mock(), GetSize()).WillRepeatedly(Return(12345));
  EXPECT_EQ(12345u, stream_->GetSize());
  EXPECT_CALL(fd_mock(), IsOpen()).WillOnce(Return(false));
  EXPECT_EQ(0u, stream_->GetSize());
}

TEST_F(FileStreamTest, SetSizeBlocking) {
  EXPECT_CALL(fd_mock(), Truncate(0)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->SetSizeBlocking(0, nullptr));

  EXPECT_CALL(fd_mock(), Truncate(123)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->SetSizeBlocking(123, nullptr));

  EXPECT_CALL(fd_mock(), Truncate(kMaxSize)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->SetSizeBlocking(kMaxSize, nullptr));
}

TEST_F(FileStreamTest, SetSizeBlocking_Fail) {
  brillo::ErrorPtr error;

  EXPECT_CALL(fd_mock(), Truncate(1235)).WillOnce(SetErrnoAndReturn(EIO, -1));
  EXPECT_FALSE(stream_->SetSizeBlocking(1235, &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EIO", error->GetCode());

  error.reset();
  EXPECT_FALSE(stream_->SetSizeBlocking(kTooLargeSize, &error));
  ExpectStreamOffsetTooLarge(error);

  error.reset();
  EXPECT_CALL(fd_mock(), IsOpen()).WillOnce(Return(false));
  EXPECT_FALSE(stream_->SetSizeBlocking(1235, &error));
  ExpectStreamClosed(error);
}

TEST_F(FileStreamTest, GetRemainingSize) {
  EXPECT_CALL(fd_mock(), Seek(0, SEEK_CUR)).WillOnce(Return(234));
  EXPECT_CALL(fd_mock(), GetSize()).WillOnce(Return(1234));
  EXPECT_EQ(1000u, stream_->GetRemainingSize());

  EXPECT_CALL(fd_mock(), Seek(0, SEEK_CUR)).WillOnce(Return(1234));
  EXPECT_CALL(fd_mock(), GetSize()).WillOnce(Return(1000));
  EXPECT_EQ(0u, stream_->GetRemainingSize());
}

TEST_F(FileStreamTest, Seek_Set) {
  uint64_t pos = 0;

  EXPECT_CALL(fd_mock(), Seek(0, SEEK_SET)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->Seek(0, Stream::Whence::FROM_BEGIN, &pos, nullptr));
  EXPECT_EQ(0u, pos);

  EXPECT_CALL(fd_mock(), Seek(123456, SEEK_SET)).WillOnce(Return(123456));
  EXPECT_TRUE(stream_->Seek(123456, Stream::Whence::FROM_BEGIN, &pos, nullptr));
  EXPECT_EQ(123456u, pos);

  EXPECT_CALL(fd_mock(), Seek(kMaxSize, SEEK_SET))
      .WillRepeatedly(Return(kMaxSize));
  EXPECT_TRUE(stream_->Seek(kMaxSize, Stream::Whence::FROM_BEGIN, &pos,
              nullptr));
  EXPECT_EQ(kMaxSize, pos);
  EXPECT_TRUE(stream_->Seek(kMaxSize, Stream::Whence::FROM_BEGIN, nullptr,
              nullptr));
}

TEST_F(FileStreamTest, Seek_Cur) {
  uint64_t pos = 0;

  EXPECT_CALL(fd_mock(), Seek(0, SEEK_CUR)).WillOnce(Return(100));
  EXPECT_TRUE(stream_->Seek(0, Stream::Whence::FROM_CURRENT, &pos, nullptr));
  EXPECT_EQ(100u, pos);

  EXPECT_CALL(fd_mock(), Seek(234, SEEK_CUR)).WillOnce(Return(1234));
  EXPECT_TRUE(stream_->Seek(234, Stream::Whence::FROM_CURRENT, &pos, nullptr));
  EXPECT_EQ(1234u, pos);

  EXPECT_CALL(fd_mock(), Seek(-100, SEEK_CUR)).WillOnce(Return(900));
  EXPECT_TRUE(stream_->Seek(-100, Stream::Whence::FROM_CURRENT, &pos, nullptr));
  EXPECT_EQ(900u, pos);
}

TEST_F(FileStreamTest, Seek_End) {
  uint64_t pos = 0;

  EXPECT_CALL(fd_mock(), Seek(0, SEEK_END)).WillOnce(Return(1000));
  EXPECT_TRUE(stream_->Seek(0, Stream::Whence::FROM_END, &pos, nullptr));
  EXPECT_EQ(1000u, pos);

  EXPECT_CALL(fd_mock(), Seek(234, SEEK_END)).WillOnce(Return(10234));
  EXPECT_TRUE(stream_->Seek(234, Stream::Whence::FROM_END, &pos, nullptr));
  EXPECT_EQ(10234u, pos);

  EXPECT_CALL(fd_mock(), Seek(-100, SEEK_END)).WillOnce(Return(9900));
  EXPECT_TRUE(stream_->Seek(-100, Stream::Whence::FROM_END, &pos, nullptr));
  EXPECT_EQ(9900u, pos);
}

TEST_F(FileStreamTest, Seek_Fail) {
  brillo::ErrorPtr error;
  EXPECT_CALL(fd_mock(), Seek(0, SEEK_SET))
      .WillOnce(SetErrnoAndReturn(EPIPE, -1));
  EXPECT_FALSE(stream_->Seek(0, Stream::Whence::FROM_BEGIN, nullptr, &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EPIPE", error->GetCode());
}

TEST_F(FileStreamTest, ReadAsync) {
  size_t read_size = 0;
  bool failed = false;
  auto success_callback = [&read_size](size_t size) { read_size = size; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };
  FileStream::FileDescriptorInterface::DataCallback data_callback;

  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 100))
      .WillOnce(ReturnWouldBlock());
  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::READ, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_->ReadAsync(test_read_buffer_, 100,
                                 base::Bind(success_callback),
                                 base::Bind(error_callback),
                                 nullptr));
  EXPECT_EQ(0u, read_size);
  EXPECT_FALSE(failed);

  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 100)).WillOnce(Return(83));
  data_callback.Run(Stream::AccessMode::READ);
  EXPECT_EQ(83u, read_size);
  EXPECT_FALSE(failed);
}

TEST_F(FileStreamTest, ReadNonBlocking) {
  size_t size = 0;
  bool eos = false;
  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, _))
      .WillRepeatedly(ReturnArg<1>());
  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 100, &size, &eos,
                                       nullptr));
  EXPECT_EQ(100u, size);
  EXPECT_FALSE(eos);

  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 0, &size, &eos,
                                       nullptr));
  EXPECT_EQ(0u, size);
  EXPECT_FALSE(eos);

  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, _)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 100, &size, &eos,
                                       nullptr));
  EXPECT_EQ(0u, size);
  EXPECT_TRUE(eos);

  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, _))
      .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 100, &size, &eos,
                                       nullptr));
  EXPECT_EQ(0u, size);
  EXPECT_FALSE(eos);
}

TEST_F(FileStreamTest, ReadNonBlocking_Fail) {
  size_t size = 0;
  brillo::ErrorPtr error;
  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, _))
      .WillOnce(SetErrnoAndReturn(EACCES, -1));
  EXPECT_FALSE(stream_->ReadNonBlocking(test_read_buffer_, 100, &size, nullptr,
                                        &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EACCES", error->GetCode());
}

TEST_F(FileStreamTest, ReadBlocking) {
  size_t size = 0;
  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 100)).WillOnce(Return(20));
  EXPECT_TRUE(stream_->ReadBlocking(test_read_buffer_, 100, &size, nullptr));
  EXPECT_EQ(20u, size);

  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::READ, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 80)).WillOnce(Return(45));
  }
  EXPECT_TRUE(stream_->ReadBlocking(test_read_buffer_, 80, &size, nullptr));
  EXPECT_EQ(45u, size);

  EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 50)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->ReadBlocking(test_read_buffer_, 50, &size, nullptr));
  EXPECT_EQ(0u, size);
}

TEST_F(FileStreamTest, ReadBlocking_Fail) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::READ, _, _))
        .WillOnce(SetErrnoAndReturn(EBADF, -1));
  }
  brillo::ErrorPtr error;
  size_t size = 0;
  EXPECT_FALSE(stream_->ReadBlocking(test_read_buffer_, 80, &size, &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EBADF", error->GetCode());
}

TEST_F(FileStreamTest, ReadAllBlocking) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 100)).WillOnce(Return(20));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 20, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::READ, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 20, 80))
        .WillOnce(Return(45));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 65, 35))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::READ, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 65, 35))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::READ, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 65, 35))
        .WillOnce(Return(35));
  }
  EXPECT_TRUE(stream_->ReadAllBlocking(test_read_buffer_, 100, nullptr));
}

TEST_F(FileStreamTest, ReadAllBlocking_Fail) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_, 100)).WillOnce(Return(20));
    EXPECT_CALL(fd_mock(), Read(test_read_buffer_ + 20, 80))
        .WillOnce(Return(0));
  }
  brillo::ErrorPtr error;
  EXPECT_FALSE(stream_->ReadAllBlocking(test_read_buffer_, 100, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kPartialData, error->GetCode());
  EXPECT_EQ("Reading past the end of stream", error->GetMessage());
}

TEST_F(FileStreamTest, WriteAsync) {
  size_t write_size = 0;
  bool failed = false;
  auto success_callback = [&write_size](size_t size) { write_size = size; };
  auto error_callback = [&failed](const Error* /* error */) { failed = true; };
  FileStream::FileDescriptorInterface::DataCallback data_callback;

  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 100))
      .WillOnce(ReturnWouldBlock());
  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::WRITE, _, _))
      .WillOnce(DoAll(SaveArg<1>(&data_callback), Return(true)));
  EXPECT_TRUE(stream_->WriteAsync(test_write_buffer_, 100,
                                  base::Bind(success_callback),
                                  base::Bind(error_callback),
                                  nullptr));
  EXPECT_EQ(0u, write_size);
  EXPECT_FALSE(failed);

  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 100)).WillOnce(Return(87));
  data_callback.Run(Stream::AccessMode::WRITE);
  EXPECT_EQ(87u, write_size);
  EXPECT_FALSE(failed);
}

TEST_F(FileStreamTest, WriteNonBlocking) {
  size_t size = 0;
  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, _))
      .WillRepeatedly(ReturnArg<1>());
  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 100, &size,
                                        nullptr));
  EXPECT_EQ(100u, size);

  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 0, &size, nullptr));
  EXPECT_EQ(0u, size);

  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, _)).WillOnce(Return(0));
  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 100, &size,
                                        nullptr));
  EXPECT_EQ(0u, size);

  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, _))
      .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 100, &size,
                                        nullptr));
  EXPECT_EQ(0u, size);
}

TEST_F(FileStreamTest, WriteNonBlocking_Fail) {
  size_t size = 0;
  brillo::ErrorPtr error;
  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, _))
      .WillOnce(SetErrnoAndReturn(EACCES, -1));
  EXPECT_FALSE(stream_->WriteNonBlocking(test_write_buffer_, 100, &size,
                                         &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EACCES", error->GetCode());
}

TEST_F(FileStreamTest, WriteBlocking) {
  size_t size = 0;
  EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 100)).WillOnce(Return(20));
  EXPECT_TRUE(stream_->WriteBlocking(test_write_buffer_, 100, &size, nullptr));
  EXPECT_EQ(20u, size);

  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 80)).WillOnce(Return(45));
  }
  EXPECT_TRUE(stream_->WriteBlocking(test_write_buffer_, 80, &size, nullptr));
  EXPECT_EQ(45u, size);

  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 50)).WillOnce(Return(0));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 50)).WillOnce(Return(1));
  }
  EXPECT_TRUE(stream_->WriteBlocking(test_write_buffer_, 50, &size, nullptr));
  EXPECT_EQ(1u, size);
}

TEST_F(FileStreamTest, WriteBlocking_Fail) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(SetErrnoAndReturn(EBADF, -1));
  }
  brillo::ErrorPtr error;
  size_t size = 0;
  EXPECT_FALSE(stream_->WriteBlocking(test_write_buffer_, 80, &size, &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EBADF", error->GetCode());
}

TEST_F(FileStreamTest, WriteAllBlocking) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 100)).WillOnce(Return(20));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_ + 20, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_ + 20, 80))
        .WillOnce(Return(45));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_ + 65, 35))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_ + 65, 35))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(Return(1));
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_ + 65, 35))
        .WillOnce(Return(35));
  }
  EXPECT_TRUE(stream_->WriteAllBlocking(test_write_buffer_, 100, nullptr));
}

TEST_F(FileStreamTest, WriteAllBlocking_Fail) {
  {
    InSequence seq;
    EXPECT_CALL(fd_mock(), Write(test_write_buffer_, 80))
        .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
        .WillOnce(SetErrnoAndReturn(EBADF, -1));
  }
  brillo::ErrorPtr error;
  EXPECT_FALSE(stream_->WriteAllBlocking(test_write_buffer_, 80, &error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EBADF", error->GetCode());
}

TEST_F(FileStreamTest, WaitForDataBlocking_Timeout) {
  EXPECT_CALL(fd_mock(), WaitForDataBlocking(Stream::AccessMode::WRITE, _, _))
      .WillOnce(Return(0));
  brillo::ErrorPtr error;
  EXPECT_FALSE(stream_->WaitForDataBlocking(Stream::AccessMode::WRITE, {},
                                            nullptr, &error));
  EXPECT_EQ(errors::stream::kDomain, error->GetDomain());
  EXPECT_EQ(errors::stream::kTimeout, error->GetCode());
}

TEST_F(FileStreamTest, FlushBlocking) {
  EXPECT_TRUE(stream_->FlushBlocking(nullptr));
}

TEST_F(FileStreamTest, CloseBlocking) {
  EXPECT_CALL(fd_mock(), Close()).WillOnce(Return(0));
  EXPECT_TRUE(stream_->CloseBlocking(nullptr));

  EXPECT_CALL(fd_mock(), IsOpen()).WillOnce(Return(false));
  EXPECT_TRUE(stream_->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, CloseBlocking_Fail) {
  brillo::ErrorPtr error;
  EXPECT_CALL(fd_mock(), Close()).WillOnce(SetErrnoAndReturn(EFBIG, -1));
  EXPECT_FALSE(stream_->CloseBlocking(&error));
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EFBIG", error->GetCode());
}

TEST_F(FileStreamTest, WaitForData) {
  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::READ, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(CallWaitForData(Stream::AccessMode::READ, nullptr));

  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::WRITE, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(CallWaitForData(Stream::AccessMode::WRITE, nullptr));

  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::READ_WRITE, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(CallWaitForData(Stream::AccessMode::READ_WRITE, nullptr));

  EXPECT_CALL(fd_mock(), WaitForData(Stream::AccessMode::READ_WRITE, _, _))
      .WillOnce(Return(false));
  EXPECT_FALSE(CallWaitForData(Stream::AccessMode::READ_WRITE, nullptr));
}

TEST_F(FileStreamTest, CreateTemporary) {
  StreamPtr stream = FileStream::CreateTemporary(nullptr);
  ASSERT_NE(nullptr, stream.get());
  TestCreateFile(stream.get());
}

TEST_F(FileStreamTest, OpenRead) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::vector<char> buffer(1024 * 1024);
  base::RandBytes(buffer.data(), buffer.size());
  int file_size = buffer.size();  // Stupid base::WriteFile taking "int" size.
  ASSERT_EQ(file_size, base::WriteFile(path, buffer.data(), file_size));

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ,
                                      FileStream::Disposition::OPEN_EXISTING,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  ASSERT_TRUE(stream->IsOpen());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_FALSE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(buffer.size(), stream->GetSize());

  std::vector<char> buffer2(buffer.size());
  EXPECT_TRUE(stream->ReadAllBlocking(buffer2.data(), buffer2.size(), nullptr));
  EXPECT_EQ(buffer2, buffer);
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, OpenWrite) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::vector<char> buffer(1024 * 1024);
  base::RandBytes(buffer.data(), buffer.size());

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::WRITE,
                                      FileStream::Disposition::CREATE_ALWAYS,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  ASSERT_TRUE(stream->IsOpen());
  EXPECT_FALSE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(0u, stream->GetSize());

  EXPECT_TRUE(stream->WriteAllBlocking(buffer.data(), buffer.size(), nullptr));
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  std::vector<char> buffer2(buffer.size());
  int file_size = buffer2.size();  // Stupid base::ReadFile taking "int" size.
  ASSERT_EQ(file_size, base::ReadFile(path, buffer2.data(), file_size));
  EXPECT_EQ(buffer2, buffer);
}

TEST_F(FileStreamTest, Open_OpenExisting) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::string data{"Lorem ipsum dolor sit amet ..."};
  int data_size = data.size();  // I hate ints for data size...
  ASSERT_EQ(data_size, base::WriteFile(path, data.data(), data_size));

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::OPEN_EXISTING,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(data.size(), stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, Open_OpenExisting_Fail) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});

  ErrorPtr error;
  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::OPEN_EXISTING,
                                      &error);
  ASSERT_EQ(nullptr, stream.get());
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("ENOENT", error->GetCode());
}

TEST_F(FileStreamTest, Open_CreateAlways_New) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::CREATE_ALWAYS,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(0u, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, Open_CreateAlways_Existing) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::string data{"Lorem ipsum dolor sit amet ..."};
  int data_size = data.size();  // I hate ints for data size...
  ASSERT_EQ(data_size, base::WriteFile(path, data.data(), data_size));

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::CREATE_ALWAYS,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(0u, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, Open_CreateNewOnly_New) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});

  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::CREATE_NEW_ONLY,
                                      nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(0u, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, Open_CreateNewOnly_Existing) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::string data{"Lorem ipsum dolor sit amet ..."};
  int data_size = data.size();  // I hate ints for data size...
  ASSERT_EQ(data_size, base::WriteFile(path, data.data(), data_size));

  ErrorPtr error;
  StreamPtr stream = FileStream::Open(path,
                                      Stream::AccessMode::READ_WRITE,
                                      FileStream::Disposition::CREATE_NEW_ONLY,
                                      &error);
  ASSERT_EQ(nullptr, stream.get());
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EEXIST", error->GetCode());
}

TEST_F(FileStreamTest, Open_TruncateExisting_New) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});

  ErrorPtr error;
  StreamPtr stream = FileStream::Open(
      path,
      Stream::AccessMode::READ_WRITE,
      FileStream::Disposition::TRUNCATE_EXISTING,
      &error);
  ASSERT_EQ(nullptr, stream.get());
  EXPECT_EQ(errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("ENOENT", error->GetCode());
}

TEST_F(FileStreamTest, Open_TruncateExisting_Existing) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.path().Append(base::FilePath{"test.dat"});
  std::string data{"Lorem ipsum dolor sit amet ..."};
  int data_size = data.size();  // I hate ints for data size...
  ASSERT_EQ(data_size, base::WriteFile(path, data.data(), data_size));

  StreamPtr stream = FileStream::Open(
      path,
      Stream::AccessMode::READ_WRITE,
      FileStream::Disposition::TRUNCATE_EXISTING,
      nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_TRUE(stream->CanSeek());
  EXPECT_TRUE(stream->CanGetSize());
  EXPECT_EQ(0u, stream->GetPosition());
  EXPECT_EQ(0u, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, FromFileDescriptor_StdIn) {
  StreamPtr stream =
      FileStream::FromFileDescriptor(STDIN_FILENO, false, nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_FALSE(stream->CanSeek());
  EXPECT_FALSE(stream->CanGetSize());
}

TEST_F(FileStreamTest, FromFileDescriptor_StdOut) {
  StreamPtr stream =
      FileStream::FromFileDescriptor(STDOUT_FILENO, false, nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_FALSE(stream->CanSeek());
  EXPECT_FALSE(stream->CanGetSize());
}

TEST_F(FileStreamTest, FromFileDescriptor_StdErr) {
  StreamPtr stream =
      FileStream::FromFileDescriptor(STDERR_FILENO, false, nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_FALSE(stream->CanSeek());
  EXPECT_FALSE(stream->CanGetSize());
}

TEST_F(FileStreamTest, FromFileDescriptor_ReadNonBlocking) {
  int fds[2] = {-1, -1};
  ASSERT_EQ(0, pipe(fds));

  StreamPtr stream = FileStream::FromFileDescriptor(fds[0], true, nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_TRUE(stream->CanRead());
  EXPECT_FALSE(stream->CanWrite());
  EXPECT_FALSE(stream->CanSeek());
  EXPECT_FALSE(stream->CanGetSize());

  char buf[10];
  size_t read = 0;
  bool eos = true;
  EXPECT_TRUE(stream->ReadNonBlocking(buf, sizeof(buf), &read, &eos, nullptr));
  EXPECT_EQ(0, read);
  EXPECT_FALSE(eos);

  std::string data{"foo_bar"};
  EXPECT_TRUE(base::WriteFileDescriptor(fds[1], data.data(), data.size()));
  EXPECT_TRUE(stream->ReadNonBlocking(buf, sizeof(buf), &read, &eos, nullptr));
  EXPECT_EQ(data.size(), read);
  EXPECT_FALSE(eos);
  EXPECT_EQ(data, (std::string{buf, read}));

  EXPECT_TRUE(stream->ReadNonBlocking(buf, sizeof(buf), &read, &eos, nullptr));
  EXPECT_EQ(0, read);
  EXPECT_FALSE(eos);

  close(fds[1]);

  EXPECT_TRUE(stream->ReadNonBlocking(buf, sizeof(buf), &read, &eos, nullptr));
  EXPECT_EQ(0, read);
  EXPECT_TRUE(eos);

  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, FromFileDescriptor_WriteNonBlocking) {
  int fds[2] = {-1, -1};
  ASSERT_EQ(0, pipe(fds));

  StreamPtr stream = FileStream::FromFileDescriptor(fds[1], true, nullptr);
  ASSERT_NE(nullptr, stream.get());
  EXPECT_TRUE(stream->IsOpen());
  EXPECT_FALSE(stream->CanRead());
  EXPECT_TRUE(stream->CanWrite());
  EXPECT_FALSE(stream->CanSeek());
  EXPECT_FALSE(stream->CanGetSize());

  // Pipe buffer is generally 64K, so 128K should be more than enough.
  std::vector<char> buffer(128 * 1024);
  base::RandBytes(buffer.data(), buffer.size());
  size_t written = 0;
  size_t total_size = 0;

  // Fill the output buffer of the pipe until we can no longer write any data
  // to it.
  do {
    ASSERT_TRUE(stream->WriteNonBlocking(buffer.data(), buffer.size(), &written,
                                         nullptr));
    total_size += written;
  } while (written == buffer.size());

  EXPECT_TRUE(stream->WriteNonBlocking(buffer.data(), buffer.size(), &written,
                                       nullptr));
  EXPECT_EQ(0, written);

  std::vector<char> out_buffer(total_size);
  EXPECT_TRUE(base::ReadFromFD(fds[0], out_buffer.data(), out_buffer.size()));

  EXPECT_TRUE(stream->WriteNonBlocking(buffer.data(), buffer.size(), &written,
                                       nullptr));
  EXPECT_GT(written, 0);
  out_buffer.resize(written);
  EXPECT_TRUE(base::ReadFromFD(fds[0], out_buffer.data(), out_buffer.size()));
  EXPECT_TRUE(std::equal(out_buffer.begin(), out_buffer.end(), buffer.begin()));

  close(fds[0]);
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, FromFileDescriptor_ReadAsync) {
  int fds[2] = {-1, -1};
  bool succeeded = false;
  bool failed = false;
  char buffer[100];
  base::MessageLoopForIO base_loop;
  BaseMessageLoop brillo_loop{&base_loop};
  brillo_loop.SetAsCurrent();

  auto success_callback = [&succeeded, &buffer](size_t size) {
    std::string data{buffer, buffer + size};
    ASSERT_EQ("abracadabra", data);
    succeeded = true;
  };

  auto error_callback = [&failed](const Error* /* error */) {
    failed = true;
  };

  auto write_data_callback = [](int write_fd) {
    std::string data{"abracadabra"};
    EXPECT_TRUE(base::WriteFileDescriptor(write_fd, data.data(), data.size()));
  };

  ASSERT_EQ(0, pipe(fds));

  StreamPtr stream = FileStream::FromFileDescriptor(fds[0], true, nullptr);

  // Write to the pipe with a bit of delay.
  brillo_loop.PostDelayedTask(
      FROM_HERE,
      base::Bind(write_data_callback, fds[1]),
      base::TimeDelta::FromMilliseconds(10));

  EXPECT_TRUE(stream->ReadAsync(buffer, 100, base::Bind(success_callback),
                                base::Bind(error_callback), nullptr));

  auto end_condition = [&failed, &succeeded] { return failed || succeeded; };
  MessageLoopRunUntil(&brillo_loop,
                      base::TimeDelta::FromSeconds(1),
                      base::Bind(end_condition));

  EXPECT_TRUE(succeeded);
  EXPECT_FALSE(failed);

  close(fds[1]);
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

TEST_F(FileStreamTest, FromFileDescriptor_WriteAsync) {
  int fds[2] = {-1, -1};
  bool succeeded = false;
  bool failed = false;
  const std::string data{"abracadabra"};
  base::MessageLoopForIO base_loop;
  BaseMessageLoop brillo_loop{&base_loop};
  brillo_loop.SetAsCurrent();

  ASSERT_EQ(0, pipe(fds));

  auto success_callback = [&succeeded, &data](int read_fd, size_t /* size */) {
    char buffer[100];
    EXPECT_TRUE(base::ReadFromFD(read_fd, buffer, data.size()));
    EXPECT_EQ(data, (std::string{buffer, buffer + data.size()}));
    succeeded = true;
  };

  auto error_callback = [&failed](const Error* /* error */) {
    failed = true;
  };

  StreamPtr stream = FileStream::FromFileDescriptor(fds[1], true, nullptr);

  EXPECT_TRUE(stream->WriteAsync(data.data(), data.size(),
                                 base::Bind(success_callback, fds[0]),
                                 base::Bind(error_callback), nullptr));

  auto end_condition = [&failed, &succeeded] { return failed || succeeded; };
  MessageLoopRunUntil(&brillo_loop,
                      base::TimeDelta::FromSeconds(1),
                      base::Bind(end_condition));

  EXPECT_TRUE(succeeded);
  EXPECT_FALSE(failed);

  close(fds[0]);
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
}

}  // namespace brillo
