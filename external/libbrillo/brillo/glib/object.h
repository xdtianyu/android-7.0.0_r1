// Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_GLIB_OBJECT_H_
#define LIBBRILLO_BRILLO_GLIB_OBJECT_H_

#include <glib-object.h>
#include <stdint.h>

#include <base/logging.h>
#include <base/macros.h>
#include <base/memory/scoped_ptr.h>

#include <algorithm>
#include <cstddef>
#include <string>

namespace brillo {

namespace details {  // NOLINT

// \brief ResetHelper is a private class for use with Resetter().
//
// ResetHelper passes ownership of a pointer to a scoped pointer type with reset
// on destruction.

template <typename T>  // T models ScopedPtr
class ResetHelper {
 public:
  typedef typename T::element_type element_type;

  explicit ResetHelper(T* x)
      : ptr_(nullptr),
        scoped_(x) {
  }
  ~ResetHelper() {
    scoped_->reset(ptr_);
  }
  element_type*& lvalue() {
    return ptr_;
  }

 private:
  element_type* ptr_;
  T* scoped_;
};

}  // namespace details

// \brief Resetter() is a utility function for passing pointers to
//  scoped pointers.
//
// The Resetter() function return a temporary object containing an lvalue of
// \code T::element_type which can be assigned to. When the temporary object
// destructs, the associated scoped pointer is reset with the lvalue. It is of
// general use when a pointer is returned as an out-argument.
//
// \example
// void function(int** x) {
//   *x = new int(10);
// }
// ...
// scoped_ptr<int> x;
// function(Resetter(x).lvalue());
//
// \end_example

template <typename T>  // T models ScopedPtr
details::ResetHelper<T> Resetter(T* x) {
  return details::ResetHelper<T>(x);
}

// \precondition No functions in the glib namespace can be called before
// ::g_type_init();

namespace glib {

// \brief type_to_gtypeid is a type function mapping from a canonical type to
// the GType typeid for the associated GType (see type_to_gtype).

template <typename T> ::GType type_to_gtypeid();

template < >
inline ::GType type_to_gtypeid<const char*>() {
  return G_TYPE_STRING;
}
template < >
inline ::GType type_to_gtypeid<char*>() {
  return G_TYPE_STRING;
}
template < >
inline ::GType type_to_gtypeid< ::uint8_t>() {
  return G_TYPE_UCHAR;
}
template < >
inline ::GType type_to_gtypeid<double>() {
  return G_TYPE_DOUBLE;
}
template < >
inline ::GType type_to_gtypeid<bool>() {
  return G_TYPE_BOOLEAN;
}
class Value;
template < >
inline ::GType type_to_gtypeid<const Value*>() {
  return G_TYPE_VALUE;
}

template < >
inline ::GType type_to_gtypeid< ::uint32_t>() {
  // REVISIT (seanparent) : There currently isn't any G_TYPE_UINT32, this code
  // assumes sizeof(guint) == sizeof(guint32). Need a static_assert to assert
  // that.
  return G_TYPE_UINT;
}

template < >
inline ::GType type_to_gtypeid< ::int64_t>() {
  return G_TYPE_INT64;
}

template < >
inline ::GType type_to_gtypeid< ::int32_t>() {
  return G_TYPE_INT;
}

// \brief Value (and Retrieve) support using std::string as well as const char*
// by promoting from const char* to the string. promote_from provides a mapping
// for this promotion (and possibly others in the future).

template <typename T> struct promotes_from {
  typedef T type;
};
template < > struct promotes_from<std::string> {
  typedef const char* type;
};

// \brief RawCast converts from a GValue to a value of a canonical type.
//
// RawCast is a low level function. Generally, use Cast() instead.
//
// \precondition \param x contains a value of type \param T.

template <typename T>
inline T RawCast(const ::GValue& x) {
  // Use static_assert() to issue a meaningful compile-time error.
  // To prevent this from happening for all references to RawCast, use sizeof(T)
  // to make static_assert depend on type T and therefore prevent binding it
  // unconditionally until the actual RawCast<T> instantiation happens.
  static_assert(sizeof(T) == 0, "Using RawCast on unsupported type");
  return T();
}

template < >
inline const char* RawCast<const char*>(const ::GValue& x) {
  return static_cast<const char*>(::g_value_get_string(&x));
}
template < >
inline double RawCast<double>(const ::GValue& x) {
  return static_cast<double>(::g_value_get_double(&x));
}
template < >
inline bool RawCast<bool>(const ::GValue& x) {
  return static_cast<bool>(::g_value_get_boolean(&x));
}
template < >
inline ::uint32_t RawCast< ::uint32_t>(const ::GValue& x) {
  return static_cast< ::uint32_t>(::g_value_get_uint(&x));
}
template < >
inline ::uint8_t RawCast< ::uint8_t>(const ::GValue& x) {
  return static_cast< ::uint8_t>(::g_value_get_uchar(&x));
}
template < >
inline ::int64_t RawCast< ::int64_t>(const ::GValue& x) {
  return static_cast< ::int64_t>(::g_value_get_int64(&x));
}
template < >
inline ::int32_t RawCast< ::int32_t>(const ::GValue& x) {
  return static_cast< ::int32_t>(::g_value_get_int(&x));
}

inline void RawSet(GValue* x, const std::string& v) {
  ::g_value_set_string(x, v.c_str());
}
inline void RawSet(GValue* x, const char* v) {
  ::g_value_set_string(x, v);
}
inline void RawSet(GValue* x, double v) {
  ::g_value_set_double(x, v);
}
inline void RawSet(GValue* x, bool v) {
  ::g_value_set_boolean(x, v);
}
inline void RawSet(GValue* x, ::uint32_t v) {
  ::g_value_set_uint(x, v);
}
inline void RawSet(GValue* x, ::uint8_t v) {
  ::g_value_set_uchar(x, v);
}
inline void RawSet(GValue* x, ::int64_t v) {
  ::g_value_set_int64(x, v);
}
inline void RawSet(GValue* x, ::int32_t v) {
  ::g_value_set_int(x, v);
}

// \brief Value is a data type for managing GValues.
//
// A Value is a polymorphic container holding at most a single value.
//
// The Value wrapper ensures proper initialization, copies, and assignment of
// GValues.
//
// \note GValues are equationally incomplete and so can't support proper
// equality. The semantics of copy are verified with equality of retrieved
// values.

class Value : public ::GValue {
 public:
  Value()
      : GValue() {
  }
  explicit Value(const ::GValue& x)
      : GValue() {
    *this = *static_cast<const Value*>(&x);
  }
  template <typename T>
  explicit Value(T x)
      : GValue() {
    ::g_value_init(this,
        type_to_gtypeid<typename promotes_from<T>::type>());
    RawSet(this, x);
  }
  Value(const Value& x)
      : GValue() {
    if (x.empty())
      return;
    ::g_value_init(this, G_VALUE_TYPE(&x));
    ::g_value_copy(&x, this);
  }
  ~Value() {
    clear();
  }
  Value& operator=(const Value& x) {
    if (this == &x)
      return *this;
    clear();
    if (x.empty())
      return *this;
    ::g_value_init(this, G_VALUE_TYPE(&x));
    ::g_value_copy(&x, this);
    return *this;
  }
  template <typename T>
  Value& operator=(const T& x) {
    clear();
    ::g_value_init(this,
                   type_to_gtypeid<typename promotes_from<T>::type>());
    RawSet(this, x);
    return *this;
  }

  // Lower-case names to follow STL container conventions.

  void clear() {
    if (!empty())
      ::g_value_unset(this);
  }

  bool empty() const {
    return G_VALUE_TYPE(this) == G_TYPE_INVALID;
  }
};

template < >
inline const Value* RawCast<const Value*>(const ::GValue& x) {
  return static_cast<const Value*>(&x);
}

// \brief Retrieve gets a value from a GValue.
//
// \postcondition If \param x contains a value of type \param T, then the
//  value is copied to \param result and \true is returned. Otherwise, \param
//  result is unchanged and \false is returned.
//
// \precondition \param result is not \nullptr.

template <typename T>
bool Retrieve(const ::GValue& x, T* result) {
  if (!G_VALUE_HOLDS(&x, type_to_gtypeid<typename promotes_from<T>::type>())) {
    LOG(WARNING) << "GValue retrieve failed. Expected: "
        << g_type_name(type_to_gtypeid<typename promotes_from<T>::type>())
        << ", Found: " << g_type_name(G_VALUE_TYPE(&x));
    return false;
  }

  *result = RawCast<typename promotes_from<T>::type>(x);
  return true;
}

inline bool Retrieve(const ::GValue& x, Value* result) {
  *result = Value(x);
  return true;
}

// \brief ScopedError holds a ::GError* and deletes it on destruction.

struct FreeError {
  void operator()(::GError* x) const {
    if (x)
      ::g_error_free(x);
  }
};

typedef ::scoped_ptr< ::GError, FreeError> ScopedError;

// \brief ScopedArray holds a ::GArray* and deletes both the container and the
// segment containing the elements on destruction.

struct FreeArray {
  void operator()(::GArray* x) const {
    if (x)
      ::g_array_free(x, TRUE);
  }
};

typedef ::scoped_ptr< ::GArray, FreeArray> ScopedArray;

// \brief ScopedPtrArray adapts ::GPtrArray* to conform to the standard
//  container requirements.
//
// \note ScopedPtrArray is only partially implemented and is being fleshed out
//  as needed.
//
// \models Random Access Container, Back Insertion Sequence, ScopedPtrArray is
//  not copyable and equationally incomplete.

template <typename T>  // T models pointer
class ScopedPtrArray {
 public:
  typedef ::GPtrArray element_type;

  typedef T value_type;
  typedef const value_type& const_reference;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;

  ScopedPtrArray()
      : object_(0) {
  }

  explicit ScopedPtrArray(::GPtrArray* x)
      : object_(x) {
  }

  ~ScopedPtrArray() {
    clear();
  }

  iterator begin() {
    return iterator(object_ ? object_->pdata : nullptr);
  }
  iterator end() {
    return begin() + size();
  }
  const_iterator begin() const {
    return const_iterator(object_ ? object_->pdata : nullptr);
  }
  const_iterator end() const {
    return begin() + size();
  }

  // \precondition x is a pointer to an object allocated with g_new().

  void push_back(T x) {
    if (!object_)
      object_ = ::g_ptr_array_sized_new(1);
    ::g_ptr_array_add(object_, ::gpointer(x));
  }

  T& operator[](std::size_t n) {
    DCHECK(!(size() < n)) << "ScopedPtrArray index out-of-bound.";
    return *(begin() + n);
  }

  std::size_t size() const {
    return object_ ? object_->len : 0;
  }

  void clear() {
    if (object_) {
      std::for_each(begin(), end(), FreeHelper());
      ::g_ptr_array_free(object_, true);
      object_ = nullptr;
    }
  }

  void reset(::GPtrArray* p = nullptr) {
    if (p != object_) {
      clear();
      object_ = p;
    }
  }

 private:
  struct FreeHelper {
    void operator()(T x) const {
      ::g_free(::gpointer(x));
    }
  };

  template <typename U>
  friend void swap(ScopedPtrArray<U>& x, ScopedPtrArray<U>& y);

  ::GPtrArray* object_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPtrArray);
};

template <typename U>
inline void swap(ScopedPtrArray<U>& x, ScopedPtrArray<U>& y) {
  std::swap(x.object_, y.object_);
}

// \brief ScopedHashTable manages the lifetime of a ::GHashTable* with an
// interface compatibitle with a scoped ptr.
//
// The ScopedHashTable is also the start of an adaptor to model a standard
// Container. The standard for an associative container would have an iterator
// returning a key value pair. However, that isn't possible with
// ::GHashTable because there is no interface returning a reference to the
// key value pair, only to retrieve the keys and values and individual elements.
//
// So the standard interface of find() wouldn't work. I considered implementing
// operator[] and count() - operator []. So retrieving a value would look like:
//
// if (table.count(key))
//   success = Retrieve(table[key], &value);
//
// But that requires hashing the key twice.
// For now I implemented a Retrieve member function to follow the pattern
// developed elsewhere in the code.
//
// bool success = Retrieve(key, &x);
//
// This is also a template to retrieve the corect type from the stored GValue
// type.
//
// I may revisit this and use scoped_ptr_malloc and a non-member function
// Retrieve() in the future. The Retrieve pattern is becoming common enough
// that I want to give some thought as to how to generalize it further.

class ScopedHashTable {
 public:
  typedef ::GHashTable element_type;

  ScopedHashTable()
      : object_(nullptr) {
  }

  explicit ScopedHashTable(::GHashTable* p)
      : object_(p) {
  }

  ~ScopedHashTable() {
    clear();
  }

  template <typename T>
  bool Retrieve(const char* key, T* result) const {
    DCHECK(object_) << "Retrieve on empty ScopedHashTable.";
    if (!object_)
      return false;

    ::gpointer ptr = ::g_hash_table_lookup(object_, key);
    if (!ptr)
      return false;
    return glib::Retrieve(*static_cast< ::GValue*>(ptr), result);
  }

  void clear() {
    if (object_) {
      ::g_hash_table_unref(object_);
      object_ = nullptr;
    }
  }

  GHashTable* get() {
    return object_;
  }

  void reset(::GHashTable* p = nullptr) {
    if (p != object_) {
      clear();
      object_ = p;
    }
  }

 private:
  ::GHashTable* object_;
};

}  // namespace glib
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_GLIB_OBJECT_H_
