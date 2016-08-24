# -*- mode: makefile -*-

LOCAL_PATH := $(call my-dir)

sqlite_jdbc_src_files := src/main/native/sqlite_jni.c
sqlite_jdbc_local_c_includes := external/sqlite/dist
sqlite_cflags := -Wno-unused-parameter

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under,src/main/java)
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sqlite-jdbc
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(sqlite_cflags)
LOCAL_SRC_FILES := $(sqlite_jdbc_src_files)
LOCAL_C_INCLUDES += $(sqlite_jdbc_local_c_includes)
LOCAL_STATIC_LIBRARIES += libsqlite_static_minimal
LOCAL_MODULE_TAGS := optional
# This name is dictated by the fact that the SQLite code calls loadLibrary("sqlite_jni").
LOCAL_MODULE := libsqlite_jni
LOCAL_SDK_VERSION := 23
include $(BUILD_SHARED_LIBRARY)

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under,src/main/java)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sqlite-jdbc-host
include $(BUILD_HOST_DALVIK_STATIC_JAVA_LIBRARY)
endif  # HOST_OS == linux.

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(sqlite_cflags)
LOCAL_SRC_FILES := $(sqlite_jdbc_src_files)
LOCAL_C_INCLUDES += $(sqlite_jdbc_local_c_includes)
LOCAL_STATIC_LIBRARIES += libsqlite_static_minimal
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsqlite_jni
include $(BUILD_HOST_SHARED_LIBRARY)
