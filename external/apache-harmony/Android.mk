# -*- mode: makefile -*-

LOCAL_PATH := $(call my-dir)

define all-harmony-test-java-files-under
  $(foreach dir,$(1),$(patsubst ./%,%,$(shell cd $(LOCAL_PATH) && find $(dir)/$(2) -name "*.java" 2> /dev/null)))
endef

harmony_test_dirs := \
    beans \
    logging \
    luni \
    sql \
    support \

# TODO: get these working too!
#    crypto \
#    security \
#    x-net

harmony_test_src_files := \
    $(call all-harmony-test-java-files-under,$(harmony_test_dirs),src/test/java) \
    $(call all-harmony-test-java-files-under,$(harmony_test_dirs),src/test/support/java) \
    $(call all-harmony-test-java-files-under,luni,src/test/api/common) \
    $(call all-harmony-test-java-files-under,luni,src/test/api/unix) \
    $(call all-harmony-test-java-files-under,luni,src/test/impl/common) \
    $(call all-harmony-test-java-files-under,luni,src/test/impl/unix)

# We need to use -maxdepth 4 because there's a non-resource directory called "resources" deeper in the tree.
define harmony-test-resource-dirs
  $(foreach dir,$(1),$(patsubst %,./%,$(shell cd $(LOCAL_PATH) && find $(dir) -maxdepth 4 -name resources 2> /dev/null)))
endef

harmony_test_resource_dirs := $(call harmony-test-resource-dirs,$(harmony_test_dirs))

harmony_test_javac_flags:=-Xmaxwarns 9999999

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_test_src_files)
LOCAL_JAVA_RESOURCE_DIRS := $(harmony_test_resource_dirs)
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart core-junit
LOCAL_JAVACFLAGS := $(harmony_test_javac_flags)
LOCAL_MODULE := apache-harmony-tests
LOCAL_JARJAR_RULES := $(LOCAL_PATH)/jarjar-rules.txt
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_test_src_files)
LOCAL_JAVA_RESOURCE_DIRS := $(harmony_test_resource_dirs)
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj-hostdex core-libart-hostdex core-junit-hostdex
LOCAL_JAVACFLAGS := $(harmony_test_javac_flags)
LOCAL_MODULE := apache-harmony-tests-hostdex
LOCAL_JARJAR_RULES := $(LOCAL_PATH)/jarjar-rules.txt
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif  # HOST_OS == linux

include $(call all-makefiles-under,$(LOCAL_PATH))
