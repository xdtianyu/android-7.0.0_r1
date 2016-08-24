// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_XML_NODE_H_
#define LIBWEAVE_SRC_NOTIFICATION_XML_NODE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

namespace weave {

class XmlNodeTest;
class XmppStreamParser;

// XmlNode is a very simple class to represent the XML document element tree.
// It is used in conjunction with expat XML parser to implement XmppStreamParser
// class used to parse Xmpp data stream into individual stanzas.
class XmlNode final {
 public:
  XmlNode(const std::string& name,
          std::map<std::string, std::string> attributes);

  // The node's name. E.g. in <foo bar="baz">quux</foo> this will return "foo".
  const std::string& name() const;
  // The node text content. E.g. in <foo bar="baz">quux</foo> this will return
  // "quux".
  const std::string& text() const;
  // The node attribute map. E.g. in <foo bar="baz">quux</foo> this will return
  // {{"bar", "baz"}}.
  const std::map<std::string, std::string>& attributes() const;
  // Returns the list of child nodes, if any.
  const std::vector<std::unique_ptr<XmlNode>>& children() const;

  // Retrieves the value of the given attribute specified by |name|.
  // If the attribute doesn't exist, returns false and |value| is not modified.
  bool GetAttribute(const std::string& name, std::string* value) const;
  // Returns the value of the given attribute specified by |name|.
  // Returns empty string if the attribute does not exist. This method should be
  // used only in limited scopes such as unit tests.
  std::string GetAttributeOrEmpty(const std::string& name) const;

  // Finds a first occurrence of a child node specified by |name_path|. A name
  // path is a "/"-separated list of node names to look for. If |recursive| is
  // set to true, the children are recursively traversed trying to match the
  // node names. Otherwise only first-level children of the current node are
  // matched against the top-level name of |name_path|.
  // This method returns a pointer to the first node that matches the path,
  // otherwise a nullptr is returned.
  const XmlNode* FindFirstChild(const std::string& name_path,
                                bool recursive) const;

  // Finds all the child nodes matching the |name_path|. This returns the list
  // of pointers to the child nodes matching the criteria. If |recursive| is
  // set to true, the children are recursively traversed trying to match the
  // node names. Otherwise only first-level children of the current node are
  // matched against the top-level name of |name_path|.
  // For example, if the current node represents the <top> element of the
  // following XML document:
  //  <top>
  //    <node1 id="1"><node2 id="2"><node3 id="3"/></node2></node1>
  //    <node2 id="4"><node3 id="5"/></node2>
  //    <node3 id="6"/>
  //    <node2 id="7"><node4 id="8"><node3 id="9"/></node4></node2>
  //  </top>
  // Then recursively searching for nodes will produce the following results
  // (only the node "id" attributes are listed in the results, for brevity):
  //    FindChildren("node2/node3", false) -> {"5"}.
  //    FindChildren("node2/node3", true) -> {"3", "5"}.
  //    FindChildren("node3", false) -> {"6"}.
  //    FindChildren("node3", true) -> {"3", "5", "6", "9"}.
  std::vector<const XmlNode*> FindChildren(const std::string& name_path,
                                           bool recursive) const;

  // Adds a new child to the bottom of the child list of this node.
  void AddChild(std::unique_ptr<XmlNode> child);

  // Converts the node tree to XML-like string. Note that this not necessarily
  // produces a valid XML string. It does not use any character escaping or
  // canonicalization, which will produce invalid XML if any of the node or
  // attribute names or values contain special characters such as ", <, >, etc.
  // This function should be used only for logging/debugging purposes only and
  // never to generate valid XML from the parsed node tree.
  std::string ToString() const;

 private:
  friend class XmlNodeTest;
  friend class XmppStreamParser;

  // Sets the node's text. Used by XML parser.
  void SetText(const std::string& text);
  // Appends the |text| to the node's text string.
  void AppendText(const std::string& text);

  // Helper method used by FindFirstChild() and FindChildren(). Searches for
  // child node(s) matching |name_path|.
  // If |children| is not specified (nullptr), this function find the first
  // matching node and returns it via return value of the function. If no match
  // is found, this function will return nullptr.
  // If |children| parameter is not nullptr, found nodes are added to the
  // vector pointed to by |children| and search continues until the whole tree
  // is inspected. In this mode, the function always returns nullptr.
  const XmlNode* FindChildHelper(const std::string& name_path,
                                 bool recursive,
                                 std::vector<const XmlNode*>* children) const;

  const XmlNode* parent_{nullptr};  // Weak pointer to the parent node, if any.
  std::string name_;
  std::string text_;
  std::map<std::string, std::string> attributes_;
  std::vector<std::unique_ptr<XmlNode>> children_;

  DISALLOW_COPY_AND_ASSIGN(XmlNode);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_XML_NODE_H_
