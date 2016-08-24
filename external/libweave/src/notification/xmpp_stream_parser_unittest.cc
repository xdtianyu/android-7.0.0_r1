// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xmpp_stream_parser.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "src/notification/xml_node.h"

namespace weave {
namespace {
// Use some real-world XMPP stream snippet to make sure all the expected
// elements are parsed properly.
const char kXmppStreamData[] =
    "<stream:stream from=\"clouddevices.gserviceaccount.com\" id=\"76EEB8FDB449"
    "5558\" version=\"1.0\" xmlns:stream=\"http://etherx.jabber.org/streams\" x"
    "mlns=\"jabber:client\">"
    "<stream:features><starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"><requ"
    "ired/></starttls><mechanisms xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"><m"
    "echanism>X-OAUTH2</mechanism><mechanism>X-GOOGLE-TOKEN</mechanism></mechan"
    "isms></stream:features>"
    "<message from=\"cloud-devices@clouddevices.google.com/srvenc-xgbCfg9hX6tCp"
    "xoMYsExqg==\" to=\"4783f652b387449fc52a76f9a16e616f@clouddevices.gservicea"
    "ccount.com/5A85ED9C\"><push:push channel=\"cloud_devices\" xmlns:push=\"go"
    "ogle:push\"><push:recipient to=\"4783f652b387449fc52a76f9a16e616f@clouddev"
    "ices.gserviceaccount.com\"></push:recipient><push:data>eyJraW5kIjoiY2xvdWR"
    "kZXZpY2VzI25vdGlmaWNhdGlvbiIsInR5cGUiOiJDT01NQU5EX0NSRUFURUQiLCJjb21tYW5kS"
    "WQiOiIwNWE3MTA5MC1hZWE4LWMzNzQtOTYwNS0xZTRhY2JhNDRmM2Y4OTAzZmM3Yy01NjExLWI"
    "5ODAtOTkyMy0yNjc2YjYwYzkxMGMiLCJkZXZpY2VJZCI6IjA1YTcxMDkwLWFlYTgtYzM3NC05N"
    "jA1LTFlNGFjYmE0NGYzZiIsImNvbW1hbmQiOnsia2luZCI6ImNsb3VkZGV2aWNlcyNjb21tYW5"
    "kIiwiaWQiOiIwNWE3MTA5MC1hZWE4LWMzNzQtOTYwNS0xZTRhY2JhNDRmM2Y4OTAzZmM3Yy01N"
    "jExLWI5ODAtOTkyMy0yNjc2YjYwYzkxMGMiLCJkZXZpY2VJZCI6IjA1YTcxMDkwLWFlYTgtYzM"
    "3NC05NjA1LTFlNGFjYmE0NGYzZiIsIm5hbWUiOiJiYXNlLl9qdW1wIiwic3RhdGUiOiJxdWV1Z"
    "WQiLCJlcnJvciI6eyJhcmd1bWVudHMiOltdfSwiY3JlYXRpb25UaW1lTXMiOiIxNDMxNTY0NDY"
    "4MjI3IiwiZXhwaXJhdGlvblRpbWVNcyI6IjE0MzE1NjgwNjgyMjciLCJleHBpcmF0aW9uVGltZ"
    "W91dE1zIjoiMzYwMDAwMCJ9fQ==</push:data></push:push></message>";

}  // anonymous namespace

class XmppStreamParserTest : public testing::Test,
                             public XmppStreamParser::Delegate {
 public:
  void SetUp() override { parser_.reset(new XmppStreamParser{this}); }

  void OnStreamStart(const std::string& node_name,
                     std::map<std::string, std::string> attributes) override {
    EXPECT_FALSE(stream_started_);
    stream_started_ = true;
    stream_start_node_name_ = node_name;
    stream_start_node_attributes_ = std::move(attributes);
  }

  void OnStreamEnd(const std::string& node_name) override {
    EXPECT_TRUE(stream_started_);
    EXPECT_EQ(stream_start_node_name_, node_name);
    stream_started_ = false;
  }

  void OnStanza(std::unique_ptr<XmlNode> stanza) override {
    stanzas_.push_back(std::move(stanza));
  }

  void Reset() {
    parser_.reset(new XmppStreamParser{this});
    stream_started_ = false;
    stream_start_node_name_.clear();
    stream_start_node_attributes_.clear();
    stanzas_.clear();
  }

  std::unique_ptr<XmppStreamParser> parser_;
  bool stream_started_{false};
  std::string stream_start_node_name_;
  std::map<std::string, std::string> stream_start_node_attributes_;
  std::vector<std::unique_ptr<XmlNode>> stanzas_;
};

TEST_F(XmppStreamParserTest, InitialState) {
  EXPECT_FALSE(stream_started_);
  EXPECT_TRUE(stream_start_node_name_.empty());
  EXPECT_TRUE(stream_start_node_attributes_.empty());
  EXPECT_TRUE(stanzas_.empty());
}

TEST_F(XmppStreamParserTest, FullStartElement) {
  parser_->ParseData("<foo bar=\"baz\" quux=\"1\">");
  EXPECT_TRUE(stream_started_);
  EXPECT_EQ("foo", stream_start_node_name_);
  const std::map<std::string, std::string> expected_attrs{{"bar", "baz"},
                                                          {"quux", "1"}};
  EXPECT_EQ(expected_attrs, stream_start_node_attributes_);
}

TEST_F(XmppStreamParserTest, PartialStartElement) {
  parser_->ParseData("<foo bar=\"baz");
  EXPECT_FALSE(stream_started_);
  EXPECT_TRUE(stream_start_node_name_.empty());
  EXPECT_TRUE(stream_start_node_attributes_.empty());
  EXPECT_TRUE(stanzas_.empty());
  parser_->ParseData("\" quux");
  EXPECT_FALSE(stream_started_);
  parser_->ParseData("=\"1\">");
  EXPECT_TRUE(stream_started_);
  EXPECT_EQ("foo", stream_start_node_name_);
  const std::map<std::string, std::string> expected_attrs{{"bar", "baz"},
                                                          {"quux", "1"}};
  EXPECT_EQ(expected_attrs, stream_start_node_attributes_);
}

TEST_F(XmppStreamParserTest, VariableLengthPackets) {
  std::string value;
  const std::string xml_data = kXmppStreamData;
  const std::map<std::string, std::string> expected_stream_attrs{
      {"from", "clouddevices.gserviceaccount.com"},
      {"id", "76EEB8FDB4495558"},
      {"version", "1.0"},
      {"xmlns:stream", "http://etherx.jabber.org/streams"},
      {"xmlns", "jabber:client"}};
  // Try splitting the data into pieces from 1 character in size to the whole
  // data block and verify that we still can parse the whole message correctly.
  // Here |step| is the size of each individual data chunk.
  for (size_t step = 1; step <= xml_data.size(); step++) {
    // Feed each individual chunk to the parser and hope it can piece everything
    // together correctly.
    for (size_t pos = 0; pos < xml_data.size(); pos += step) {
      parser_->ParseData(xml_data.substr(pos, step));
    }
    EXPECT_TRUE(stream_started_);
    EXPECT_EQ("stream:stream", stream_start_node_name_);
    EXPECT_EQ(expected_stream_attrs, stream_start_node_attributes_);
    EXPECT_EQ(2u, stanzas_.size());

    const XmlNode* stanza1 = stanzas_[0].get();
    EXPECT_EQ("stream:features", stanza1->name());
    ASSERT_EQ(2u, stanza1->children().size());
    const XmlNode* child1 = stanza1->children()[0].get();
    EXPECT_EQ("starttls", child1->name());
    ASSERT_EQ(1u, child1->children().size());
    EXPECT_EQ("required", child1->children()[0]->name());
    const XmlNode* child2 = stanza1->children()[1].get();
    EXPECT_EQ("mechanisms", child2->name());
    ASSERT_EQ(2u, child2->children().size());
    EXPECT_EQ("mechanism", child2->children()[0]->name());
    EXPECT_EQ("X-OAUTH2", child2->children()[0]->text());
    EXPECT_EQ("mechanism", child2->children()[1]->name());
    EXPECT_EQ("X-GOOGLE-TOKEN", child2->children()[1]->text());

    const XmlNode* stanza2 = stanzas_[1].get();
    EXPECT_EQ("message", stanza2->name());
    ASSERT_EQ(2u, stanza2->attributes().size());
    EXPECT_TRUE(stanza2->GetAttribute("from", &value));
    EXPECT_EQ(
        "cloud-devices@clouddevices.google.com/"
        "srvenc-xgbCfg9hX6tCpxoMYsExqg==",
        value);
    EXPECT_TRUE(stanza2->GetAttribute("to", &value));
    EXPECT_EQ(
        "4783f652b387449fc52a76f9a16e616f@clouddevices.gserviceaccount."
        "com/5A85ED9C",
        value);
    ASSERT_EQ(1u, stanza2->children().size());

    const XmlNode* child = stanza2->children().back().get();
    EXPECT_EQ("push:push", child->name());
    ASSERT_EQ(2u, child->attributes().size());
    EXPECT_TRUE(child->GetAttribute("channel", &value));
    EXPECT_EQ("cloud_devices", value);
    EXPECT_TRUE(child->GetAttribute("xmlns:push", &value));
    EXPECT_EQ("google:push", value);
    ASSERT_EQ(2u, child->children().size());

    child1 = child->children()[0].get();
    EXPECT_EQ("push:recipient", child1->name());
    ASSERT_EQ(1u, child1->attributes().size());
    EXPECT_TRUE(child1->GetAttribute("to", &value));
    EXPECT_EQ(
        "4783f652b387449fc52a76f9a16e616f@clouddevices.gserviceaccount."
        "com",
        value);
    EXPECT_TRUE(child1->children().empty());

    child2 = child->children()[1].get();
    EXPECT_EQ("push:data", child2->name());
    EXPECT_TRUE(child2->attributes().empty());
    EXPECT_TRUE(child2->children().empty());
    const std::string expected_data =
        "eyJraW5kIjoiY2xvdWRkZXZpY2VzI25vdGlmaWNh"
        "dGlvbiIsInR5cGUiOiJDT01NQU5EX0NSRUFURUQiLCJjb21tYW5kSWQiOiIwNWE3MTA5MC"
        "1hZWE4LWMzNzQtOTYwNS0xZTRhY2JhNDRmM2Y4OTAzZmM3Yy01NjExLWI5ODAtOTkyMy0y"
        "Njc2YjYwYzkxMGMiLCJkZXZpY2VJZCI6IjA1YTcxMDkwLWFlYTgtYzM3NC05NjA1LTFlNG"
        "FjYmE0NGYzZiIsImNvbW1hbmQiOnsia2luZCI6ImNsb3VkZGV2aWNlcyNjb21tYW5kIiwi"
        "aWQiOiIwNWE3MTA5MC1hZWE4LWMzNzQtOTYwNS0xZTRhY2JhNDRmM2Y4OTAzZmM3Yy01Nj"
        "ExLWI5ODAtOTkyMy0yNjc2YjYwYzkxMGMiLCJkZXZpY2VJZCI6IjA1YTcxMDkwLWFlYTgt"
        "YzM3NC05NjA1LTFlNGFjYmE0NGYzZiIsIm5hbWUiOiJiYXNlLl9qdW1wIiwic3RhdGUiOi"
        "JxdWV1ZWQiLCJlcnJvciI6eyJhcmd1bWVudHMiOltdfSwiY3JlYXRpb25UaW1lTXMiOiIx"
        "NDMxNTY0NDY4MjI3IiwiZXhwaXJhdGlvblRpbWVNcyI6IjE0MzE1NjgwNjgyMjciLCJleH"
        "BpcmF0aW9uVGltZW91dE1zIjoiMzYwMDAwMCJ9fQ==";
    EXPECT_EQ(expected_data, child2->text());
  }
}

}  // namespace weave
