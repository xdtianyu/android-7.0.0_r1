#=====================================================================
# Calculate SHA1 checksum for libbcc.so, libRS.so and libclcore.bc
#=====================================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_MODULE := libbcc.sha1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

libbcc_SHA1_SRCS := \
  $($(my_2nd_arch_prefix)TARGET_OUT_INTERMEDIATE_LIBRARIES)/libbcc.so \
  $($(my_2nd_arch_prefix)TARGET_OUT_INTERMEDIATE_LIBRARIES)/libcompiler_rt.so \
  $($(my_2nd_arch_prefix)TARGET_OUT_INTERMEDIATE_LIBRARIES)/libRS.so \
  $(call intermediates-dir-for,SHARED_LIBRARIES,libclcore.bc,,,$(my_2nd_arch_prefix))/libclcore.bc \
  $(call intermediates-dir-for,SHARED_LIBRARIES,libclcore_debug.bc,,,$(my_2nd_arch_prefix))/libclcore_debug.bc

ifeq ($(TARGET_$(my_2nd_arch_prefix)ARCH),arm)
ifeq ($(ARCH_ARM_HAVE_NEON),true)
  libbcc_SHA1_SRCS += \
    $(call intermediates-dir-for,SHARED_LIBRARIES,libclcore_neon.bc,,,$(my_2nd_arch_prefix))/libclcore_neon.bc
endif
endif

libbcc_GEN_SHA1_STAMP := $(LOCAL_PATH)/tools/build/gen-sha1-stamp.py
intermediates := $(call local-intermediates-dir,,$(my_2nd_arch_prefix))

libbcc_SHA1_ASM := $(intermediates)/libbcc.sha1.S
LOCAL_GENERATED_SOURCES += $(libbcc_SHA1_ASM)
$(libbcc_SHA1_ASM): PRIVATE_SHA1_SRCS := $(libbcc_SHA1_SRCS)
$(libbcc_SHA1_ASM): $(libbcc_SHA1_SRCS) $(libbcc_GEN_SHA1_STAMP)
	@echo libbcc.sha1: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(libbcc_GEN_SHA1_STAMP) $(PRIVATE_SHA1_SRCS) > $@

LOCAL_CFLAGS += -D_REENTRANT -DPIC -fPIC
LOCAL_CFLAGS += -O3 -nodefaultlibs -nostdlib

LOCAL_MODULE_TARGET_ARCH := $(filter $(TARGET_$(my_2nd_arch_prefix)ARCH),$(LLVM_SUPPORTED_ARCH))

ifdef LOCAL_MODULE_TARGET_ARCH
include $(BUILD_SHARED_LIBRARY)
endif
endif
