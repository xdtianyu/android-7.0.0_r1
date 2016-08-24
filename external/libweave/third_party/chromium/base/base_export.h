// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_BASE_EXPORT_H_
#define BASE_BASE_EXPORT_H_

#define BASE_EXPORT __attribute__((__visibility__("default")))
#define BASE_EXPORT_PRIVATE __attribute__((__visibility__("hidden")))

#endif  // BASE_BASE_EXPORT_H_
