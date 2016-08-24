LOCAL_PATH := $(call my-dir)

test_executable := CtsSimpleperfTestCases
list_executable := $(test_executable)_list
simpleperf_src_path := system/extras/simpleperf

LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

include $(CLEAR_VARS)
LOCAL_MODULE := $(test_executable)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/nativetest
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64

LOCAL_SHARED_LIBRARIES += \
  libbacktrace \
  libunwind \
  libbase \
  liblog \
  libutils \
  libLLVM \

LOCAL_WHOLE_STATIC_LIBRARIES = \
  libsimpleperf_cts_test \

LOCAL_STATIC_LIBRARIES += \
  libgtest \
  libbacktrace_offline \
  libziparchive \
  libz \
  liblzma \

LOCAL_POST_LINK_CMD =  \
  TMP_FILE=`mktemp $(OUT_DIR)/simpleperf-post-link-XXXXXXXXXX` && \
  (cd $(simpleperf_src_path)/testdata && zip - -0 -r .) > $$TMP_FILE && \
  $($(LOCAL_2ND_ARCH_VAR_PREFIX)TARGET_OBJCOPY) --add-section .testzipdata=$$TMP_FILE $(linked_module) && \
  rm -f $$TMP_FILE

LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_CTS_TEST_PACKAGE := android.simpleperf
include $(BUILD_CTS_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := $(list_executable)
LOCAL_MODULE_HOST_OS := linux
LOCAL_MULTILIB := first
LOCAL_LDLIBS = -lrt

LOCAL_WHOLE_STATIC_LIBRARIES += \
  libsimpleperf_cts_test \

LOCAL_STATIC_LIBRARIES += \
  libbacktrace_offline \
  libbacktrace \
  libunwind \
  libziparchive-host \
  libz \
  liblzma \
  libbase \
  liblog \
  libcutils \
  libutils \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMCore \
  libLLVMSupport \

include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_NATIVE_TEST)
