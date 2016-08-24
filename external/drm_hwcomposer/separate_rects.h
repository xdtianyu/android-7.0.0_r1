/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DRM_HWCOMPOSER_SEPARATE_RECTS_H_
#define DRM_HWCOMPOSER_SEPARATE_RECTS_H_

#include <stdint.h>

#include <sstream>
#include <vector>

namespace separate_rects {

template <typename TFloat>
struct Rect {
  union {
    struct {
      TFloat left, top, right, bottom;
    };
    struct {
      TFloat x1, y1, x2, y2;
    };
    TFloat bounds[4];
  };

  typedef TFloat TNum;

  Rect() {
  }

  Rect(TFloat xx1, TFloat yy1, TFloat xx2, TFloat yy2)
      : x1(xx1), y1(yy1), x2(xx2), y2(yy2) {
  }

  template <typename T>
  Rect(const Rect<T> &rhs) {
    for (int i = 0; i < 4; i++)
      bounds[i] = rhs.bounds[i];
  }

  template <typename T>
  Rect<TFloat> &operator=(const Rect<T> &rhs) {
    for (int i = 0; i < 4; i++)
      bounds[i] = rhs.bounds[i];
    return *this;
  }

  bool operator==(const Rect &rhs) const {
    for (int i = 0; i < 4; i++) {
      if (bounds[i] != rhs.bounds[i])
        return false;
    }

    return true;
  }

  TFloat width() const {
    return bounds[2] - bounds[0];
  }

  TFloat height() const {
    return bounds[3] - bounds[1];
  }

  TFloat area() const {
    return width() * height();
  }

  void Dump(std::ostringstream *out) const {
    *out << "[x/y/w/h]=" << left << "/" << top << "/" << width() << "/"
         << height();
  }
};

template <typename TUInt>
struct IdSet {
 public:
  typedef TUInt TId;

  IdSet() : bitset(0) {
  }

  IdSet(TId id) : bitset(0) {
    add(id);
  }

  void add(TId id) {
    bitset |= ((TUInt)1) << id;
  }

  void subtract(TId id) {
    bitset &= ~(((TUInt)1) << id);
  }

  bool isEmpty() const {
    return bitset == 0;
  }

  TUInt getBits() const {
    return bitset;
  }

  bool operator==(const IdSet<TId> &rhs) const {
    return bitset == rhs.bitset;
  }

  bool operator<(const IdSet<TId> &rhs) const {
    return bitset < rhs.bitset;
  }

  IdSet<TId> operator|(const IdSet<TId> &rhs) const {
    IdSet ret;
    ret.bitset = bitset | rhs.bitset;
    return ret;
  }

  IdSet<TId> operator|(TId id) const {
    IdSet<TId> ret;
    ret.bitset = bitset;
    ret.add(id);
    return ret;
  }

  static const int max_elements = sizeof(TId) * 8;

 private:
  TUInt bitset;
};

template <typename TId, typename TNum>
struct RectSet {
  IdSet<TId> id_set;
  Rect<TNum> rect;

  RectSet(const IdSet<TId> &i, const Rect<TNum> &r) : id_set(i), rect(r) {
  }

  bool operator==(const RectSet<TId, TNum> &rhs) const {
    return id_set == rhs.id_set && rect == rhs.rect;
  }
};

// Separates up to a maximum of 64 input rectangles into mutually non-
// overlapping rectangles that cover the exact same area and outputs those non-
// overlapping rectangles. Each output rectangle also includes the set of input
// rectangle indices that overlap the output rectangle encoded in a bitset. For
// example, an output rectangle that overlaps input rectangles in[0], in[1], and
// in[4], the bitset would be (ommitting leading zeroes) 10011.
void separate_frects_64(const std::vector<Rect<float>> &in,
                        std::vector<RectSet<uint64_t, float>> *out);
void separate_rects_64(const std::vector<Rect<int>> &in,
                       std::vector<RectSet<uint64_t, int>> *out);

}  // namespace separate_rects

#endif
