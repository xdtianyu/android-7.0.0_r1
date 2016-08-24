LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

zopfli_files := \
	src/zopfli/blocksplitter.c src/zopfli/cache.c\
    src/zopfli/deflate.c src/zopfli/gzip_container.c\
    src/zopfli/hash.c src/zopfli/katajainen.c\
    src/zopfli/lz77.c src/zopfli/squeeze.c\
    src/zopfli/tree.c src/zopfli/util.c\
    src/zopfli/zlib_container.c src/zopfli/zopfli_lib.c

LOCAL_MODULE := libzopfli
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS += -O2
LOCAL_SRC_FILES := $(zopfli_files)
LOCAL_MULTILIB := both
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libzopfli
LOCAL_CFLAGS += -O2
LOCAL_SRC_FILES := $(zopfli_files)
LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)
