LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbinary_parse
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= \
    src/binary_parse/cached_paged_byte_array.cc \
    src/binary_parse/range_checked_byte_ptr.cc
LOCAL_CPPFALGS := -Wsign-compare
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libimage_type_recognition
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= \
    src/image_type_recognition/image_type_recognition_lite.cc
LOCAL_SHARED_LIBRARIES := libbinary_parse
LOCAL_CPPFALGS := -Wsign-compare
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libtiff_directory
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= \
    src/tiff_directory/tiff_directory.cc
LOCAL_SHARED_LIBRARIES := libbinary_parse
LOCAL_CPPFALGS := -Wsign-compare
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libpiex
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= \
    src/tiff_parser.cc \
    src/piex.cc
LOCAL_SHARED_LIBRARIES := \
    libbinary_parse \
    libimage_type_recognition \
    libtiff_directory
LOCAL_CPPFALGS := -Wsign-compare
include $(BUILD_SHARED_LIBRARY)
