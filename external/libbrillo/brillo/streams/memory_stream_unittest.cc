// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/memory_stream.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <brillo/streams/stream_errors.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::_;

namespace brillo {

namespace {

int ReadByte(Stream* stream, brillo::ErrorPtr* error) {
  uint8_t byte = 0;
  return stream->ReadAllBlocking(&byte, sizeof(byte), error) ? byte : -1;
}

class MockMemoryContainer : public data_container::DataContainerInterface {
 public:
  MockMemoryContainer() = default;

  MOCK_METHOD5(Read, bool(void*, size_t, size_t, size_t*, ErrorPtr*));
  MOCK_METHOD5(Write, bool(const void*, size_t, size_t, size_t*, ErrorPtr*));
  MOCK_METHOD2(Resize, bool(size_t, ErrorPtr*));
  MOCK_CONST_METHOD0(GetSize, size_t());
  MOCK_CONST_METHOD0(IsReadOnly, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockMemoryContainer);
};

}  // anonymous namespace

class MemoryStreamTest : public testing::Test {
 public:
  void SetUp() override {
    std::unique_ptr<MockMemoryContainer> container{new MockMemoryContainer{}};
    stream_.reset(new MemoryStream{std::move(container), 0});
  }

  MockMemoryContainer& container_mock() {
    return *static_cast<MockMemoryContainer*>(stream_->container_.get());
  }

  inline static void* IntToPtr(int addr) {
    return reinterpret_cast<void*>(addr);
  }

  inline static const void* IntToConstPtr(int addr) {
    return reinterpret_cast<const void*>(addr);
  }

  std::unique_ptr<MemoryStream> stream_;
  // Dummy buffer pointer values to make sure that input pointer values
  // are delegated to the stream interface without a change.
  void* const test_read_buffer_ = IntToPtr(12345);
  const void* const test_write_buffer_ = IntToConstPtr(67890);
  // We limit the size of memory streams to be the maximum size of either of
  // size_t (on 32 bit platforms) or the size of signed 64 bit integer.
  const size_t kSizeMax =
      std::min<uint64_t>(std::numeric_limits<size_t>::max(),
                         std::numeric_limits<int64_t>::max());
};

TEST_F(MemoryStreamTest, CanRead) {
  EXPECT_TRUE(stream_->CanRead());
}

TEST_F(MemoryStreamTest, CanWrite) {
  EXPECT_CALL(container_mock(), IsReadOnly())
    .WillOnce(Return(true))
    .WillOnce(Return(false));

  EXPECT_FALSE(stream_->CanWrite());
  EXPECT_TRUE(stream_->CanWrite());
}

TEST_F(MemoryStreamTest, CanSeek) {
  EXPECT_TRUE(stream_->CanSeek());
}

TEST_F(MemoryStreamTest, GetSize) {
  EXPECT_CALL(container_mock(), GetSize())
    .WillOnce(Return(0))
    .WillOnce(Return(1234))
    .WillOnce(Return(kSizeMax));

  EXPECT_EQ(0, stream_->GetSize());
  EXPECT_EQ(1234, stream_->GetSize());
  EXPECT_EQ(kSizeMax, stream_->GetSize());
}

TEST_F(MemoryStreamTest, SetSizeBlocking) {
  EXPECT_CALL(container_mock(), Resize(0, _)).WillOnce(Return(true));

  ErrorPtr error;
  EXPECT_TRUE(stream_->SetSizeBlocking(0, &error));
  EXPECT_EQ(nullptr, error.get());

  EXPECT_CALL(container_mock(), Resize(kSizeMax, nullptr))
    .WillOnce(Return(true));

  EXPECT_TRUE(stream_->SetSizeBlocking(kSizeMax, nullptr));
}

TEST_F(MemoryStreamTest, SeekAndGetPosition) {
  EXPECT_EQ(0, stream_->GetPosition());

  EXPECT_CALL(container_mock(), GetSize()).WillRepeatedly(Return(200));

  ErrorPtr error;
  uint64_t new_pos = 0;
  EXPECT_TRUE(stream_->Seek(2, Stream::Whence::FROM_BEGIN, &new_pos, &error));
  EXPECT_EQ(nullptr, error.get());
  EXPECT_EQ(2, new_pos);
  EXPECT_EQ(2, stream_->GetPosition());
  EXPECT_TRUE(stream_->Seek(2, Stream::Whence::FROM_CURRENT, &new_pos, &error));
  EXPECT_EQ(nullptr, error.get());
  EXPECT_EQ(4, new_pos);
  EXPECT_EQ(4, stream_->GetPosition());

  EXPECT_TRUE(stream_->Seek(-2, Stream::Whence::FROM_END, nullptr, nullptr));
  EXPECT_EQ(198, stream_->GetPosition());

  EXPECT_CALL(container_mock(), GetSize()).WillOnce(Return(kSizeMax));
  EXPECT_TRUE(stream_->Seek(0, Stream::Whence::FROM_END, nullptr, nullptr));
  EXPECT_EQ(kSizeMax, stream_->GetPosition());
}

TEST_F(MemoryStreamTest, ReadNonBlocking) {
  size_t read = 0;
  bool eos = false;

  EXPECT_CALL(container_mock(), Read(test_read_buffer_, 10, 0, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(5), Return(true)));

  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 10, &read, &eos,
                                       nullptr));
  EXPECT_EQ(5, read);
  EXPECT_EQ(5, stream_->GetPosition());
  EXPECT_FALSE(eos);

  EXPECT_CALL(container_mock(), Read(test_read_buffer_, 100, 5, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(100), Return(true)));

  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 100, &read, &eos,
                                       nullptr));
  EXPECT_EQ(100, read);
  EXPECT_EQ(105, stream_->GetPosition());
  EXPECT_FALSE(eos);

  EXPECT_CALL(container_mock(), Read(test_read_buffer_, 10, 105, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(0), Return(true)));

  EXPECT_TRUE(stream_->ReadNonBlocking(test_read_buffer_, 10, &read, &eos,
                                       nullptr));
  EXPECT_EQ(0, read);
  EXPECT_EQ(105, stream_->GetPosition());
  EXPECT_TRUE(eos);
}

TEST_F(MemoryStreamTest, WriteNonBlocking) {
  size_t written = 0;

  EXPECT_CALL(container_mock(), Write(test_write_buffer_, 10, 0, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(5), Return(true)));

  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 10, &written,
                                        nullptr));
  EXPECT_EQ(5, written);
  EXPECT_EQ(5, stream_->GetPosition());

  EXPECT_CALL(container_mock(), Write(test_write_buffer_, 100, 5, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(100), Return(true)));

  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 100, &written,
                                        nullptr));
  EXPECT_EQ(100, written);
  EXPECT_EQ(105, stream_->GetPosition());

  EXPECT_CALL(container_mock(), Write(test_write_buffer_, 10, 105, _, nullptr))
    .WillOnce(DoAll(SetArgPointee<3>(10), Return(true)));

  EXPECT_TRUE(stream_->WriteNonBlocking(test_write_buffer_, 10, &written,
                                        nullptr));
  EXPECT_EQ(115, stream_->GetPosition());
}

//////////////////////////////////////////////////////////////////////////////
// Factory method tests.
TEST(MemoryStream, OpenBinary) {
  char buffer[] = {1, 2, 3};
  StreamPtr stream = MemoryStream::OpenRef(buffer, sizeof(buffer), nullptr);
  buffer[0] = 5;
  EXPECT_EQ(3, stream->GetSize());
  EXPECT_EQ(5, ReadByte(stream.get(), nullptr));
  EXPECT_EQ(2, ReadByte(stream.get(), nullptr));
  EXPECT_EQ(3, ReadByte(stream.get(), nullptr));
  brillo::ErrorPtr error;
  EXPECT_EQ(-1, ReadByte(stream.get(), &error));
  EXPECT_EQ(errors::stream::kPartialData, error->GetCode());
  EXPECT_EQ("Reading past the end of stream", error->GetMessage());
}

TEST(MemoryStream, OpenBinaryCopy) {
  char buffer[] = {1, 2, 3};
  StreamPtr stream = MemoryStream::OpenCopyOf(buffer, sizeof(buffer), nullptr);
  buffer[0] = 5;
  EXPECT_EQ(3, stream->GetSize());
  EXPECT_EQ(1, ReadByte(stream.get(), nullptr));
  EXPECT_EQ(2, ReadByte(stream.get(), nullptr));
  EXPECT_EQ(3, ReadByte(stream.get(), nullptr));
  brillo::ErrorPtr error;
  EXPECT_EQ(-1, ReadByte(stream.get(), &error));
  EXPECT_EQ(errors::stream::kPartialData, error->GetCode());
  EXPECT_EQ("Reading past the end of stream", error->GetMessage());
}

TEST(MemoryStream, OpenString) {
  std::string str("abcd");
  StreamPtr stream = MemoryStream::OpenRef(str, nullptr);
  str[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ('A', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(-1, ReadByte(stream.get(), nullptr));
}

TEST(MemoryStream, OpenStringCopy) {
  std::string str("abcd");
  StreamPtr stream = MemoryStream::OpenCopyOf(str, nullptr);
  str[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ('a', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(-1, ReadByte(stream.get(), nullptr));
}

TEST(MemoryStream, OpenCharBuf) {
  char str[] = "abcd";
  StreamPtr stream = MemoryStream::OpenRef(str, nullptr);
  str[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ('A', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(-1, ReadByte(stream.get(), nullptr));
}

TEST(MemoryStream, OpenCharBufCopy) {
  char str[] = "abcd";
  StreamPtr stream = MemoryStream::OpenCopyOf(str, nullptr);
  str[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ('a', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(-1, ReadByte(stream.get(), nullptr));
}

TEST(MemoryStream, OpenVector) {
  std::vector<char> data = {'a', 'b', 'c', 'd'};
  StreamPtr stream = MemoryStream::OpenRef(data, nullptr);
  data[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(4, stream->GetRemainingSize());
  EXPECT_EQ('A', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(4, stream->GetPosition());
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(0, stream->GetRemainingSize());
}

TEST(MemoryStream, OpenVectorCopy) {
  std::vector<uint8_t> data = {'a', 'b', 'c', 'd'};
  StreamPtr stream = MemoryStream::OpenCopyOf(data, nullptr);
  data[0] = 'A';
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(4, stream->GetRemainingSize());
  EXPECT_EQ('a', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('b', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('c', ReadByte(stream.get(), nullptr));
  EXPECT_EQ('d', ReadByte(stream.get(), nullptr));
  EXPECT_EQ(4, stream->GetPosition());
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(0, stream->GetRemainingSize());
}

TEST(MemoryStream, CreateVector) {
  std::vector<uint8_t> buffer;
  StreamPtr stream = MemoryStream::CreateRef(&buffer, nullptr);
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(0, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  buffer.resize(5);
  std::iota(buffer.begin(), buffer.end(), 0);
  stream = MemoryStream::CreateRef(&buffer, nullptr);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(5, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  stream = MemoryStream::CreateRefForAppend(&buffer, nullptr);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(5, stream->GetPosition());
  EXPECT_EQ(5, stream->GetSize());
  EXPECT_TRUE(stream->WriteAllBlocking("abcde", 5, nullptr));
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(10, stream->GetPosition());
  EXPECT_EQ(10, stream->GetSize());
  EXPECT_TRUE(stream->SetPosition(0, nullptr));
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(10, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  EXPECT_EQ(10, buffer.size());
  EXPECT_EQ((std::vector<uint8_t>{0, 1, 2, 3, 4, 'a', 'b', 'c', 'd', 'e'}),
            buffer);

  stream = MemoryStream::OpenRef(buffer, nullptr);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(10, stream->GetSize());
}

TEST(MemoryStream, CreateString) {
  std::string buffer;
  StreamPtr stream = MemoryStream::CreateRef(&buffer, nullptr);
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(0, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  buffer = "abc";
  stream = MemoryStream::CreateRef(&buffer, nullptr);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(3, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));

  stream = MemoryStream::CreateRefForAppend(&buffer, nullptr);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(3, stream->GetPosition());
  EXPECT_EQ(3, stream->GetSize());
  EXPECT_TRUE(stream->WriteAllBlocking("d_1234", 6, nullptr));
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(9, stream->GetPosition());
  EXPECT_EQ(9, stream->GetSize());
  EXPECT_TRUE(stream->SetPosition(0, nullptr));
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_EQ(9, stream->GetSize());
  EXPECT_TRUE(stream->CloseBlocking(nullptr));
  EXPECT_EQ(9, buffer.size());
  EXPECT_EQ("abcd_1234", buffer);
}

}  // namespace brillo
