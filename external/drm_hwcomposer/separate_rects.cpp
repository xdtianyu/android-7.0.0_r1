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

#include "separate_rects.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace separate_rects {

enum EventType { START, END };

template <typename TId, typename TNum>
struct StartedRect {
  IdSet<TId> id_set;
  TNum left, top, bottom;

  // Note that this->left is not part of the key. That field is only to mark the
  // left edge of the rectangle.
  bool operator<(const StartedRect<TId, TNum> &rhs) const {
    return (top < rhs.top || (top == rhs.top && bottom < rhs.bottom)) ||
           (top == rhs.top && bottom == rhs.bottom && id_set < rhs.id_set);
  }
};

template <typename TId, typename TNum>
struct SweepEvent {
  EventType type;
  union {
    TNum x;
    TNum y;
  };

  TId rect_id;

  bool operator<(const SweepEvent<TId, TNum> &rhs) const {
    return (y < rhs.y || (y == rhs.y && rect_id < rhs.rect_id));
  }
};

template <typename TNum>
std::ostream &operator<<(std::ostream &os, const Rect<TNum> &rect) {
  return os << rect.bounds[0] << ", " << rect.bounds[1] << ", "
            << rect.bounds[2] << ", " << rect.bounds[3];
}

template <typename TUInt>
std::ostream &operator<<(std::ostream &os, const IdSet<TUInt> &obj) {
  int bits = IdSet<TUInt>::max_elements;
  TUInt mask = ((TUInt)0x1) << (bits - 1);
  for (int i = 0; i < bits; i++)
    os << ((obj.getBits() & (mask >> i)) ? "1" : "0");
  return os;
}

template <typename TNum, typename TId>
void separate_rects(const std::vector<Rect<TNum>> &in,
                    std::vector<RectSet<TId, TNum>> *out) {
  // Overview:
  // This algorithm is a line sweep algorithm that travels from left to right.
  // The sweep stops at each vertical edge of each input rectangle in sorted
  // order of x-coordinate. At each stop, the sweep line is examined in order of
  // y-coordinate from top to bottom. Along the way, a running set of rectangle
  // IDs is either added to or subtracted from as the top and bottom edges are
  // encountered, respectively. At each change of that running set, a copy of
  // that set is recorded in along with the the y-coordinate it happened at in a
  // list. This list is then interpreted as a sort of vertical cross section of
  // our output set of non-overlapping rectangles. Based of the algorithm found
  // at: http://stackoverflow.com/a/2755498

  if (in.size() > IdSet<TNum>::max_elements) {
    return;
  }

  // Events are when the sweep line encounters the starting or ending edge of
  // any input rectangle.
  std::set<SweepEvent<TId, TNum>> sweep_h_events;  // Left or right bounds
  std::set<SweepEvent<TId, TNum>> sweep_v_events;  // Top or bottom bounds

  // A started rect is a rectangle whose left, top, bottom edge, and set of
  // rectangle IDs is known. The key of this map includes all that information
  // (except the left edge is never used to determine key equivalence or
  // ordering),
  std::map<StartedRect<TId, TNum>, bool> started_rects;

  // This is cleared after every event. Its declaration is here to avoid
  // reallocating a vector and its buffers every event.
  std::vector<std::pair<TNum, IdSet<TId>>> active_regions;

  // This pass will add rectangle start and end events to be triggered as the
  // algorithm sweeps from left to right.
  for (TId i = 0; i < in.size(); i++) {
    const Rect<TNum> &rect = in[i];

    // Filter out empty or invalid rects.
    if (rect.left >= rect.right || rect.top >= rect.bottom)
      continue;

    SweepEvent<TId, TNum> evt;
    evt.rect_id = i;

    evt.type = START;
    evt.x = rect.left;
    sweep_h_events.insert(evt);

    evt.type = END;
    evt.x = rect.right;
    sweep_h_events.insert(evt);
  }

  for (typename std::set<SweepEvent<TId, TNum>>::iterator it =
           sweep_h_events.begin();
       it != sweep_h_events.end(); ++it) {
    const SweepEvent<TId, TNum> &h_evt = *it;
    const Rect<TNum> &rect = in[h_evt.rect_id];

    // During this event, we have encountered a vertical starting or ending edge
    // of a rectangle so want to append or remove (respectively) that rectangles
    // top and bottom from the vertical sweep line.
    SweepEvent<TId, TNum> v_evt;
    v_evt.rect_id = h_evt.rect_id;
    if (h_evt.type == START) {
      v_evt.type = START;
      v_evt.y = rect.top;
      sweep_v_events.insert(v_evt);

      v_evt.type = END;
      v_evt.y = rect.bottom;
      sweep_v_events.insert(v_evt);
    } else {
      v_evt.type = START;
      v_evt.y = rect.top;
      typename std::set<SweepEvent<TId, TNum>>::iterator start_it =
          sweep_v_events.find(v_evt);
      assert(start_it != sweep_v_events.end());
      sweep_v_events.erase(start_it);

      v_evt.type = END;
      v_evt.y = rect.bottom;
      typename std::set<SweepEvent<TId, TNum>>::iterator end_it =
          sweep_v_events.find(v_evt);
      assert(end_it != sweep_v_events.end());
      sweep_v_events.erase(end_it);
    }

    // Peeks ahead to see if there are other rectangles sharing a vertical edge
    // with the current sweep line. If so, we want to continue marking up the
    // sweep line before actually processing the rectangles the sweep line is
    // intersecting.
    typename std::set<SweepEvent<TId, TNum>>::iterator next_it = it;
    ++next_it;
    if (next_it != sweep_h_events.end()) {
      if (next_it->x == h_evt.x) {
        continue;
      }
    }

#ifdef RECTS_DEBUG
    std::cout << h_evt.x << std::endl;
#endif

    // After the following for loop, active_regions will be a list of
    // y-coordinates paired with the set of rectangle IDs that are intersect at
    // that y-coordinate (and the current sweep line's x-coordinate). For
    // example if the current sweep line were the left edge of a scene with only
    // one rectangle of ID 0 and bounds (left, top, right, bottom) == (2, 3, 4,
    // 5), active_regions will be [({ 0 }, 3), {}, 5].
    active_regions.clear();
    IdSet<TId> active_set;
    for (typename std::set<SweepEvent<TId, TNum>>::iterator it =
             sweep_v_events.begin();
         it != sweep_v_events.end(); ++it) {
      const SweepEvent<TId, TNum> &v_evt = *it;

      if (v_evt.type == START) {
        active_set.add(v_evt.rect_id);
      } else {
        active_set.subtract(v_evt.rect_id);
      }

      if (active_regions.size() > 0 && active_regions.back().first == v_evt.y) {
        active_regions.back().second = active_set;
      } else {
        active_regions.push_back(std::make_pair(v_evt.y, active_set));
      }
    }

#ifdef RECTS_DEBUG
    std::cout << "x:" << h_evt.x;
    for (std::vector<std::pair<TNum, IdSet>>::iterator it =
             active_regions.begin();
         it != active_regions.end(); ++it) {
      std::cout << " " << it->first << "(" << it->second << ")"
                << ",";
    }
    std::cout << std::endl;
#endif

    // To determine which started rectangles are ending this event, we make them
    // all as false, or unseen during this sweep line.
    for (typename std::map<StartedRect<TId, TNum>, bool>::iterator it =
             started_rects.begin();
         it != started_rects.end(); ++it) {
      it->second = false;
    }

    // This for loop will iterate all potential new rectangles and either
    // discover it was already started (and then mark it true), or that it is a
    // new rectangle and add it to the started rectangles. A started rectangle
    // is unique if it has a distinct top, bottom, and set of rectangle IDs.
    // This is tricky because a potential rectangle could be encountered here
    // that has a non-unique top and bottom, so it shares geometry with an
    // already started rectangle, but the set of rectangle IDs differs. In that
    // case, we have a new rectangle, and the already existing started rectangle
    // will not be marked as seen ("true" in the std::pair) and will get ended
    // by the for loop after this one. This is as intended.
    for (typename std::vector<std::pair<TNum, IdSet<TId>>>::iterator it =
             active_regions.begin();
         it != active_regions.end(); ++it) {
      IdSet<TId> region_set = it->second;

      if (region_set.isEmpty())
        continue;

      // An important property of active_regions is that each region where a set
      // of rectangles applies is bounded at the bottom by the next (in the
      // vector) region's starting y-coordinate.
      typename std::vector<std::pair<TNum, IdSet<TId>>>::iterator next_it = it;
      ++next_it;
      assert(next_it != active_regions.end());

      TNum region_top = it->first;
      TNum region_bottom = next_it->first;

      StartedRect<TId, TNum> rect_key;
      rect_key.id_set = region_set;
      rect_key.left = h_evt.x;
      rect_key.top = region_top;
      rect_key.bottom = region_bottom;

      // Remember that rect_key.left is ignored for the purposes of searching
      // the started rects. This follows from the fact that a previously started
      // rectangle would by definition have a left bound less than the current
      // event's x-coordinate. We are interested in continuing the started
      // rectangles by marking them seen (true) but we don't know, care, or wish
      // to change the left bound at this point. If there are no matching
      // rectangles for this region, start a new one and mark it as seen (true).
      typename std::map<StartedRect<TId, TNum>, bool>::iterator
          started_rect_it = started_rects.find(rect_key);
      if (started_rect_it == started_rects.end()) {
        started_rects[rect_key] = true;
      } else {
        started_rect_it->second = true;
      }
    }

    // This for loop ends all rectangles that were unseen during this event.
    // Because this is the first event where we didn't see this rectangle, it's
    // right edge is exactly the current event's x-coordinate. With this, we
    // have the final piece of information to output this rectangle's geometry
    // and set of input rectangle IDs. To end a started rectangle, we erase it
    // from the started_rects map and append the completed rectangle to the
    // output vector.
    for (typename std::map<StartedRect<TId, TNum>, bool>::iterator it =
             started_rects.begin();
         it != started_rects.end();
         /* inc in body */) {
      if (!it->second) {
        const StartedRect<TId, TNum> &proto_rect = it->first;
        Rect<TNum> out_rect;
        out_rect.left = proto_rect.left;
        out_rect.top = proto_rect.top;
        out_rect.right = h_evt.x;
        out_rect.bottom = proto_rect.bottom;
        out->push_back(RectSet<TId, TNum>(proto_rect.id_set, out_rect));
        started_rects.erase(it++);  // Also increments out iterator.

#ifdef RECTS_DEBUG
        std::cout << "    <" << proto_rect.id_set << "(" << rect << ")"
                  << std::endl;
#endif
      } else {
        // Remember this for loop has no built in increment step. We do it here.
        ++it;
      }
    }
  }
}

void separate_frects_64(const std::vector<Rect<float>> &in,
                        std::vector<RectSet<uint64_t, float>> *out) {
  separate_rects(in, out);
}

void separate_rects_64(const std::vector<Rect<int>> &in,
                       std::vector<RectSet<uint64_t, int>> *out) {
  separate_rects(in, out);
}

}  // namespace separate_rects

#ifdef RECTS_TEST

using namespace separate_rects;

int main(int argc, char **argv) {
#define RectSet RectSet<TId, TNum>
#define Rect Rect<TNum>
#define IdSet IdSet<TId>
  typedef uint64_t TId;
  typedef float TNum;

  std::vector<Rect> in;
  std::vector<RectSet> out;
  std::vector<RectSet> expected_out;

  in.push_back({0, 0, 4, 5});
  in.push_back({2, 0, 6, 6});
  in.push_back({4, 0, 8, 5});
  in.push_back({0, 7, 8, 9});

  in.push_back({10, 0, 18, 5});
  in.push_back({12, 0, 16, 5});

  in.push_back({20, 11, 24, 17});
  in.push_back({22, 13, 26, 21});
  in.push_back({32, 33, 36, 37});
  in.push_back({30, 31, 38, 39});

  in.push_back({40, 43, 48, 45});
  in.push_back({44, 41, 46, 47});

  in.push_back({50, 51, 52, 53});
  in.push_back({50, 51, 52, 53});
  in.push_back({50, 51, 52, 53});

  in.push_back({0, 0, 0, 10});
  in.push_back({0, 0, 10, 0});
  in.push_back({10, 0, 0, 10});
  in.push_back({0, 10, 10, 0});

  for (int i = 0; i < 100000; i++) {
    out.clear();
    separate_rects(in, &out);
  }

  for (int i = 0; i < out.size(); i++) {
    std::cout << out[i].id_set << "(" << out[i].rect << ")" << std::endl;
  }

  std::cout << "# of rects: " << out.size() << std::endl;

  expected_out.push_back(RectSet(IdSet(0), Rect(0, 0, 2, 5)));
  expected_out.push_back(RectSet(IdSet(1), Rect(2, 5, 6, 6)));
  expected_out.push_back(RectSet(IdSet(1) | 0, Rect(2, 0, 4, 5)));
  expected_out.push_back(RectSet(IdSet(1) | 2, Rect(4, 0, 6, 5)));
  expected_out.push_back(RectSet(IdSet(2), Rect(6, 0, 8, 5)));
  expected_out.push_back(RectSet(IdSet(3), Rect(0, 7, 8, 9)));
  expected_out.push_back(RectSet(IdSet(4), Rect(10, 0, 12, 5)));
  expected_out.push_back(RectSet(IdSet(5) | 4, Rect(12, 0, 16, 5)));
  expected_out.push_back(RectSet(IdSet(4), Rect(16, 0, 18, 5)));
  expected_out.push_back(RectSet(IdSet(6), Rect(20, 11, 22, 17)));
  expected_out.push_back(RectSet(IdSet(6) | 7, Rect(22, 13, 24, 17)));
  expected_out.push_back(RectSet(IdSet(6), Rect(22, 11, 24, 13)));
  expected_out.push_back(RectSet(IdSet(7), Rect(22, 17, 24, 21)));
  expected_out.push_back(RectSet(IdSet(7), Rect(24, 13, 26, 21)));
  expected_out.push_back(RectSet(IdSet(9), Rect(30, 31, 32, 39)));
  expected_out.push_back(RectSet(IdSet(8) | 9, Rect(32, 33, 36, 37)));
  expected_out.push_back(RectSet(IdSet(9), Rect(32, 37, 36, 39)));
  expected_out.push_back(RectSet(IdSet(9), Rect(32, 31, 36, 33)));
  expected_out.push_back(RectSet(IdSet(9), Rect(36, 31, 38, 39)));
  expected_out.push_back(RectSet(IdSet(10), Rect(40, 43, 44, 45)));
  expected_out.push_back(RectSet(IdSet(10) | 11, Rect(44, 43, 46, 45)));
  expected_out.push_back(RectSet(IdSet(11), Rect(44, 41, 46, 43)));
  expected_out.push_back(RectSet(IdSet(11), Rect(44, 45, 46, 47)));
  expected_out.push_back(RectSet(IdSet(10), Rect(46, 43, 48, 45)));
  expected_out.push_back(RectSet(IdSet(12) | 13 | 14, Rect(50, 51, 52, 53)));

  for (int i = 0; i < expected_out.size(); i++) {
    RectSet &ex_out = expected_out[i];
    if (std::find(out.begin(), out.end(), ex_out) == out.end()) {
      std::cout << "Missing Rect: " << ex_out.id_set << "(" << ex_out.rect
                << ")" << std::endl;
    }
  }

  for (int i = 0; i < out.size(); i++) {
    RectSet &actual_out = out[i];
    if (std::find(expected_out.begin(), expected_out.end(), actual_out) ==
        expected_out.end()) {
      std::cout << "Extra Rect: " << actual_out.id_set << "(" << actual_out.rect
                << ")" << std::endl;
    }
  }

  return 0;
}

#endif
