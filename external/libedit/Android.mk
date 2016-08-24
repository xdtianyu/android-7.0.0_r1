ifeq ($(ANDROID_BUILD_LIBEDIT),true) # only maintainer needs this
ifeq ($(HOST_OS),linux) # only build on linux hosts
ifeq ($(HOST_ARCH),x86) # only build on x86 hosts

LOCAL_PATH := $(call my-dir)
PREBUILT_ROOT := prebuilts/libs/libedit
PREBUILT_ARCH_ROOT := $(PREBUILT_ROOT)/$(HOST_OS)-$(HOST_ARCH)

PREBUILT_CFLAGS="CFLAGS=-m32"
PREBUILT_CXXFLAGS="CXXFLAGS=-m32"
PREBUILT_LDFLAGS="LDFLAGS=-m32"

HOST_STATIC_LIB_EXT := .a

# Light wrapper rules around ./configure-based build.
# Output goes to the prebuilts/libs/libedit directory.

$(LOCAL_PATH)/lib/libedit.la: $(wildcard $(LOCAL_PATH)/src/*) $(LOCAL_PATH)/config.h
	@echo making libedit for $(HOST_OS)-$(HOST_ARCH)
	make -C $(LOCAL_PATH)

$(PREBUILT_ARCH_ROOT)/lib/libedit.la: $(LOCAL_PATH)/lib/libedit.la
	make -C $(LOCAL_PATH) all install

$(LOCAL_PATH)/config.h: $(LOCAL_PATH)/config.h.in
	$(hide) cd $(LOCAL_PATH) \
  && ./configure \
    --prefix=$(abspath $(PREBUILT_ROOT)) \
    --exec-prefix=$(abspath $(PREBUILT_ARCH_ROOT)) \
    --disable-shared \
    $(PREBUILT_CFLAGS) $(PREBUILT_CXXFLAGS) $(PREBUILT_LDFLAGS) \
  && make clean

$(PREBUILT_ARCH_ROOT)/lib/libedit$(HOST_STATIC_LIB_EXT): \
  $(PREBUILT_ARCH_ROOT)/lib/libedit.la

endif # only build on x86 hosts
endif # only build on linux hosts
endif # only maintainer needs this
