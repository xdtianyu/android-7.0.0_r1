ifeq ($(FORCE_BUILD_LLVM_DEBUG),true)
local_optflags = -O0 -g
else
local_optflags = -O2
endif

LOCAL_CFLAGS +=	\
	-D_GNU_SOURCE	\
	-D__STDC_LIMIT_MACROS	\
	$(local_optflags)	\
	-fomit-frame-pointer	\
	-Wall	\
	-W	\
	-Wno-unused-parameter	\
	-Wno-maybe-uninitialized \
	-Wno-missing-field-initializers \
	-Wwrite-strings	\
	-Werror \
	-Dsprintf=sprintf \
	$(LOCAL_CFLAGS)

# Disable certain warnings for use with mingw.
# We also must undefine WIN32_LEAN_AND_MEAN, since it is being passed globally
# on the command line, and LLVM defines this internally itself.
LOCAL_CFLAGS_windows += -Wno-array-bounds \
			-Wno-comment \
			-UWIN32_LEAN_AND_MEAN

ifeq ($(FORCE_BUILD_LLVM_DISABLE_NDEBUG),true)
LOCAL_CFLAGS :=	\
	$(LOCAL_CFLAGS) \
	-D_DEBUG	\
	-UNDEBUG
endif

ifneq ($(REQUIRES_EH),1)
LOCAL_CFLAGS += -fno-exceptions
else
REQUIRES_EH := 0
LOCAL_CFLAGS += -fexceptions
endif

ifneq ($(REQUIRES_RTTI),1)
LOCAL_CPPFLAGS += -fno-rtti
else
REQUIRES_RTTI := 0
endif

LOCAL_CPPFLAGS :=	\
	$(LOCAL_CPPFLAGS)	\
	-Wno-sign-promo         \
	-std=c++11

LOCAL_CPPFLAGS_linux := \
	-Woverloaded-virtual

LOCAL_CPPFLAGS_darwin += \
	-Wno-deprecated-declarations \
	-Woverloaded-virtual

# Make sure bionic is first so we can include system headers.
LOCAL_C_INCLUDES :=	\
	$(LLVM_ROOT_PATH)	\
	$(LLVM_ROOT_PATH)/include	\
	$(LLVM_ROOT_PATH)/host/include	\
	$(LOCAL_C_INCLUDES)

# Add on ncurses to have support for terminfo
LOCAL_LDLIBS_darwin += -lncurses
LOCAL_LDLIBS_linux += -lncurses
LOCAL_LDLIBS_linux += -lgcc_s

LOCAL_IS_HOST_MODULE := true

ifeq ($(HOST_PREFER_32_BIT),true)
LOCAL_MULTILIB := 32
else
ifeq (libLLVM, $(filter libLLVM,$(LOCAL_SHARED_LIBRARIES)$(LOCAL_SHARED_LIBRARIES_$(HOST_OS))))
# Skip building a 32-bit shared object if they are using libLLVM.
LOCAL_MULTILIB := first
endif
endif

###########################################################
## Commands for running tblgen to compile a td file
###########################################################
define transform-host-td-to-out
@mkdir -p $(dir $@)
@echo "Host TableGen: $(TBLGEN_LOCAL_MODULE) (gen-$(1)) <= $<"
$(hide) $(LLVM_TBLGEN) \
	-I $(dir $<)	\
	-I $(LLVM_ROOT_PATH)/include	\
	-I $(LLVM_ROOT_PATH)/host/include	\
	-I $(LLVM_ROOT_PATH)/lib/Target	\
	$(if $(strip $(CLANG_ROOT_PATH)),-I $(CLANG_ROOT_PATH)/include,)	\
	-gen-$(strip $(1))	\
	-o $@ $<
endef
