//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_PROPERTY_ITERATOR_H_
#define SHILL_PROPERTY_ITERATOR_H_

#include <map>
#include <string>

#include "shill/accessor_interface.h"
#include "shill/error.h"

namespace shill {

// An iterator wrapper class to hide the details of what kind of data structure
// we're using to store key/value pairs for properties. It is intended for use
// with PropertyStore and always advances to the next readable property.
template <class V>
class ReadablePropertyConstIterator {
 public:
  ~ReadablePropertyConstIterator() {}

  bool AtEnd() const { return it_ == collection_.end(); }

  void Advance() {
    if (!AtEnd()) {
      do {
        ++it_;
      } while (MustAdvance());
    }
  }

  const std::string& Key() const { return it_->first; }

  const V& value() const { return value_; }

 private:
  friend class PropertyStore;

  typedef std::shared_ptr<AccessorInterface<V>> VAccessorPtr;

  explicit ReadablePropertyConstIterator(
      const typename std::map<std::string, VAccessorPtr>& collection)
      : collection_(collection),
        it_(collection_.begin()),
        value_() {
    if (MustAdvance()) {
      Advance();
    }
  }

  bool MustAdvance() { return !AtEnd() && !RetrieveCurrentValue(); }

  bool RetrieveCurrentValue() {
    Error error;
    value_ = it_->second->Get(&error);
    return error.IsSuccess();
  }

  const typename std::map<std::string, VAccessorPtr>& collection_;
  typename std::map<std::string, VAccessorPtr>::const_iterator it_;
  V value_;
};

}  // namespace shill

#endif  // SHILL_PROPERTY_ITERATOR_H_
