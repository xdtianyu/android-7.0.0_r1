LOCAL_PATH:= $(call my-dir)

###########################################

# trove prebuilt. Module stem is chosen so it can be used as a static library.

include $(CLEAR_VARS)

LOCAL_MODULE := trove-prebuilt
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := repository/net/sf/trove4j/trove4j/1.1/trove4j-1.1.jar
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_BUILT_MODULE_STEM := javalib.jar
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)

include $(BUILD_PREBUILT)

###########################################

# com.squareup.haha prebuilt.

include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
    squareup-haha-prebuilt:repository/com/squareup/haha/haha/2.0.2/haha-2.0.2.jar

include $(BUILD_MULTI_PREBUILT)

###########################################
