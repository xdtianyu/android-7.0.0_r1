// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/input_stream_set.h>

#include <brillo/errors/error_codes.h>
#include <brillo/streams/mock_stream.h>
#include <brillo/streams/stream_errors.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::An;
using testing::DoAll;
using testing::InSequence;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::_;

namespace brillo {

class InputStreamSetTest : public testing::Test {
 public:
  void SetUp() override {
    itf1_.reset(new StrictMock<MockStream>{});
    itf2_.reset(new StrictMock<MockStream>{});
    stream_.reset(new InputStreamSet({itf1_.get(), itf2_.get()}, {}, 100));
  }

  void TearDown() override {
    stream_.reset();
    itf2_.reset();
    itf1_.reset();
  }

  std::unique_ptr<StrictMock<MockStream>> itf1_;
  std::unique_ptr<StrictMock<MockStream>> itf2_;
  std::unique_ptr<InputStreamSet> stream_;

  inline static void* IntToPtr(int addr) {
    return reinterpret_cast<void*>(addr);
  }
};

TEST_F(InputStreamSetTest, InitialFalseAssumptions) {
  // Methods that should just succeed/fail without calling underlying streams.
  EXPECT_TRUE(stream_->CanRead());
  EXPECT_FALSE(stream_->CanWrite());
  EXPECT_FALSE(stream_->CanSeek());
  EXPECT_EQ(100, stream_->GetSize());
  EXPECT_FALSE(stream_->SetSizeBlocking(0, nullptr));
  EXPECT_FALSE(stream_->GetPosition());
  EXPECT_FALSE(stream_->Seek(0, Stream::Whence::FROM_BEGIN, nullptr, nullptr));
  char buffer[100];
  size_t size = 0;
  EXPECT_FALSE(stream_->WriteAsync(buffer, sizeof(buffer), {}, {}, nullptr));
  EXPECT_FALSE(stream_->WriteAllAsync(buffer, sizeof(buffer), {}, {}, nullptr));
  EXPECT_FALSE(stream_->WriteNonBlocking(buffer, sizeof(buffer), &size,
                                         nullptr));
  EXPECT_FALSE(stream_->WriteBlocking(buffer, sizeof(buffer), &size, nullptr));
  EXPECT_FALSE(stream_->WriteAllBlocking(buffer, sizeof(buffer), nullptr));
  EXPECT_TRUE(stream_->FlushBlocking(nullptr));
  EXPECT_TRUE(stream_->CloseBlocking(nullptr));
}

TEST_F(InputStreamSetTest, InitialTrueAssumptions) {
  // Methods that redirect calls to underlying streams.
  EXPECT_CALL(*itf1_, CanGetSize()).WillOnce(Return(true));
  EXPECT_CALL(*itf2_, CanGetSize()).WillOnce(Return(true));
  EXPECT_TRUE(stream_->CanGetSize());

  // Reading from the first stream fails, so the second one shouldn't be used.
  EXPECT_CALL(*itf1_, ReadNonBlocking(_, _, _, _, _))
      .WillOnce(Return(false));
  EXPECT_CALL(*itf2_, ReadNonBlocking(_, _, _, _, _)).Times(0);
  char buffer[100];
  size_t size = 0;
  EXPECT_FALSE(stream_->ReadBlocking(buffer, sizeof(buffer), &size, nullptr));
}

TEST_F(InputStreamSetTest, CanGetSize) {
  EXPECT_CALL(*itf1_, CanGetSize()).WillOnce(Return(true));
  EXPECT_CALL(*itf2_, CanGetSize()).WillOnce(Return(true));
  EXPECT_TRUE(stream_->CanGetSize());

  EXPECT_CALL(*itf1_, CanGetSize()).WillOnce(Return(false));
  EXPECT_FALSE(stream_->CanGetSize());

  EXPECT_CALL(*itf1_, CanGetSize()).WillOnce(Return(true));
  EXPECT_CALL(*itf2_, CanGetSize()).WillOnce(Return(false));
  EXPECT_FALSE(stream_->CanGetSize());
}

TEST_F(InputStreamSetTest, GetRemainingSize) {
  EXPECT_CALL(*itf1_, GetRemainingSize()).WillOnce(Return(10));
  EXPECT_CALL(*itf2_, GetRemainingSize()).WillOnce(Return(32));
  EXPECT_EQ(42, stream_->GetRemainingSize());
}

TEST_F(InputStreamSetTest, ReadNonBlocking) {
  size_t read = 0;
  bool eos = false;

  InSequence s;
  EXPECT_CALL(*itf1_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
    .WillOnce(DoAll(SetArgPointee<2>(10),
                    SetArgPointee<3>(false),
                    Return(true)));
  EXPECT_TRUE(stream_->ReadNonBlocking(IntToPtr(1000), 100, &read, &eos,
                                       nullptr));
  EXPECT_EQ(10, read);
  EXPECT_FALSE(eos);

  EXPECT_CALL(*itf1_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
    .WillOnce(DoAll(SetArgPointee<2>(0), SetArgPointee<3>(true), Return(true)));
  EXPECT_CALL(*itf2_, ReadNonBlocking(IntToPtr(1000), 100 , _, _, _))
    .WillOnce(DoAll(SetArgPointee<2>(100),
                    SetArgPointee<3>(false),
                    Return(true)));
  EXPECT_TRUE(stream_->ReadNonBlocking(IntToPtr(1000), 100, &read, &eos,
                                       nullptr));
  EXPECT_EQ(100, read);
  EXPECT_FALSE(eos);

  EXPECT_CALL(*itf2_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
    .WillOnce(DoAll(SetArgPointee<2>(0), SetArgPointee<3>(true), Return(true)));
  EXPECT_TRUE(stream_->ReadNonBlocking(IntToPtr(1000), 100, &read, &eos,
                                       nullptr));
  EXPECT_EQ(0, read);
  EXPECT_TRUE(eos);
}

TEST_F(InputStreamSetTest, ReadBlocking) {
  size_t read = 0;

  InSequence s;
  EXPECT_CALL(*itf1_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(10),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_TRUE(stream_->ReadBlocking(IntToPtr(1000), 100, &read, nullptr));
  EXPECT_EQ(10, read);

  EXPECT_CALL(*itf1_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(true),
                      Return(true)));
  EXPECT_CALL(*itf2_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_CALL(*itf2_, WaitForDataBlocking(Stream::AccessMode::READ, _, _, _))
      .WillOnce(Return(true));
  EXPECT_CALL(*itf2_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(100),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_TRUE(stream_->ReadBlocking(IntToPtr(1000), 100, &read, nullptr));
  EXPECT_EQ(100, read);

  EXPECT_CALL(*itf2_, ReadNonBlocking(IntToPtr(1000), 100, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(true),
                      Return(true)));
  EXPECT_TRUE(stream_->ReadBlocking(IntToPtr(1000), 100, &read, nullptr));
  EXPECT_EQ(0, read);
}

}  // namespace brillo
