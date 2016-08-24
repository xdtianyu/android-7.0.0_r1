// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_XMPP_STREAM_PARSER_H_
#define LIBWEAVE_SRC_NOTIFICATION_XMPP_STREAM_PARSER_H_

#include <expat.h>

#include <map>
#include <memory>
#include <stack>
#include <string>

#include <base/macros.h>

namespace weave {

class XmlNode;

// A simple XML stream parser. As the XML data is being read from a data source
// (for example, a socket), XmppStreamParser::ParseData() should be called.
// This method parses the provided XML data chunk and if it finds complete
// XML elements, it will call internal OnOpenElement(), OnCloseElement() and
// OnCharData() member functions. These will track the element nesting level.
// When a top-level element starts, the parser will call Delegate::OnStreamStart
// method. Once this happens, every complete XML element (including its children
// if they are present) will trigger Delegate::OnStanze() callback.
// Finally, when top-level element is closed, Delegate::OnStreamEnd() is called.
// This class is specifically tailored to XMPP streams which look like this:
// B:  <stream:stream to='example.com' xmlns='jabber:client' version='1.0'>
// S:    <presence><show/></presence>
// S:    <message to='foo'><body/></message>
// S:    <iq to='bar'><query/></iq>
// S:    ...
// E:  </stream:stream>
// Here, "B:" will trigger OnStreamStart(), "S:" will result in OnStanza() and
// "E:" will result in OnStreamEnd().
class XmppStreamParser final {
 public:
  // Delegate interface that interested parties implement to receive
  // notifications of stream opening/closing and on new stanzas arriving.
  class Delegate {
   public:
    virtual void OnStreamStart(
        const std::string& node_name,
        std::map<std::string, std::string> attributes) = 0;
    virtual void OnStreamEnd(const std::string& node_name) = 0;
    virtual void OnStanza(std::unique_ptr<XmlNode> stanza) = 0;

   protected:
    virtual ~Delegate() {}
  };

  explicit XmppStreamParser(Delegate* delegate);
  ~XmppStreamParser();

  // Parses additional XML data received from an input stream.
  void ParseData(const std::string& data);

  // Resets the parser to expect the top-level stream node again.
  void Reset();

 private:
  // Raw expat callbacks.
  static void HandleElementStart(void* user_data,
                                 const XML_Char* element,
                                 const XML_Char** attr);
  static void HandleElementEnd(void* user_data, const XML_Char* element);
  static void HandleCharData(void* user_data, const char* content, int length);

  // Reinterpreted callbacks from expat with some data pre-processed.
  void OnOpenElement(const std::string& node_name,
                     std::map<std::string, std::string> attributes);
  void OnCloseElement(const std::string& node_name);
  void OnCharData(const std::string& text);

  Delegate* delegate_;
  XML_Parser parser_{nullptr};
  bool started_{false};
  std::stack<std::unique_ptr<XmlNode>> node_stack_;

  DISALLOW_COPY_AND_ASSIGN(XmppStreamParser);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_XMPP_STREAM_PARSER_H_
