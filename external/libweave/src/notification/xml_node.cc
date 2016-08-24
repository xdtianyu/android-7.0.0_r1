// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xml_node.h"

#include <base/strings/stringprintf.h>

#include "src/string_utils.h"

namespace weave {

XmlNode::XmlNode(const std::string& name,
                 std::map<std::string, std::string> attributes)
    : name_{name}, attributes_{std::move(attributes)} {}

const std::string& XmlNode::name() const {
  return name_;
}

const std::string& XmlNode::text() const {
  return text_;
}

const std::map<std::string, std::string>& XmlNode::attributes() const {
  return attributes_;
}

const std::vector<std::unique_ptr<XmlNode>>& XmlNode::children() const {
  return children_;
}

bool XmlNode::GetAttribute(const std::string& name, std::string* value) const {
  auto p = attributes_.find(name);
  if (p == attributes_.end())
    return false;

  *value = p->second;
  return true;
}

std::string XmlNode::GetAttributeOrEmpty(const std::string& name) const {
  std::string value;
  GetAttribute(name, &value);
  return value;
}

const XmlNode* XmlNode::FindFirstChild(const std::string& name_path,
                                       bool recursive) const {
  return FindChildHelper(name_path, recursive, nullptr);
}

std::vector<const XmlNode*> XmlNode::FindChildren(const std::string& name_path,
                                                  bool recursive) const {
  std::vector<const XmlNode*> children;
  FindChildHelper(name_path, recursive, &children);
  return children;
}

const XmlNode* XmlNode::FindChildHelper(
    const std::string& name_path,
    bool recursive,
    std::vector<const XmlNode*>* children) const {
  auto parts = SplitAtFirst(name_path, "/", false);
  const std::string& name = parts.first;
  const std::string& rest_of_path = parts.second;
  for (const auto& child : children_) {
    const XmlNode* found_node = nullptr;
    if (child->name() == name) {
      if (rest_of_path.empty()) {
        found_node = child.get();
      } else {
        found_node = child->FindChildHelper(rest_of_path, false, children);
      }
    } else if (recursive) {
      found_node = child->FindChildHelper(name_path, true, children);
    }

    if (found_node) {
      if (!children)
        return found_node;
      children->push_back(found_node);
    }
  }
  return nullptr;
}

void XmlNode::SetText(const std::string& text) {
  text_ = text;
}

void XmlNode::AppendText(const std::string& text) {
  text_ += text;
}

void XmlNode::AddChild(std::unique_ptr<XmlNode> child) {
  child->parent_ = this;
  children_.push_back(std::move(child));
}

std::string XmlNode::ToString() const {
  std::string xml = base::StringPrintf("<%s", name_.c_str());
  for (const auto& pair : attributes_) {
    base::StringAppendF(&xml, " %s=\"%s\"", pair.first.c_str(),
                        pair.second.c_str());
  }
  if (text_.empty() && children_.empty()) {
    xml += "/>";
  } else {
    xml += '>';
    if (!text_.empty()) {
      xml += text_;
    }
    for (const auto& child : children_) {
      xml += child->ToString();
    }
    base::StringAppendF(&xml, "</%s>", name_.c_str());
  }
  return xml;
}

}  // namespace weave
