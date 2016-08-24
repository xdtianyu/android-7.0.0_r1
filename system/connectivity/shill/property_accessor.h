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

#ifndef SHILL_PROPERTY_ACCESSOR_H_
#define SHILL_PROPERTY_ACCESSOR_H_

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST.

#include "shill/accessor_interface.h"
#include "shill/error.h"
#include "shill/logging.h"

namespace shill {

// Templated implementations of AccessorInterface<>.
//
// PropertyAccessor<>, ConstPropertyAccessor<>, and
// WriteOnlyPropertyAccessor<> provide R/W, R/O, and W/O access
// (respectively) to the value pointed to by |property|.
//
// This allows a class to easily map strings to member variables, so that
// pieces of state stored in the class can be queried or updated by name.
//
//   bool foo = true;
//   map<string, BoolAccessor> accessors;
//   accessors["foo"] = BoolAccessor(new PropertyAccessor<bool>(&foo));
//   bool new_foo = accessors["foo"]->Get();  // new_foo == true
//   accessors["foo"]->Set(false);  // returns true, because setting is allowed.
//                                  // foo == false, new_foo == true
//   new_foo = accessors["foo"]->Get();  // new_foo == false
//   // Clear resets |foo| to its value when the PropertyAccessor was created.
//   accessors["foo"]->Clear();  // foo == true
//
// Generic accessors that provide write capability will check that the
// new value differs from the present one. If the old and new values
// are the same, the setter will not invoke the assignment operator, and
// will return false.
//
// Custom accessors are responsible for handling set-to-same-value
// themselves. It is not possible to handle that here, because some
// custom getters return default values, rather than the actual
// value. (I'm looking at you, WiFi::GetBgscanMethod.)
template <class T>
class PropertyAccessor : public AccessorInterface<T> {
 public:
  explicit PropertyAccessor(T* property)
      : property_(property), default_value_(*property) {
    DCHECK(property);
  }
  ~PropertyAccessor() override {}

  void Clear(Error* error) override { Set(default_value_, error); }
  T Get(Error* /*error*/) override { return *property_; }
  bool Set(const T& value, Error* /*error*/) override {
    if (*property_ == value) {
      return false;
    }
    *property_ = value;
    return true;
  }

 private:
  T* const property_;
  const T default_value_;
  DISALLOW_COPY_AND_ASSIGN(PropertyAccessor);
};

template <class T>
class ConstPropertyAccessor : public AccessorInterface<T> {
 public:
  explicit ConstPropertyAccessor(const T* property) : property_(property) {
    DCHECK(property);
  }
  ~ConstPropertyAccessor() override {}

  void Clear(Error* error) override {
    // TODO(quiche): check if this is the right error.
    // (maybe Error::kInvalidProperty instead?)
    error->Populate(Error::kInvalidArguments, "Property is read-only");
  }
  T Get(Error* /*error*/) override { return *property_; }
  bool Set(const T& /*value*/, Error* error) override {
    // TODO(quiche): check if this is the right error.
    // (maybe Error::kPermissionDenied instead?)
    error->Populate(Error::kInvalidArguments, "Property is read-only");
    return false;
  }

 private:
  const T* const property_;
  DISALLOW_COPY_AND_ASSIGN(ConstPropertyAccessor);
};

template <class T>
class WriteOnlyPropertyAccessor : public AccessorInterface<T> {
 public:
  explicit WriteOnlyPropertyAccessor(T* property)
      : property_(property), default_value_(*property) {
    DCHECK(property);
  }
  ~WriteOnlyPropertyAccessor() override {}

  void Clear(Error* error) override { Set(default_value_, error); }
  T Get(Error* error) override {
    error->Populate(Error::kPermissionDenied, "Property is write-only");
    return T();
  }
  bool Set(const T& value, Error* /*error*/) override {
    if (*property_ == value) {
      return false;
    }
    *property_ = value;
    return true;
  }

 private:
  FRIEND_TEST(PropertyAccessorTest, SignedIntCorrectness);
  FRIEND_TEST(PropertyAccessorTest, UnsignedIntCorrectness);
  FRIEND_TEST(PropertyAccessorTest, StringCorrectness);
  FRIEND_TEST(PropertyAccessorTest, ByteArrayCorrectness);

  T* const property_;
  const T default_value_;
  DISALLOW_COPY_AND_ASSIGN(WriteOnlyPropertyAccessor);
};

// CustomAccessor<> allows custom getter and setter methods to be provided.
// Thus, if the state to be returned is to be derived on-demand, or if
// setting the property requires validation, we can still fit it into the
// AccessorInterface<> framework.
//
// If the property is write-only, use CustomWriteOnlyAccessor instead.
template<class C, class T>
class CustomAccessor : public AccessorInterface<T> {
 public:
  // |target| is the object on which to call the methods |getter|, |setter|
  // and |clearer|.  |setter| is allowed to be NULL, in which case we will
  // simply reject attempts to set via the accessor. |setter| should return
  // true if the value was changed, and false otherwise.  |clearer| is allowed
  // to be NULL (which is what happens if it is not passed to the constructor),
  // in which case, |setter| is called is called with the default value.
  // It is an error to pass NULL for either |target| or |getter|.
  CustomAccessor(C* target,
                 T(C::*getter)(Error* error),
                 bool(C::*setter)(const T& value, Error* error),
                 void(C::*clearer)(Error* error))
      : target_(target),
        default_value_(),
        getter_(getter),
        setter_(setter),
        clearer_(clearer) {
    DCHECK(target);
    DCHECK(getter);  // otherwise, use CustomWriteOnlyAccessor
    if (setter_) {
      Error e;
      default_value_ = Get(&e);
    }
  }
  CustomAccessor(C* target,
                 T(C::*getter)(Error* error),
                 bool(C::*setter)(const T& value, Error* error))
      : CustomAccessor(target, getter, setter, nullptr) {}
  ~CustomAccessor() override {}

  void Clear(Error* error) override {
    if (clearer_) {
      (target_->*clearer_)(error);
    } else {
      Set(default_value_, error);
    }
  }
  T Get(Error* error) override {
    return (target_->*getter_)(error);
  }
  bool Set(const T& value, Error* error) override {
    if (setter_) {
      return (target_->*setter_)(value, error);
    } else {
      error->Populate(Error::kInvalidArguments, "Property is read-only");
      return false;
    }
  }

 private:
  C* const target_;
  // |default_value_| is non-const because it can't be initialized in
  // the initializer list.
  T default_value_;
  T(C::*const getter_)(Error* error);
  bool(C::*const setter_)(const T& value, Error* error);  // NOLINT - "casting"
  void(C::*const clearer_)(Error* error);
  DISALLOW_COPY_AND_ASSIGN(CustomAccessor);
};

// CustomWriteOnlyAccessor<> allows a custom writer method to be provided.
// Get returns an error automatically. Clear resets the value to a
// default value.
template<class C, class T>
class CustomWriteOnlyAccessor : public AccessorInterface<T> {
 public:
  // |target| is the object on which to call |setter| and |clearer|.
  //
  // |target| and |setter| must be non-NULL. |setter| should return true
  // if the value was changed, and false otherwise.
  //
  // Either |clearer| or |default_value|, but not both, must be non-NULL.
  // Whichever is non-NULL is used to clear the property.
  CustomWriteOnlyAccessor(C* target,
                          bool(C::*setter)(const T& value, Error* error),
                          void(C::*clearer)(Error* error),
                          const T* default_value)
      : target_(target),
        setter_(setter),
        clearer_(clearer),
        default_value_() {
    DCHECK(target);
    DCHECK(setter);
    DCHECK(clearer || default_value);
    DCHECK(!clearer || !default_value);
    if (default_value) {
      default_value_ = *default_value;
    }
  }
  ~CustomWriteOnlyAccessor() override {}

  void Clear(Error* error) override {
    if (clearer_) {
      (target_->*clearer_)(error);
    } else {
      Set(default_value_, error);
    }
  }
  T Get(Error* error) override {
    error->Populate(Error::kPermissionDenied, "Property is write-only");
    return T();
  }
  bool Set(const T& value, Error* error) override {
    return (target_->*setter_)(value, error);
  }

 private:
  C* const target_;
  bool(C::*const setter_)(const T& value, Error* error);  // NOLINT - "casting"
  void(C::*const clearer_)(Error* error);
  // |default_value_| is non-const because it can't be initialized in
  // the initializer list.
  T default_value_;
  DISALLOW_COPY_AND_ASSIGN(CustomWriteOnlyAccessor);
};

// CustomReadOnlyAccessor<> allows a custom getter method to be provided.
// Set and Clear return errors automatically.
template<class C, class T>
class CustomReadOnlyAccessor : public AccessorInterface<T> {
 public:
  // |target| is the object on which to call the |getter| method.
  // |getter| is a const method.  If a non-const method needs to be used,
  // use the CustomAccessor with a NULL setter instead.
  CustomReadOnlyAccessor(C* target, T(C::*getter)(Error* error) const)
      : target_(target), getter_(getter) {
    DCHECK(target);
    DCHECK(getter);
  }
  ~CustomReadOnlyAccessor() override {}

  void Clear(Error* error) override {
    error->Populate(Error::kInvalidArguments, "Property is read-only");
  }
  T Get(Error* error) override {
    return (target_->*getter_)(error);
  }
  bool Set(const T& value, Error* error) override {
    error->Populate(Error::kInvalidArguments, "Property is read-only");
    return false;
  }

 private:
  C* const target_;
  T(C::*const getter_)(Error* error) const;
  DISALLOW_COPY_AND_ASSIGN(CustomReadOnlyAccessor);
};

// CustomMappedAccessor<> passes an argument to the getter and setter
// so that a generic method can be used, for example one that accesses the
// property in a map.
template<class C, class T, class A>
class CustomMappedAccessor : public AccessorInterface<T> {
 public:
  // |target| is the object on which to call the methods |getter| and |setter|.
  // |setter| is allowed to be NULL, in which case we will simply reject
  // attempts to set via the accessor. |setter| should return true if the
  // value was changed, and false otherwise.
  // |argument| is passed to the getter and setter methods to disambiguate
  // between different properties in |target|.
  // It is an error to pass NULL for any of |target|, |clearer| or |getter|.
  CustomMappedAccessor(C* target,
                       void(C::*clearer)(const A& argument, Error* error),
                       T(C::*getter)(const A& argument, Error* error),
                       bool(C::*setter)(const A& argument, const T& value,
                                        Error* error),
                       const A& argument)
      : target_(target),
        clearer_(clearer),
        getter_(getter),
        setter_(setter),
        argument_(argument) {
    DCHECK(clearer);
    DCHECK(target);
    DCHECK(getter);
  }
  ~CustomMappedAccessor() override {}

  void Clear(Error* error) override {
    (target_->*clearer_)(argument_, error);
  }
  T Get(Error* error) override {
    return (target_->*getter_)(argument_, error);
  }
  bool Set(const T& value, Error* error) override {
    if (setter_) {
      return (target_->*setter_)(argument_, value, error);
    } else {
      error->Populate(Error::kInvalidArguments, "Property is read-only");
      return false;
    }
  }

 private:
  C* const target_;
  void(C::*const clearer_)(const A& argument, Error* error);
  T(C::*const getter_)(const A& argument, Error* error);
  bool(C::*const setter_)(const A& argument,  // NOLINT - "casting"
                          const T& value, Error* error);
  A argument_;
  DISALLOW_COPY_AND_ASSIGN(CustomMappedAccessor);
};

}  // namespace shill

#endif  // SHILL_PROPERTY_ACCESSOR_H_
