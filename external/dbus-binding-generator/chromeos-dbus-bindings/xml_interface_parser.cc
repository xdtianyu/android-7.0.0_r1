// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/xml_interface_parser.h"

#include <utility>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/strings/string_utils.h>

using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

// static
const char XmlInterfaceParser::kArgumentTag[] = "arg";
const char XmlInterfaceParser::kInterfaceTag[] = "interface";
const char XmlInterfaceParser::kMethodTag[] = "method";
const char XmlInterfaceParser::kNodeTag[] = "node";
const char XmlInterfaceParser::kSignalTag[] = "signal";
const char XmlInterfaceParser::kPropertyTag[] = "property";
const char XmlInterfaceParser::kAnnotationTag[] = "annotation";
const char XmlInterfaceParser::kDocStringTag[] = "tp:docstring";
const char XmlInterfaceParser::kNameAttribute[] = "name";
const char XmlInterfaceParser::kTypeAttribute[] = "type";
const char XmlInterfaceParser::kValueAttribute[] = "value";
const char XmlInterfaceParser::kDirectionAttribute[] = "direction";
const char XmlInterfaceParser::kAccessAttribute[] = "access";
const char XmlInterfaceParser::kArgumentDirectionIn[] = "in";
const char XmlInterfaceParser::kArgumentDirectionOut[] = "out";

const char XmlInterfaceParser::kTrue[] = "true";
const char XmlInterfaceParser::kFalse[] = "false";

const char XmlInterfaceParser::kMethodConst[] =
    "org.chromium.DBus.Method.Const";
const char XmlInterfaceParser::kMethodAsync[] =
    "org.freedesktop.DBus.GLib.Async";
const char XmlInterfaceParser::kMethodIncludeDBusMessage[] =
    "org.chromium.DBus.Method.IncludeDBusMessage";

const char XmlInterfaceParser::kMethodKind[] = "org.chromium.DBus.Method.Kind";
const char XmlInterfaceParser::kMethodKindSimple[] = "simple";
const char XmlInterfaceParser::kMethodKindNormal[] = "normal";
const char XmlInterfaceParser::kMethodKindAsync[] = "async";
const char XmlInterfaceParser::kMethodKindRaw[] = "raw";

namespace {

string GetElementPath(const vector<string>& path) {
  return brillo::string_utils::Join("/", path);
}

}  // anonymous namespace

bool XmlInterfaceParser::ParseXmlInterfaceFile(
    const std::string& contents,
    const std::vector<std::string>& ignore_interfaces) {
  auto parser = XML_ParserCreate(nullptr);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser,
                        &XmlInterfaceParser::HandleElementStart,
                        &XmlInterfaceParser::HandleElementEnd);
  XML_SetCharacterDataHandler(parser, &XmlInterfaceParser::HandleCharData);
  const int kIsFinal = XML_TRUE;

  element_path_.clear();
  XML_Status res = XML_Parse(parser,
                             contents.c_str(),
                             contents.size(),
                             kIsFinal);
  XML_ParserFree(parser);

  if (res != XML_STATUS_OK) {
    LOG(ERROR) << "XML parse failure";
    return false;
  }

  CHECK(element_path_.empty());

  if (!ignore_interfaces.empty()) {
    // Remove interfaces whose names are in |ignore_interfaces| list.
    auto condition = [&ignore_interfaces](const Interface& itf) {
      return std::find(ignore_interfaces.begin(), ignore_interfaces.end(),
                       itf.name) != ignore_interfaces.end();
    };
    auto p = std::remove_if(interfaces_.begin(), interfaces_.end(), condition);
    interfaces_.erase(p, interfaces_.end());
  }
  return true;
}

void XmlInterfaceParser::OnOpenElement(
    const string& element_name, const XmlAttributeMap& attributes) {
  string prev_element;
  if (!element_path_.empty())
    prev_element = element_path_.back();
  element_path_.push_back(element_name);
  if (element_name == kNodeTag) {
    CHECK(prev_element.empty() || prev_element == kNodeTag)
        << "Unexpected tag " << element_name << " inside " << prev_element;
    // 'name' attribute is optional for <node> element.
    string name;
    GetElementAttribute(attributes, element_path_, kNameAttribute, &name);
    base::TrimWhitespaceASCII(name, base::TRIM_ALL, &name);
    node_names_.push_back(name);
  } else if (element_name == kInterfaceTag) {
    CHECK_EQ(kNodeTag, prev_element)
        << "Unexpected tag " << element_name << " inside " << prev_element;
    string interface_name = GetValidatedElementName(attributes, element_path_);
    Interface itf(interface_name,
                  std::vector<Interface::Method>{},
                  std::vector<Interface::Signal>{},
                  std::vector<Interface::Property>{});
    itf.path = node_names_.back();
    interfaces_.push_back(std::move(itf));
  } else if (element_name == kMethodTag) {
    CHECK_EQ(kInterfaceTag, prev_element)
        << "Unexpected tag " << element_name << " inside " << prev_element;
    interfaces_.back().methods.push_back(
        Interface::Method(GetValidatedElementName(attributes, element_path_)));
  } else if (element_name == kSignalTag) {
    CHECK_EQ(kInterfaceTag, prev_element)
        << "Unexpected tag " << element_name << " inside " << prev_element;
    interfaces_.back().signals.push_back(
        Interface::Signal(GetValidatedElementName(attributes, element_path_)));
  } else if (element_name == kPropertyTag) {
    CHECK_EQ(kInterfaceTag, prev_element)
        << "Unexpected tag " << element_name << " inside " << prev_element;
    interfaces_.back().properties.push_back(ParseProperty(attributes,
                                                          element_path_));
  } else if (element_name == kArgumentTag) {
    if (prev_element == kMethodTag) {
      AddMethodArgument(attributes);
    } else if (prev_element == kSignalTag) {
      AddSignalArgument(attributes);
    } else {
      LOG(FATAL) << "Unexpected tag " << element_name
                 << " inside " << prev_element;
    }
  } else if (element_name == kAnnotationTag) {
    string name = GetValidatedElementAttribute(attributes, element_path_,
                                               kNameAttribute);
    // Value is optional. Default to empty string if omitted.
    string value;
    GetElementAttribute(attributes, element_path_, kValueAttribute, &value);
    if (prev_element == kInterfaceTag) {
      // Parse interface annotations...
    } else if (prev_element == kMethodTag) {
      // Parse method annotations...
      Interface::Method& method = interfaces_.back().methods.back();
      if (name == kMethodConst) {
        CHECK(value == kTrue || value == kFalse);
        method.is_const = (value == kTrue);
      } else if (name == kMethodIncludeDBusMessage) {
        CHECK(value == kTrue || value == kFalse);
        method.include_dbus_message = (value == kTrue);
      } else if (name == kMethodAsync) {
        // Support GLib.Async annotation as well.
        method.kind = Interface::Method::Kind::kAsync;
      } else if (name == kMethodKind) {
        if (value == kMethodKindSimple) {
          method.kind = Interface::Method::Kind::kSimple;
        } else if (value == kMethodKindNormal) {
          method.kind = Interface::Method::Kind::kNormal;
        } else if (value == kMethodKindAsync) {
          method.kind = Interface::Method::Kind::kAsync;
        } else if (value == kMethodKindRaw) {
          method.kind = Interface::Method::Kind::kRaw;
        } else {
          LOG(FATAL) << "Invalid method kind: " << value;
        }
      }
    } else if (prev_element == kSignalTag) {
      // Parse signal annotations...
    } else if (prev_element == kPropertyTag) {
      // Parse property annotations...
    } else {
      LOG(FATAL) << "Unexpected tag " << element_name
                 << " inside " << prev_element;
    }
  } else if (element_name == kDocStringTag) {
    CHECK(!prev_element.empty() && prev_element != kNodeTag)
        << "Unexpected tag " << element_name << " inside " << prev_element;
  }
}

void XmlInterfaceParser::OnCharData(const std::string& content) {
  // Handle the text data only for <tp:docstring> element.
  if (element_path_.back() != kDocStringTag)
    return;

  CHECK_GT(element_path_.size(), 1u);
  string* doc_string_ptr = nullptr;
  string target_element = element_path_[element_path_.size() - 2];
  if (target_element == kInterfaceTag) {
    doc_string_ptr = &(interfaces_.back().doc_string);
  } else if (target_element == kMethodTag) {
    doc_string_ptr = &(interfaces_.back().methods.back().doc_string);
  } else if (target_element == kSignalTag) {
    doc_string_ptr = &(interfaces_.back().signals.back().doc_string);
  } else if (target_element == kPropertyTag) {
    doc_string_ptr = &(interfaces_.back().properties.back().doc_string);
  }

  // If <tp:docstring> is attached to elements we don't care about, do nothing.
  if (doc_string_ptr == nullptr)
    return;

  (*doc_string_ptr) += content;
}


void XmlInterfaceParser::AddMethodArgument(const XmlAttributeMap& attributes) {
  string argument_direction;
  vector<string> path = element_path_;
  path.push_back(kArgumentTag);
  bool is_direction_paramter_present = GetElementAttribute(
      attributes, path, kDirectionAttribute, &argument_direction);
  vector<Interface::Argument>* argument_list = nullptr;
  if (!is_direction_paramter_present ||
      argument_direction == kArgumentDirectionIn) {
    argument_list = &interfaces_.back().methods.back().input_arguments;
  } else if (argument_direction == kArgumentDirectionOut) {
    argument_list = &interfaces_.back().methods.back().output_arguments;
  } else {
    LOG(FATAL) << "Unknown method argument direction " << argument_direction;
  }
  argument_list->push_back(ParseArgument(attributes, element_path_));
}

void XmlInterfaceParser::AddSignalArgument(const XmlAttributeMap& attributes) {
  interfaces_.back().signals.back().arguments.push_back(
      ParseArgument(attributes, element_path_));
}

void XmlInterfaceParser::OnCloseElement(const string& element_name) {
  VLOG(1) << "Close Element " << element_name;
  CHECK(!element_path_.empty());
  CHECK_EQ(element_path_.back(), element_name);
  element_path_.pop_back();
  if (element_name == kNodeTag) {
    CHECK(!node_names_.empty());
    node_names_.pop_back();
  }
}

// static
bool XmlInterfaceParser::GetElementAttribute(
    const XmlAttributeMap& attributes,
    const vector<string>& element_path,
    const string& element_key,
    string* element_value) {
  if (attributes.find(element_key) == attributes.end()) {
    return false;
  }
  *element_value = attributes.find(element_key)->second;
  VLOG(1) << "Got " << GetElementPath(element_path) << " element with "
          << element_key << " = " << *element_value;
  return true;
}

// static
string XmlInterfaceParser::GetValidatedElementAttribute(
    const XmlAttributeMap& attributes,
    const vector<string>& element_path,
    const string& element_key) {
  string element_value;
  CHECK(GetElementAttribute(attributes,
                            element_path,
                            element_key,
                            &element_value))
      << GetElementPath(element_path) << " does not contain a " << element_key
      << " attribute";
  CHECK(!element_value.empty()) << GetElementPath(element_path) << " "
                                << element_key << " attribute is empty";
  return element_value;
}

// static
string XmlInterfaceParser::GetValidatedElementName(
    const XmlAttributeMap& attributes,
    const vector<string>& element_path) {
  return GetValidatedElementAttribute(attributes, element_path, kNameAttribute);
}

// static
Interface::Argument XmlInterfaceParser::ParseArgument(
    const XmlAttributeMap& attributes, const vector<string>& element_path) {
  vector<string> path = element_path;
  path.push_back(kArgumentTag);
  string argument_name;
  // Since the "name" field is optional, use the un-validated variant.
  GetElementAttribute(attributes, path, kNameAttribute, &argument_name);

  string argument_type = GetValidatedElementAttribute(
      attributes, path, kTypeAttribute);
  return Interface::Argument(argument_name, argument_type);
}

// static
Interface::Property XmlInterfaceParser::ParseProperty(
    const XmlAttributeMap& attributes,
    const std::vector<std::string>& element_path) {
  vector<string> path = element_path;
  path.push_back(kPropertyTag);
  string property_name = GetValidatedElementName(attributes, path);
  string property_type = GetValidatedElementAttribute(attributes, path,
                                                      kTypeAttribute);
  string property_access = GetValidatedElementAttribute(attributes, path,
                                                        kAccessAttribute);
  return Interface::Property(property_name, property_type, property_access);
}

// static
void XmlInterfaceParser::HandleElementStart(void* user_data,
                                            const XML_Char* element,
                                            const XML_Char** attr) {
  XmlAttributeMap attributes;
  if (attr != nullptr) {
    for (size_t n = 0; attr[n] != nullptr && attr[n+1] != nullptr; n += 2) {
      auto key = attr[n];
      auto value = attr[n + 1];
      attributes.insert(std::make_pair(key, value));
    }
  }
  auto parser = reinterpret_cast<XmlInterfaceParser*>(user_data);
  parser->OnOpenElement(element, attributes);
}

// static
void XmlInterfaceParser::HandleElementEnd(void* user_data,
                                          const XML_Char* element) {
  auto parser = reinterpret_cast<XmlInterfaceParser*>(user_data);
  parser->OnCloseElement(element);
}

// static
void XmlInterfaceParser::HandleCharData(void* user_data,
                                        const char *content,
                                        int length) {
  auto parser = reinterpret_cast<XmlInterfaceParser*>(user_data);
  parser->OnCharData(string(content, length));
}

}  // namespace chromeos_dbus_bindings
