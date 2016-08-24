// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is an implementation of a "true" variant class in C++.
// The brillo::Any class can hold any C++ type, but both the setter and
// getter sites need to know the actual type of data.
// Note that C-style arrays when stored in Any are reduced to simple
// data pointers. Any will not copy a contents of the array.
//    const int data[] = [1,2,3];
//    Any v(data);  // stores const int*, effectively "Any v(&data[0]);"

// brillo::Any is a value type. Which means, the data is copied into it
// and Any owns it. The owned object (stored by value) will be destroyed
// when Any is cleared or reassigned. The contained value type must be
// copy-constructible. You can also store pointers and references to objects.
// Storing pointers is trivial. In order to store a reference, you can
// use helper functions std::ref() and std::cref() to create non-const and
// const references respectively. In such a case, the type of contained data
// will be std::reference_wrapper<T>. See 'References' unit tests in
// any_unittest.cc for examples.

#ifndef LIBBRILLO_BRILLO_ANY_H_
#define LIBBRILLO_BRILLO_ANY_H_

#include <brillo/any_internal_impl.h>

#include <algorithm>

#include <brillo/brillo_export.h>
#include <brillo/type_name_undecorate.h>

namespace dbus {
class MessageWriter;
}  // namespace dbus

namespace brillo {

class BRILLO_EXPORT Any final {
 public:
  Any();  // Do not inline to hide internal_details::Buffer from export table.
  // Standard copy/move constructors. This is a value-class container
  // that must be copy-constructible and movable. The copy constructors
  // should not be marked as explicit.
  Any(const Any& rhs);
  Any(Any&& rhs);  // NOLINT(build/c++11)
  // Typed constructor that stores a value of type T in the Any.
  template<class T>
  inline Any(T value) {  // NOLINT(runtime/explicit)
    data_buffer_.Assign(std::move(value));
  }

  // Not declaring the destructor as virtual since this is a sealed class
  // and there is no need to introduce a virtual table to it.
  ~Any();

  // Assignment operators.
  Any& operator=(const Any& rhs);
  Any& operator=(Any&& rhs);  // NOLINT(build/c++11)
  template<class T>
  inline Any& operator=(T value) {
    data_buffer_.Assign(std::move(value));
    return *this;
  }

  // Compares the contents of two Any objects for equality. Note that the
  // contained type must be equality-comparable (must have operator== defined).
  // If operator==() is not available for contained type, comparison operation
  // always returns false (as if the data were different).
  bool operator==(const Any& rhs) const;
  inline bool operator!=(const Any& rhs) const { return !operator==(rhs); }

  // Checks if the given type DestType can be obtained from the Any.
  // For example, to check if Any has a 'double' value in it:
  //  any.IsTypeCompatible<double>()
  template<typename DestType>
  bool IsTypeCompatible() const {
    // Make sure the requested type DestType conforms to the storage
    // requirements of Any. We always store the data by value, which means we
    // strip away any references as well as cv-qualifiers. So, if the user
    // stores "const int&", we actually store just an "int".
    // When calling IsTypeCompatible, we need to do a similar "type cleansing"
    // to make sure the requested type matches the type of data actually stored,
    // so this "canonical" type is used for type checking below.
    using CanonicalDestType = typename std::decay<DestType>::type;
    const char* contained_type = GetTypeTagInternal();
    if (strcmp(GetTypeTag<CanonicalDestType>(), contained_type) == 0)
      return true;

    if (!std::is_pointer<CanonicalDestType>::value)
      return false;

    // If asking for a const pointer from a variant containing non-const
    // pointer, still satisfy the request. So, we need to remove the pointer
    // specification first, then strip the const/volatile qualifiers, then
    // re-add the pointer back, so "const int*" would become "int*".
    using NonPointer = typename std::remove_pointer<CanonicalDestType>::type;
    using CanonicalDestTypeNoConst = typename std::add_pointer<
        typename std::remove_const<NonPointer>::type>::type;
    if (strcmp(GetTypeTag<CanonicalDestTypeNoConst>(), contained_type) == 0)
      return true;

    using CanonicalDestTypeNoVolatile = typename std::add_pointer<
        typename std::remove_volatile<NonPointer>::type>::type;
    if (strcmp(GetTypeTag<CanonicalDestTypeNoVolatile>(), contained_type) == 0)
      return true;

    using CanonicalDestTypeNoConstOrVolatile = typename std::add_pointer<
        typename std::remove_cv<NonPointer>::type>::type;
    return strcmp(GetTypeTag<CanonicalDestTypeNoConstOrVolatile>(),
                  contained_type) == 0;
  }

  // Returns immutable data contained in Any.
  // Aborts if Any doesn't contain a value of type T, or trivially
  // convertible to/compatible with it.
  template<typename T>
  const T& Get() const {
    CHECK(IsTypeCompatible<T>())
        << "Requesting value of type '" << brillo::GetUndecoratedTypeName<T>()
        << "' from variant containing '" << GetUndecoratedTypeName()
        << "'";
    return data_buffer_.GetData<T>();
  }

  // Returns a copy of data in Any and returns true when that data is
  // compatible with T.  Returns false if contained data is incompatible.
  template<typename T>
  bool GetValue(T* value) const {
    if (!IsTypeCompatible<T>()) {
      return false;
    }
    *value = Get<T>();
    return true;
  }

  // Returns a pointer to mutable value of type T contained within Any.
  // No data copying is made, the data pointed to is still owned by Any.
  // If Any doesn't contain a value of type T, or trivially
  // convertible/compatible to/with it, then it returns nullptr.
  template<typename T>
  T* GetPtr() {
    if (!IsTypeCompatible<T>())
      return nullptr;
    return &(data_buffer_.GetData<T>());
  }

  // Returns a copy of the data contained in Any.
  // If the Any doesn't contain a compatible value, the provided default
  // |def_val| is returned instead.
  template<typename T>
  T TryGet(typename std::decay<T>::type const& def_val) const {
    if (!IsTypeCompatible<T>())
      return def_val;
    return data_buffer_.GetData<T>();
  }

  // A convenience specialization of the above function where the default
  // value of type T is returned in case the underlying Get() fails.
  template<typename T>
  T TryGet() const {
    return TryGet<T>(typename std::decay<T>::type());
  }

  // Returns the undecorated name of the type contained within Any.
  inline std::string GetUndecoratedTypeName() const {
    return GetUndecoratedTypeNameForTag(GetTypeTagInternal());
  }
  // Swaps the value of this object with that of |other|.
  void Swap(Any& other);
  // Checks if Any is empty, that is, not containing a value of any type.
  bool IsEmpty() const;
  // Clears the Any and destroys any contained object. Makes it empty.
  void Clear();
  // Checks if Any contains a type convertible to integer.
  // Any type that match std::is_integral<T> and std::is_enum<T> is accepted.
  // That includes signed and unsigned char, short, int, long, etc as well as
  // 'bool' and enumerated types.
  // For 'integer' type, you can call GetAsInteger to do implicit type
  // conversion to intmax_t.
  bool IsConvertibleToInteger() const;
  // For integral types and enums contained in the Any, get the integer value
  // of data. This is a useful function to obtain an integer value when
  // any can possibly have unspecified integer, such as 'short', 'unsigned long'
  // and so on.
  intmax_t GetAsInteger() const;
  // Writes the contained data to D-Bus message writer, if the appropriate
  // serialization method for contained data of the given type is provided
  // (an appropriate specialization of AppendValueToWriter<T>() is available).
  // Returns false if the Any is empty or if there is no serialization method
  // defined for the contained data.
  void AppendToDBusMessageWriter(dbus::MessageWriter* writer) const;

 private:
  // Returns a pointer to a static buffer containing type tag (sort of a type
  // name) of the contained value.
  const char* GetTypeTagInternal() const;

  // The data buffer for contained object.
  internal_details::Buffer data_buffer_;
};

}  // namespace brillo

namespace std {

// Specialize std::swap() algorithm for brillo::Any class.
inline void swap(brillo::Any& lhs, brillo::Any& rhs) {
  lhs.Swap(rhs);
}

}  // namespace std

#endif  // LIBBRILLO_BRILLO_ANY_H_
