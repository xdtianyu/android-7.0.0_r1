LOCAL_CFLAGS :=	\
	-pedantic	\
	-Wcast-qual	\
	-Wno-long-long	\
	$(LOCAL_CFLAGS)

LOCAL_CPPFLAGS := \
	-Wno-sign-promo \
	$(LOCAL_CPPFLAGS)

ifeq ($(FORCE_BUILD_LLVM_DISABLE_NDEBUG),true)
LOCAL_CFLAGS :=	\
	$(LOCAL_CFLAGS) \
	-D_DEBUG	\
	-UNDEBUG
endif

# Make sure bionic is first so we can include system headers.
LOCAL_C_INCLUDES :=	\
	$(CLANG_ROOT_PATH)/include	\
	$(CLANG_ROOT_PATH)/lib/CodeGen    \
	$(LOCAL_C_INCLUDES)

LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

ifneq ($(LLVM_DEVICE_BUILD_MK),)
include $(LLVM_DEVICE_BUILD_MK)
endif

###########################################################
## Commands for running tblgen to compile a td file
###########################################################
define transform-device-clang-td-to-out
@mkdir -p $(dir $@)
@echo "Device Clang TableGen: $(TBLGEN_LOCAL_MODULE) (gen-$(1)) <= $<"
$(hide) $(CLANG_TBLGEN) \
	-I $(dir $<)	\
	-I $(LLVM_ROOT_PATH)/include	\
	-I $(LLVM_ROOT_PATH)/device/include	\
	-I $(LLVM_ROOT_PATH)/lib/Target	\
	$(if $(strip $(CLANG_ROOT_PATH)),-I $(CLANG_ROOT_PATH)/include,)	\
	-gen-$(strip $(1))	\
	-o $@ $<
endef
