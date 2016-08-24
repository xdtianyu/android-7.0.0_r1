// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xmpp_stream_parser.h"

#include "src/notification/xml_node.h"

namespace weave {

XmppStreamParser::XmppStreamParser(Delegate* delegate) : delegate_{delegate} {
  parser_ = XML_ParserCreate(nullptr);
  XML_SetUserData(parser_, this);
  XML_SetElementHandler(parser_, &XmppStreamParser::HandleElementStart,
                        &XmppStreamParser::HandleElementEnd);
  XML_SetCharacterDataHandler(parser_, &XmppStreamParser::HandleCharData);
}

XmppStreamParser::~XmppStreamParser() {
  XML_ParserFree(parser_);
}

void XmppStreamParser::ParseData(const std::string& data) {
  XML_Parse(parser_, data.data(), data.size(), 0);
}

void XmppStreamParser::Reset() {
  std::stack<std::unique_ptr<XmlNode>>{}.swap(node_stack_);
  started_ = false;
}

void XmppStreamParser::HandleElementStart(void* user_data,
                                          const XML_Char* element,
                                          const XML_Char** attr) {
  auto self = static_cast<XmppStreamParser*>(user_data);
  std::map<std::string, std::string> attributes;
  if (attr != nullptr) {
    for (size_t n = 0; attr[n] != nullptr && attr[n + 1] != nullptr; n += 2) {
      attributes.insert(std::make_pair(attr[n], attr[n + 1]));
    }
  }
  self->OnOpenElement(element, std::move(attributes));
}

void XmppStreamParser::HandleElementEnd(void* user_data,
                                        const XML_Char* element) {
  auto self = static_cast<XmppStreamParser*>(user_data);
  self->OnCloseElement(element);
}

void XmppStreamParser::HandleCharData(void* user_data,
                                      const char* content,
                                      int length) {
  auto self = static_cast<XmppStreamParser*>(user_data);
  self->OnCharData(std::string{content, static_cast<size_t>(length)});
}

void XmppStreamParser::OnOpenElement(
    const std::string& node_name,
    std::map<std::string, std::string> attributes) {
  if (!started_) {
    started_ = true;
    if (delegate_)
      delegate_->OnStreamStart(node_name, std::move(attributes));
    return;
  }
  node_stack_.emplace(new XmlNode{node_name, std::move(attributes)});
}

void XmppStreamParser::OnCloseElement(const std::string& node_name) {
  if (node_stack_.empty()) {
    if (started_) {
      started_ = false;
      if (delegate_)
        delegate_->OnStreamEnd(node_name);
    }
    return;
  }

  auto node = std::move(node_stack_.top());
  node_stack_.pop();
  if (!node_stack_.empty()) {
    XmlNode* parent = node_stack_.top().get();
    parent->AddChild(std::move(node));
  } else if (delegate_) {
    delegate_->OnStanza(std::move(node));
  }
}

void XmppStreamParser::OnCharData(const std::string& text) {
  if (!node_stack_.empty()) {
    XmlNode* node = node_stack_.top().get();
    node->AppendText(text);
  }
}

}  // namespace weave
