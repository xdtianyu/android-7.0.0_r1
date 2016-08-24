# Copyright 2015 The Android Open Source Project
#
LOCAL_PATH := $(call my-dir)

lzma_files := \
  7zAlloc.c \
  7zArcIn.c \
  7zBuf2.c \
  7zBuf.c \
  7zCrc.c \
  7zCrcOpt.c \
  7zDec.c \
  7zFile.c \
  7zStream.c \
  Aes.c \
  AesOpt.c \
  Alloc.c \
  Bcj2.c \
  Bra86.c \
  Bra.c \
  BraIA64.c \
  CpuArch.c \
  Delta.c \
  LzFind.c \
  Lzma2Dec.c \
  Lzma2Enc.c \
  Lzma86Dec.c \
  Lzma86Enc.c \
  LzmaDec.c \
  LzmaEnc.c \
  LzmaLib.c \
  Ppmd7.c \
  Ppmd7Dec.c \
  Ppmd7Enc.c \
  Sha256.c \
  Sort.c \
  Xz.c \
  XzCrc64.c \
  XzCrc64Opt.c \
  XzDec.c \
  XzEnc.c \
  XzIn.c

lzma_cflags := -D_7ZIP_ST -Wno-empty-body
lzma_clang_cflags := -Wno-self-assign

include $(CLEAR_VARS)
LOCAL_MODULE := liblzma
LOCAL_CFLAGS := $(lzma_cflags)
LOCAL_CLANG_CFLAGS := $(lzma_clang_cflags)
LOCAL_SRC_FILES := $(lzma_files)
LOCAL_MULTILIB := both
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liblzma
LOCAL_CFLAGS := $(lzma_cflags)
LOCAL_CLANG_CFLAGS := $(lzma_clang_cflags)
LOCAL_SRC_FILES := $(lzma_files)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liblzma
LOCAL_CFLAGS := $(lzma_cflags)
LOCAL_CLANG_CFLAGS := $(lzma_clang_cflags)
LOCAL_SRC_FILES := $(lzma_files)
LOCAL_MULTILIB := both
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liblzma
LOCAL_CFLAGS := $(lzma_cflags)
LOCAL_CLANG_CFLAGS := $(lzma_clang_cflags)
LOCAL_SRC_FILES := $(lzma_files)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)
