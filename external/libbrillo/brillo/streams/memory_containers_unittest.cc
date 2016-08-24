// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/memory_containers.h>

#include <limits>
#include <memory>

#include <brillo/streams/mock_stream.h>
#include <brillo/streams/stream_errors.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::Invoke;
using testing::InSequence;
using testing::Return;
using testing::WithArgs;
using testing::_;

namespace brillo {

namespace {
class MockContiguousBuffer : public data_container::ContiguousBufferBase {
 public:
  MockContiguousBuffer() = default;

  MOCK_METHOD2(Resize, bool(size_t, ErrorPtr*));
  MOCK_CONST_METHOD0(GetSize, size_t());
  MOCK_CONST_METHOD0(IsReadOnly, bool());

  MOCK_CONST_METHOD2(GetReadOnlyBuffer, const void*(size_t, ErrorPtr*));
  MOCK_METHOD2(GetBuffer, void*(size_t, ErrorPtr*));

  MOCK_CONST_METHOD3(CopyMemoryBlock, void(void*, const void*, size_t));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockContiguousBuffer);
};
}  // anonymous namespace

class MemoryContainerTest : public testing::Test {
 public:
  inline static void* IntToPtr(int addr) {
    return reinterpret_cast<void*>(addr);
  }

  inline static const void* IntToConstPtr(int addr) {
    return reinterpret_cast<const void*>(addr);
  }

  // Dummy buffer pointer values used as external data source/destination for
  // read/write operations.
  void* const test_read_buffer_ = IntToPtr(12345);
  const void* const test_write_buffer_ = IntToConstPtr(67890);

  // Dummy buffer pointer values used for internal buffer owned by the
  // memory buffer container class.
  const void* const const_buffer_ = IntToConstPtr(123);
  void* const buffer_ = IntToPtr(456);

  MockContiguousBuffer container_;
};

TEST_F(MemoryContainerTest, Read_WithinBuffer) {
  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, GetReadOnlyBuffer(10, _))
      .WillOnce(Return(const_buffer_));
    EXPECT_CALL(container_,
                CopyMemoryBlock(test_read_buffer_, const_buffer_, 50)).Times(1);
  }
  size_t read = 0;
  ErrorPtr error;
  EXPECT_TRUE(container_.Read(test_read_buffer_, 50, 10, &read, &error));
  EXPECT_EQ(50, read);
  EXPECT_EQ(nullptr, error.get());
}

TEST_F(MemoryContainerTest, Read_PastEndOfBuffer) {
  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, GetReadOnlyBuffer(80, _))
      .WillOnce(Return(const_buffer_));
    EXPECT_CALL(container_,
                CopyMemoryBlock(test_read_buffer_, const_buffer_, 20)).Times(1);
  }
  size_t read = 0;
  EXPECT_TRUE(container_.Read(test_read_buffer_, 50, 80, &read, nullptr));
  EXPECT_EQ(20, read);
}

TEST_F(MemoryContainerTest, Read_OutsideBuffer) {
  EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
  size_t read = 0;
  EXPECT_TRUE(container_.Read(test_read_buffer_, 50, 100, &read, nullptr));
  EXPECT_EQ(0, read);
}

TEST_F(MemoryContainerTest, Read_Error) {
  auto OnReadError = [](ErrorPtr* error) {
    Error::AddTo(error, FROM_HERE, "domain", "read_error", "read error");
  };

  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, GetReadOnlyBuffer(0, _))
      .WillOnce(DoAll(WithArgs<1>(Invoke(OnReadError)), Return(nullptr)));
  }
  size_t read = 0;
  ErrorPtr error;
  EXPECT_FALSE(container_.Read(test_read_buffer_, 10, 0, &read, &error));
  EXPECT_EQ(0, read);
  EXPECT_NE(nullptr, error.get());
  EXPECT_EQ("domain", error->GetDomain());
  EXPECT_EQ("read_error", error->GetCode());
  EXPECT_EQ("read error", error->GetMessage());
}

TEST_F(MemoryContainerTest, Write_WithinBuffer) {
  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, GetBuffer(10, _))
      .WillOnce(Return(buffer_));
    EXPECT_CALL(container_,
                CopyMemoryBlock(buffer_, test_write_buffer_, 50)).Times(1);
  }
  size_t written = 0;
  ErrorPtr error;
  EXPECT_TRUE(container_.Write(test_write_buffer_, 50, 10, &written, &error));
  EXPECT_EQ(50, written);
  EXPECT_EQ(nullptr, error.get());
}

TEST_F(MemoryContainerTest, Write_PastEndOfBuffer) {
  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, Resize(130, _)).WillOnce(Return(true));
    EXPECT_CALL(container_, GetBuffer(80, _))
      .WillOnce(Return(buffer_));
    EXPECT_CALL(container_,
                CopyMemoryBlock(buffer_, test_write_buffer_, 50)).Times(1);
  }
  size_t written = 0;
  EXPECT_TRUE(container_.Write(test_write_buffer_, 50, 80, &written, nullptr));
  EXPECT_EQ(50, written);
}

TEST_F(MemoryContainerTest, Write_OutsideBuffer) {
  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, Resize(160, _)).WillOnce(Return(true));
    EXPECT_CALL(container_, GetBuffer(110, _))
      .WillOnce(Return(buffer_));
    EXPECT_CALL(container_,
                CopyMemoryBlock(buffer_, test_write_buffer_, 50)).Times(1);
  }
  size_t written = 0;
  EXPECT_TRUE(container_.Write(test_write_buffer_, 50, 110, &written, nullptr));
  EXPECT_EQ(50, written);
}

TEST_F(MemoryContainerTest, Write_Error_Resize) {
  auto OnWriteError = [](ErrorPtr* error) {
    Error::AddTo(error, FROM_HERE, "domain", "write_error", "resize error");
  };

  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, Resize(160, _))
      .WillOnce(DoAll(WithArgs<1>(Invoke(OnWriteError)), Return(false)));
  }
  size_t written = 0;
  ErrorPtr error;
  EXPECT_FALSE(container_.Write(test_write_buffer_, 50, 110, &written, &error));
  EXPECT_EQ(0, written);
  EXPECT_NE(nullptr, error.get());
  EXPECT_EQ("domain", error->GetDomain());
  EXPECT_EQ("write_error", error->GetCode());
  EXPECT_EQ("resize error", error->GetMessage());
}

TEST_F(MemoryContainerTest, Write_Error) {
  auto OnWriteError = [](ErrorPtr* error) {
    Error::AddTo(error, FROM_HERE, "domain", "write_error", "write error");
  };

  {
    InSequence s;
    EXPECT_CALL(container_, GetSize()).WillOnce(Return(100));
    EXPECT_CALL(container_, Resize(160, _)).WillOnce(Return(true));
    EXPECT_CALL(container_, GetBuffer(110, _))
      .WillOnce(DoAll(WithArgs<1>(Invoke(OnWriteError)), Return(nullptr)));
  }
  size_t written = 0;
  ErrorPtr error;
  EXPECT_FALSE(container_.Write(test_write_buffer_, 50, 110, &written, &error));
  EXPECT_EQ(0, written);
  EXPECT_NE(nullptr, error.get());
  EXPECT_EQ("domain", error->GetDomain());
  EXPECT_EQ("write_error", error->GetCode());
  EXPECT_EQ("write error", error->GetMessage());
}

}  // namespace brillo

