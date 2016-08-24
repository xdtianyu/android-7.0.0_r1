// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/openssl_stream_bio.h>

#include <memory>
#include <openssl/bio.h>

#include <brillo/streams/mock_stream.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::_;

namespace brillo {

class StreamBIOTest : public testing::Test {
 public:
  void SetUp() override {
    stream_.reset(new StrictMock<MockStream>{});
    bio_ = BIO_new_stream(stream_.get());
  }

  void TearDown() override {
    BIO_free(bio_);
    bio_ = nullptr;
    stream_.reset();
  }

  std::unique_ptr<StrictMock<MockStream>> stream_;
  BIO* bio_{nullptr};
};

TEST_F(StreamBIOTest, ReadFull) {
  char buffer[10];
  EXPECT_CALL(*stream_, ReadNonBlocking(buffer, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(10),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_EQ(10, BIO_read(bio_, buffer, sizeof(buffer)));
}

TEST_F(StreamBIOTest, ReadPartial) {
  char buffer[10];
  EXPECT_CALL(*stream_, ReadNonBlocking(buffer, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(3),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_EQ(3, BIO_read(bio_, buffer, sizeof(buffer)));
}

TEST_F(StreamBIOTest, ReadWouldBlock) {
  char buffer[10];
  EXPECT_CALL(*stream_, ReadNonBlocking(buffer, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(false),
                      Return(true)));
  EXPECT_EQ(-1, BIO_read(bio_, buffer, sizeof(buffer)));
  EXPECT_TRUE(BIO_should_retry(bio_));
}

TEST_F(StreamBIOTest, ReadEndOfStream) {
  char buffer[10];
  EXPECT_CALL(*stream_, ReadNonBlocking(buffer, 10, _, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0),
                      SetArgPointee<3>(true),
                      Return(true)));
  EXPECT_EQ(0, BIO_read(bio_, buffer, sizeof(buffer)));
  EXPECT_FALSE(BIO_should_retry(bio_));
}

TEST_F(StreamBIOTest, ReadError) {
  char buffer[10];
  EXPECT_CALL(*stream_, ReadNonBlocking(buffer, 10, _, _, _))
      .WillOnce(Return(false));
  EXPECT_EQ(-1, BIO_read(bio_, buffer, sizeof(buffer)));
  EXPECT_FALSE(BIO_should_retry(bio_));
}

TEST_F(StreamBIOTest, WriteFull) {
  char buffer[10] = {};
  EXPECT_CALL(*stream_, WriteNonBlocking(buffer, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(10), Return(true)));
  EXPECT_EQ(10, BIO_write(bio_, buffer, sizeof(buffer)));
}

TEST_F(StreamBIOTest, WritePartial) {
  char buffer[10] = {};
  EXPECT_CALL(*stream_, WriteNonBlocking(buffer, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(3), Return(true)));
  EXPECT_EQ(3, BIO_write(bio_, buffer, sizeof(buffer)));
}

TEST_F(StreamBIOTest, WriteWouldBlock) {
  char buffer[10] = {};
  EXPECT_CALL(*stream_, WriteNonBlocking(buffer, 10, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(0), Return(true)));
  EXPECT_EQ(-1, BIO_write(bio_, buffer, sizeof(buffer)));
  EXPECT_TRUE(BIO_should_retry(bio_));
}

TEST_F(StreamBIOTest, WriteError) {
  char buffer[10] = {};
  EXPECT_CALL(*stream_, WriteNonBlocking(buffer, 10, _, _))
      .WillOnce(Return(false));
  EXPECT_EQ(-1, BIO_write(bio_, buffer, sizeof(buffer)));
  EXPECT_FALSE(BIO_should_retry(bio_));
}

TEST_F(StreamBIOTest, FlushSuccess) {
  EXPECT_CALL(*stream_, FlushBlocking(_)).WillOnce(Return(true));
  EXPECT_EQ(1, BIO_flush(bio_));
}

TEST_F(StreamBIOTest, FlushError) {
  EXPECT_CALL(*stream_, FlushBlocking(_)).WillOnce(Return(false));
  EXPECT_EQ(0, BIO_flush(bio_));
}

}  // namespace brillo
