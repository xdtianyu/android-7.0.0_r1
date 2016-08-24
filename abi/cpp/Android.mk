LOCAL_PATH:= $(call my-dir)

libgabi++_c_includes := \
	$(LOCAL_PATH)/include \

libgabi++_common_src_files := \
	src/array_type_info.cc \
	src/class_type_info.cc \
        src/delete.cc \
	src/dynamic_cast.cc \
	src/enum_type_info.cc \
	src/function_type_info.cc \
        src/new.cc \
	src/pbase_type_info.cc \
	src/pointer_type_info.cc \
	src/pointer_to_member_type_info.cc \
	src/si_class_type_info.cc \
	src/type_info.cc \
	src/vmi_class_type_info.cc

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= $(libgabi++_common_src_files)
LOCAL_MODULE:= libgabi++
LOCAL_C_INCLUDES := $(libgabi++_c_includes)
LOCAL_RTTI_FLAG := -frtti
LOCAL_CXX_STL := libstdc++
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= $(libgabi++_common_src_files)
LOCAL_MODULE:= libgabi++
LOCAL_C_INCLUDES := $(libgabi++_c_includes)
LOCAL_RTTI_FLAG := -frtti
LOCAL_CXX_STL := libstdc++
include $(BUILD_STATIC_LIBRARY)
