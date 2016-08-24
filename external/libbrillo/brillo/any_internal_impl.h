// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Internal implementation of brillo::Any class.

#ifndef LIBBRILLO_BRILLO_ANY_INTERNAL_IMPL_H_
#define LIBBRILLO_BRILLO_ANY_INTERNAL_IMPL_H_

#include <type_traits>
#include <typeinfo>
#include <utility>

#include <base/logging.h>
#include <brillo/dbus/data_serialization.h>
#include <brillo/type_name_undecorate.h>

namespace brillo {

namespace internal_details {

// An extension to std::is_convertible to allow conversion from an enum to
// an integral type which std::is_convertible does not indicate as supported.
template <typename From, typename To>
struct IsConvertible
    : public std::integral_constant<
          bool,
          std::is_convertible<From, To>::value ||
              (std::is_enum<From>::value && std::is_integral<To>::value)> {};

// TryConvert is a helper function that does a safe compile-time conditional
// type cast between data types that may not be always convertible.
// From and To are the source and destination types.
// The function returns true if conversion was possible/successful.
template <typename From, typename To>
inline typename std::enable_if<IsConvertible<From, To>::value, bool>::type
TryConvert(const From& in, To* out) {
  *out = static_cast<To>(in);
  return true;
}
template <typename From, typename To>
inline typename std::enable_if<!IsConvertible<From, To>::value, bool>::type
TryConvert(const From& /* in */, To* /* out */) {
  return false;
}

//////////////////////////////////////////////////////////////////////////////
// Provide a way to compare values of unspecified types without compiler errors
// when no operator==() is provided for a given type. This is important to
// allow Any class to have operator==(), yet still allowing arbitrary types
// (not necessarily comparable) to be placed inside Any without resulting in
// compile-time error.
//
// We achieve this in two ways. First, we provide a IsEqualityComparable<T>
// class that can be used in compile-time conditions to determine if there is
// operator==() defined that takes values of type T (or which can be implicitly
// converted to type T). Secondly, this allows us to specialize a helper
// compare function EqCompare<T>(v1, v2) to use operator==() for types that
// are comparable, and just return false for those that are not.
//
// IsEqualityComparableHelper<T> is a helper class for implementing an
// an STL-compatible IsEqualityComparable<T> containing a Boolean member |value|
// which evaluates to true for comparable types and false otherwise.
template<typename T>
struct IsEqualityComparableHelper {
  struct IntWrapper {
    // A special structure that provides a constructor that takes an int.
    // This way, an int argument passed to a function will be favored over
    // IntWrapper when both overloads are provided.
    // Also this constructor must NOT be explicit.
    // NOLINTNEXTLINE(runtime/explicit)
    IntWrapper(int /* dummy */) {}  // do nothing
  };

  // Here is an obscure trick to determine if a type U has operator==().
  // We are providing two function prototypes for TriggerFunction. One that
  // takes an argument of type IntWrapper (which is implicitly convertible from
  // an int), and returns an std::false_type. This is a fall-back mechanism.
  template<typename U>
  static std::false_type TriggerFunction(IntWrapper dummy);

  // The second overload of TriggerFunction takes an int (explicitly) and
  // returns std::true_type. If both overloads are available, this one will be
  // chosen when referencing it as TriggerFunction(0), since it is a better
  // (more specific) match.
  //
  // However this overload is available only for types that support operator==.
  // This is achieved by employing SFINAE mechanism inside a template function
  // overload that refers to operator==() for two values of types U&. This is
  // used inside decltype(), so no actual code is executed. If the types
  // are not comparable, reference to "==" would fail and the compiler will
  // simply ignore this overload due to SFIANE.
  //
  // The final little trick used here is the reliance on operator comma inside
  // the decltype() expression. The result of the expression is always
  // std::true_type(). The expression on the left of comma is just evaluated and
  // discarded. If it evaluates successfully (i.e. the type has operator==), the
  // return value of the function is set to be std::true_value. If it fails,
  // the whole function prototype is discarded and is not available in the
  // IsEqualityComparableHelper<T> class.
  //
  // Here we use std::declval<U&>() to make sure we have operator==() that takes
  // lvalue references to type U which is not necessarily default-constructible.
  template<typename U>
  static decltype((std::declval<U&>() == std::declval<U&>()), std::true_type())
  TriggerFunction(int dummy);

  // Finally, use the return type of the overload of TriggerFunction that
  // matches the argument (int) to be aliased to type |type|. If T is
  // comparable, there will be two overloads and the more specific (int) will
  // be chosen which returns std::true_value. If the type is non-comparable,
  // there will be only one version of TriggerFunction available which
  // returns std::false_value.
  using type = decltype(TriggerFunction<T>(0));
};

// IsEqualityComparable<T> is simply a class that derives from either
// std::true_value, if type T is comparable, or from std::false_value, if the
// type is non-comparable. We just use |type| alias from
// IsEqualityComparableHelper<T> as the base class.
template<typename T>
struct IsEqualityComparable : IsEqualityComparableHelper<T>::type {};

// EqCompare() overload for non-comparable types. Always returns false.
template<typename T>
inline typename std::enable_if<!IsEqualityComparable<T>::value, bool>::type
EqCompare(const T& /* v1 */, const T& /* v2 */) {
  return false;
}

// EqCompare overload for comparable types. Calls operator==(v1, v2) to compare.
template<typename T>
inline typename std::enable_if<IsEqualityComparable<T>::value, bool>::type
EqCompare(const T& v1, const T& v2) {
  return (v1 == v2);
}

//////////////////////////////////////////////////////////////////////////////

class Buffer;  // Forward declaration of data buffer container.

// Abstract base class for contained variant data.
struct Data {
  virtual ~Data() {}
  // Returns the type tag (name) for the contained data.
  virtual const char* GetTypeTag() const = 0;
  // Copies the contained data to the output |buffer|.
  virtual void CopyTo(Buffer* buffer) const = 0;
  // Moves the contained data to the output |buffer|.
  virtual void MoveTo(Buffer* buffer) = 0;
  // Checks if the contained data is an integer type (not necessarily an 'int').
  virtual bool IsConvertibleToInteger() const = 0;
  // Gets the contained integral value as an integer.
  virtual intmax_t GetAsInteger() const = 0;
  // Writes the contained value to the D-Bus message buffer.
  virtual void AppendToDBusMessage(dbus::MessageWriter* writer) const = 0;
  // Compares if the two data containers have objects of the same value.
  virtual bool CompareEqual(const Data* other_data) const = 0;
};

// Concrete implementation of variant data of type T.
template<typename T>
struct TypedData : public Data {
  explicit TypedData(const T& value) : value_(value) {}
  // NOLINTNEXTLINE(build/c++11)
  explicit TypedData(T&& value) : value_(std::move(value)) {}

  const char* GetTypeTag() const override { return brillo::GetTypeTag<T>(); }
  void CopyTo(Buffer* buffer) const override;
  void MoveTo(Buffer* buffer) override;
  bool IsConvertibleToInteger() const override {
    return std::is_integral<T>::value || std::is_enum<T>::value;
  }
  intmax_t GetAsInteger() const override {
    intmax_t int_val = 0;
    bool converted = TryConvert(value_, &int_val);
    CHECK(converted) << "Unable to convert value of type '"
                     << GetUndecoratedTypeName<T>() << "' to integer";
    return int_val;
  }

  template<typename U>
  static typename std::enable_if<dbus_utils::IsTypeSupported<U>::value>::type
  AppendValueHelper(dbus::MessageWriter* writer, const U& value) {
    brillo::dbus_utils::AppendValueToWriterAsVariant(writer, value);
  }
  template<typename U>
  static typename std::enable_if<!dbus_utils::IsTypeSupported<U>::value>::type
  AppendValueHelper(dbus::MessageWriter* /* writer */, const U& /* value */) {
    LOG(FATAL) << "Type '" << GetUndecoratedTypeName<U>()
               << "' is not supported by D-Bus";
  }

  void AppendToDBusMessage(dbus::MessageWriter* writer) const override {
    return AppendValueHelper(writer, value_);
  }

  bool CompareEqual(const Data* other_data) const override {
    return EqCompare<T>(value_,
                        static_cast<const TypedData<T>*>(other_data)->value_);
  }

  // Special methods to copy/move data of the same type
  // without reallocating the buffer.
  void FastAssign(const T& source) { value_ = source; }
  // NOLINTNEXTLINE(build/c++11)
  void FastAssign(T&& source) { value_ = std::move(source); }

  T value_;
};

// Buffer class that stores the contained variant data.
// To improve performance and reduce memory fragmentation, small variants
// are stored in pre-allocated memory buffers that are part of the Any class.
// If the memory requirements are larger than the set limit or the type is
// non-trivially copyable, then the contained class is allocated in a separate
// memory block and the pointer to that memory is contained within this memory
// buffer class.
class Buffer final {
 public:
  enum StorageType { kExternal, kContained };
  Buffer() : external_ptr_(nullptr), storage_(kExternal) {}
  ~Buffer() { Clear(); }

  Buffer(const Buffer& rhs) : Buffer() { rhs.CopyTo(this); }
  // NOLINTNEXTLINE(build/c++11)
  Buffer(Buffer&& rhs) : Buffer() { rhs.MoveTo(this); }
  Buffer& operator=(const Buffer& rhs) {
    rhs.CopyTo(this);
    return *this;
  }
  // NOLINTNEXTLINE(build/c++11)
  Buffer& operator=(Buffer&& rhs) {
    rhs.MoveTo(this);
    return *this;
  }

  // Returns the underlying pointer to contained data. Uses either the pointer
  // or the raw data depending on |storage_| type.
  inline Data* GetDataPtr() {
    return (storage_ == kExternal) ? external_ptr_
                                   : reinterpret_cast<Data*>(contained_buffer_);
  }
  inline const Data* GetDataPtr() const {
    return (storage_ == kExternal)
               ? external_ptr_
               : reinterpret_cast<const Data*>(contained_buffer_);
  }

  // Destroys the contained object (and frees memory if needed).
  void Clear() {
    Data* data = GetDataPtr();
    if (storage_ == kExternal) {
      delete data;
    } else {
      // Call the destructor manually, since the object was constructed inline
      // in the pre-allocated buffer. We still need to call the destructor
      // to free any associated resources, but we can't call delete |data| here.
      data->~Data();
    }
    external_ptr_ = nullptr;
    storage_ = kExternal;
  }

  // Stores a value of type T.
  template<typename T>
  void Assign(T&& value) {  // NOLINT(build/c++11)
    using Type = typename std::decay<T>::type;
    using DataType = TypedData<Type>;
    Data* ptr = GetDataPtr();
    if (ptr && strcmp(ptr->GetTypeTag(), GetTypeTag<Type>()) == 0) {
      // We assign the data to the variant container, which already
      // has the data of the same type. Do fast copy/move with no memory
      // reallocation.
      DataType* typed_ptr = static_cast<DataType*>(ptr);
      // NOLINTNEXTLINE(build/c++11)
      typed_ptr->FastAssign(std::forward<T>(value));
    } else {
      Clear();
      // TODO(avakulenko): [see crbug.com/379833]
      // Unfortunately, GCC doesn't support std::is_trivially_copyable<T> yet,
      // so using std::is_trivial instead, which is a bit more restrictive.
      // Once GCC has support for is_trivially_copyable, update the following.
      if (!std::is_trivial<Type>::value ||
          sizeof(DataType) > sizeof(contained_buffer_)) {
        // If it is too big or not trivially copyable, allocate it separately.
        // NOLINTNEXTLINE(build/c++11)
        external_ptr_ = new DataType(std::forward<T>(value));
        storage_ = kExternal;
      } else {
        // Otherwise just use the pre-allocated buffer.
        DataType* address = reinterpret_cast<DataType*>(contained_buffer_);
        // Make sure we still call the copy/move constructor.
        // Call the constructor manually by using placement 'new'.
        // NOLINTNEXTLINE(build/c++11)
        new (address) DataType(std::forward<T>(value));
        storage_ = kContained;
      }
    }
  }

  // Helper methods to retrieve a reference to contained data.
  // These assume that type checking has already been performed by Any
  // so the type cast is valid and will succeed.
  template<typename T>
  const T& GetData() const {
    using DataType = internal_details::TypedData<typename std::decay<T>::type>;
    return static_cast<const DataType*>(GetDataPtr())->value_;
  }
  template<typename T>
  T& GetData() {
    using DataType = internal_details::TypedData<typename std::decay<T>::type>;
    return static_cast<DataType*>(GetDataPtr())->value_;
  }

  // Returns true if the buffer has no contained data.
  bool IsEmpty() const {
    return (storage_ == kExternal && external_ptr_ == nullptr);
  }

  // Copies the data from the current buffer into the |destination|.
  void CopyTo(Buffer* destination) const {
    if (IsEmpty()) {
      destination->Clear();
    } else {
      GetDataPtr()->CopyTo(destination);
    }
  }

  // Moves the data from the current buffer into the |destination|.
  void MoveTo(Buffer* destination) {
    if (IsEmpty()) {
      destination->Clear();
    } else {
      if (storage_ == kExternal) {
        destination->Clear();
        destination->storage_ = kExternal;
        destination->external_ptr_ = external_ptr_;
        external_ptr_ = nullptr;
      } else {
        GetDataPtr()->MoveTo(destination);
      }
    }
  }

  union {
    // |external_ptr_| is a pointer to a larger object allocated in
    // a separate memory block.
    Data* external_ptr_;
    // |contained_buffer_| is a pre-allocated buffer for smaller/simple objects.
    // Pre-allocate enough memory to store objects as big as "double".
    unsigned char contained_buffer_[sizeof(TypedData<double>)];
  };
  // Depending on a value of |storage_|, either |external_ptr_| or
  // |contained_buffer_| above is used to get a pointer to memory containing
  // the variant data.
  StorageType storage_;  // Declare after the union to eliminate member padding.
};

template <typename T>
void TypedData<T>::CopyTo(Buffer* buffer) const {
  buffer->Assign(value_);
}
template <typename T>
void TypedData<T>::MoveTo(Buffer* buffer) {
  buffer->Assign(std::move(value_));
}

}  // namespace internal_details

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_ANY_INTERNAL_IMPL_H_
