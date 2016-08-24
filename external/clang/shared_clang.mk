# Don't build the library in unbundled branches.
ifeq (,$(TARGET_BUILD_APPS))

LOCAL_PATH:= $(call my-dir)

clang_whole_static_libraries := \
	libclangAnalysis \
	libclangAST \
	libclangASTMatchers \
	libclangBasic \
	libclangCodeGen \
	libclangDriver \
	libclangEdit \
	libclangFormat \
	libclangFrontend \
	libclangIndex \
	libclangLex \
	libclangLibclang \
	libclangParse \
	libclangRewrite \
	libclangRewriteFrontend \
	libclangSema \
	libclangSerialization \
	libclangTooling

# host
include $(CLEAR_VARS)

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE:= libclang
LOCAL_MODULE_TAGS := optional
LOCAL_WHOLE_STATIC_LIBRARIES := $(clang_whole_static_libraries)

LOCAL_SHARED_LIBRARIES := libLLVM

LOCAL_LDLIBS_windows := -limagehlp -lpsapi

LOCAL_SHARED_LIBRARIES_darwin := libc++
LOCAL_SHARED_LIBRARIES_linux := libc++
LOCAL_LDLIBS_darwin := -ldl -lpthread
LOCAL_LDLIBS_linux := -ldl -lpthread

include $(CLANG_HOST_BUILD_MK)

# Don't build the library unless forced to. We don't
# have prebuilts for windows.
ifneq (true,$(FORCE_BUILD_LLVM_COMPONENTS))
LOCAL_MODULE_HOST_OS := windows
else
LOCAL_MODULE_HOST_OS := darwin linux windows
endif

include $(BUILD_HOST_SHARED_LIBRARY)

# Don't build the library unless forced to.
ifeq (true,$(FORCE_BUILD_LLVM_COMPONENTS))
# device
include $(CLEAR_VARS)

LOCAL_MODULE:= libclang
LOCAL_MODULE_TAGS := optional
LOCAL_WHOLE_STATIC_LIBRARIES := $(clang_whole_static_libraries)

LOCAL_SHARED_LIBRARIES := libLLVM libc++
LOCAL_LDLIBS := -ldl

include $(CLANG_DEVICE_BUILD_MK)
include $(BUILD_SHARED_LIBRARY)
endif # don't build unless forced to

endif # don't build in unbundled branches
