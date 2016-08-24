// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xml_node.h"

#include <memory>

#include <gtest/gtest.h>

#include "src/notification/xmpp_stream_parser.h"

namespace weave {
namespace {

class XmlParser : public XmppStreamParser::Delegate {
 public:
  std::unique_ptr<XmlNode> Parse(const std::string& xml) {
    parser_.ParseData(xml);
    return std::move(node_);
  }

 private:
  // Overrides from XmppStreamParser::Delegate.
  void OnStreamStart(const std::string& node_name,
                     std::map<std::string, std::string> attributes) override {
    node_.reset(new XmlNode{node_name, std::move(attributes)});
  }

  void OnStreamEnd(const std::string& node_name) override {}

  void OnStanza(std::unique_ptr<XmlNode> stanza) override {
    node_->AddChild(std::move(stanza));
  }

  std::unique_ptr<XmlNode> node_;
  XmppStreamParser parser_{this};
};

}  // anonymous namespace

class XmlNodeTest : public testing::Test {
 public:
  void SetUp() override {
    node_.reset(
        new XmlNode{"test_node", {{"attr1", "val1"}, {"attr2", "val2"}}});
  }

  // Accessor helpers for private members of XmlNode.
  static const XmlNode* GetParent(const XmlNode& node) { return node.parent_; }

  static void SetText(XmlNode* node, const std::string& text) {
    node->SetText(text);
  }

  static void AppendText(XmlNode* node, const std::string& text) {
    node->AppendText(text);
  }

  void CreateNodeTree() {
    node_ = XmlParser{}.Parse(R"(
        <top>
          <node1 id="1"><node2 id="2"><node3 id="3"/></node2></node1>
          <node2 id="4"><node3 id="5"/></node2>
          <node3 id="6"/>
          <node2 id="7"><node4 id="8"><node3 id="9"/></node4></node2>
        </top>
        )");
  }

  std::unique_ptr<XmlNode> node_;
};

TEST_F(XmlNodeTest, DefaultConstruction) {
  EXPECT_EQ("test_node", node_->name());
  EXPECT_TRUE(node_->children().empty());
  EXPECT_TRUE(node_->text().empty());
}

TEST_F(XmlNodeTest, SetText) {
  SetText(node_.get(), "foobar");
  EXPECT_EQ("foobar", node_->text());
}

TEST_F(XmlNodeTest, AppendText) {
  SetText(node_.get(), "foobar");
  AppendText(node_.get(), "-baz");
  EXPECT_EQ("foobar-baz", node_->text());
}

TEST_F(XmlNodeTest, AddChild) {
  std::unique_ptr<XmlNode> child{new XmlNode{"child", {}}};
  node_->AddChild(std::move(child));
  EXPECT_EQ(1u, node_->children().size());
  EXPECT_EQ("child", node_->children().front()->name());
  EXPECT_EQ(node_.get(), GetParent(*node_->children().front().get()));
}

TEST_F(XmlNodeTest, Attributes) {
  const std::map<std::string, std::string> expected_attrs{{"attr1", "val1"},
                                                          {"attr2", "val2"}};
  EXPECT_EQ(expected_attrs, node_->attributes());
  std::string attr = "bar";
  EXPECT_FALSE(node_->GetAttribute("foo", &attr));
  EXPECT_EQ("bar", attr);  // Shouldn't be changed by failed GetAttribute().
  EXPECT_TRUE(node_->GetAttribute("attr1", &attr));
  EXPECT_EQ("val1", attr);
  EXPECT_TRUE(node_->GetAttribute("attr2", &attr));
  EXPECT_EQ("val2", attr);

  XmlNode new_node{"node", {}};
  EXPECT_FALSE(new_node.GetAttribute("attr1", &attr));
}

TEST_F(XmlNodeTest, FindFirstChild_SingleNode) {
  CreateNodeTree();
  const XmlNode* node = node_->FindFirstChild("node3", false);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("6", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("node3", true);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("3", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("foo", true);
  ASSERT_EQ(nullptr, node);
}

TEST_F(XmlNodeTest, FindFirstChild_Path) {
  CreateNodeTree();
  const XmlNode* node = node_->FindFirstChild("node2/node3", false);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("5", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("node2/node3", true);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("3", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("node1/node2/node3", false);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("3", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("node1/node2/node3", true);
  ASSERT_NE(nullptr, node);
  EXPECT_EQ("node3", node->name());
  EXPECT_EQ("3", node->GetAttributeOrEmpty("id"));

  node = node_->FindFirstChild("foo/node3", true);
  ASSERT_EQ(nullptr, node);
}

TEST_F(XmlNodeTest, FindChildren_SingleNode) {
  CreateNodeTree();
  auto children = node_->FindChildren("node3", false);
  ASSERT_EQ(1u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("6", children[0]->GetAttributeOrEmpty("id"));

  children = node_->FindChildren("node3", true);
  ASSERT_EQ(4u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("3", children[0]->GetAttributeOrEmpty("id"));
  EXPECT_EQ("node3", children[1]->name());
  EXPECT_EQ("5", children[1]->GetAttributeOrEmpty("id"));
  EXPECT_EQ("node3", children[2]->name());
  EXPECT_EQ("6", children[2]->GetAttributeOrEmpty("id"));
  EXPECT_EQ("node3", children[3]->name());
  EXPECT_EQ("9", children[3]->GetAttributeOrEmpty("id"));
}

TEST_F(XmlNodeTest, FindChildren_Path) {
  CreateNodeTree();
  auto children = node_->FindChildren("node2/node3", false);
  ASSERT_EQ(1u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("5", children[0]->GetAttributeOrEmpty("id"));

  children = node_->FindChildren("node2/node3", true);
  ASSERT_EQ(2u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("3", children[0]->GetAttributeOrEmpty("id"));
  EXPECT_EQ("node3", children[1]->name());
  EXPECT_EQ("5", children[1]->GetAttributeOrEmpty("id"));

  children = node_->FindChildren("node1/node2/node3", false);
  ASSERT_EQ(1u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("3", children[0]->GetAttributeOrEmpty("id"));

  children = node_->FindChildren("node1/node2/node3", true);
  ASSERT_EQ(1u, children.size());
  EXPECT_EQ("node3", children[0]->name());
  EXPECT_EQ("3", children[0]->GetAttributeOrEmpty("id"));

  children = node_->FindChildren("foo/bar", false);
  ASSERT_EQ(0u, children.size());

  children = node_->FindChildren("node2/baz", false);
  ASSERT_EQ(0u, children.size());
}

}  // namespace weave
