
LOCAL_PATH:=$(call my-dir)

rs_base_CFLAGS := -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -fno-exceptions -std=c++11
ifeq ($(TARGET_BUILD_PDK), true)
  rs_base_CFLAGS += -D__RS_PDK__
endif

ifneq ($(OVERRIDE_RS_DRIVER),)
  rs_base_CFLAGS += -DOVERRIDE_RS_DRIVER=$(OVERRIDE_RS_DRIVER)
endif

ifneq ($(DISABLE_RS_64_BIT_DRIVER),)
  rs_base_CFLAGS += -DDISABLE_RS_64_BIT_DRIVER
endif

ifeq ($(RS_FIND_OFFSETS), true)
  rs_base_CFLAGS += -DRS_FIND_OFFSETS
endif

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := libRSDriver
LOCAL_MODULE_TARGET_ARCH_WARN := arm mips mips64 x86 x86_64 arm64

LOCAL_SRC_FILES:= \
	driver/rsdAllocation.cpp \
	driver/rsdBcc.cpp \
	driver/rsdCore.cpp \
	driver/rsdElement.cpp \
	driver/rsdFrameBuffer.cpp \
	driver/rsdFrameBufferObj.cpp \
	driver/rsdGL.cpp \
	driver/rsdMesh.cpp \
	driver/rsdMeshObj.cpp \
	driver/rsdProgram.cpp \
	driver/rsdProgramRaster.cpp \
	driver/rsdProgramStore.cpp \
	driver/rsdRuntimeStubs.cpp \
	driver/rsdSampler.cpp \
	driver/rsdScriptGroup.cpp \
	driver/rsdShader.cpp \
	driver/rsdShaderCache.cpp \
	driver/rsdType.cpp \
	driver/rsdVertexArray.cpp


LOCAL_SHARED_LIBRARIES += libRS_internal libRSCpuRef
LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libEGL libGLESv1_CM libGLESv2
LOCAL_SHARED_LIBRARIES += libui libgui libsync

LOCAL_SHARED_LIBRARIES += libbcinfo libLLVM

LOCAL_C_INCLUDES += frameworks/compile/libbcc/include

LOCAL_CXX_STL := libc++

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Build rsg-generator ====================
include $(CLEAR_VARS)

LOCAL_MODULE := rsg-generator

# These symbols are normally defined by BUILD_XXX, but we need to define them
# here so that local-intermediates-dir works.

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
intermediates := $(local-intermediates-dir)

LOCAL_SRC_FILES:= \
    spec.l \
    rsg_generator.c

LOCAL_CXX_STL := none
LOCAL_SANITIZE := never

include $(BUILD_HOST_EXECUTABLE)

# TODO: This should go into build/core/config.mk
RSG_GENERATOR:=$(LOCAL_BUILT_MODULE)

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := libRS_internal
LOCAL_MODULE_TARGET_ARCH_WARN := arm mips mips64 x86 x86_64 arm64

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
generated_sources:= $(local-generated-sources-dir)

# Generate custom headers

GEN := $(addprefix $(generated_sources)/, \
            rsgApiStructs.h \
            rsgApiFuncDecl.h \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(generated_sources)/%.h : $(LOCAL_PATH)/%.h.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)
LOCAL_GENERATED_SOURCES += $(GEN)

# Generate custom source files

GEN := $(addprefix $(generated_sources)/, \
            rsgApiReplay.cpp \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(generated_sources)/%.cpp : $(LOCAL_PATH)/%.cpp.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_SRC_FILES:= \
	rsAllocation.cpp \
	rsAnimation.cpp \
	rsComponent.cpp \
	rsContext.cpp \
	rsClosure.cpp \
	rsCppUtils.cpp \
	rsDevice.cpp \
	rsDriverLoader.cpp \
	rsElement.cpp \
	rsFBOCache.cpp \
	rsFifoSocket.cpp \
	rsFileA3D.cpp \
	rsFont.cpp \
	rsGrallocConsumer.cpp \
	rsObjectBase.cpp \
	rsMatrix2x2.cpp \
	rsMatrix3x3.cpp \
	rsMatrix4x4.cpp \
	rsMesh.cpp \
	rsMutex.cpp \
	rsProgram.cpp \
	rsProgramFragment.cpp \
	rsProgramStore.cpp \
	rsProgramRaster.cpp \
	rsProgramVertex.cpp \
	rsSampler.cpp \
	rsScript.cpp \
	rsScriptC.cpp \
	rsScriptC_Lib.cpp \
	rsScriptC_LibGL.cpp \
	rsScriptGroup.cpp \
	rsScriptGroup2.cpp \
	rsScriptIntrinsic.cpp \
	rsSignal.cpp \
	rsStream.cpp \
	rsThreadIO.cpp \
	rsType.cpp

LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libEGL libGLESv1_CM libGLESv2
LOCAL_SHARED_LIBRARIES += libgui libsync libdl libui
LOCAL_SHARED_LIBRARIES += libft2 libpng libz

LOCAL_SHARED_LIBRARIES += libbcinfo libLLVM

LOCAL_C_INCLUDES += external/freetype/include
LOCAL_C_INCLUDES += frameworks/compile/libbcc/include

LOCAL_CXX_STL := libc++

LOCAL_CFLAGS += $(rs_base_CFLAGS)
# TODO: external/freetype still uses the register keyword
# Bug: 17163086
LOCAL_CFLAGS += -Wno-deprecated-register

LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_MODULE := libRS
LOCAL_MODULE_TARGET_ARCH_WARN := arm mips mips64 x86 x86_64 arm64

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
generated_sources:= $(local-generated-sources-dir)

# Generate custom headers

GEN := $(addprefix $(generated_sources)/, \
            rsgApiStructs.h \
            rsgApiFuncDecl.h \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(generated_sources)/%.h : $(LOCAL_PATH)/%.h.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)
LOCAL_GENERATED_SOURCES += $(GEN)

# Generate custom source files

GEN := $(addprefix $(generated_sources)/, \
            rsgApi.cpp \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(generated_sources)/%.cpp : $(LOCAL_PATH)/%.cpp.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_SRC_FILES:= \
	rsApiAllocation.cpp \
	rsApiContext.cpp \
	rsApiDevice.cpp \
	rsApiElement.cpp \
	rsApiFileA3D.cpp \
	rsApiMesh.cpp \
	rsApiType.cpp \

LOCAL_SHARED_LIBRARIES += libRS_internal
LOCAL_SHARED_LIBRARIES += liblog

LOCAL_CFLAGS += $(rs_base_CFLAGS)
# TODO: external/freetype still uses the register keyword
# Bug: 17163086
LOCAL_CFLAGS += -Wno-deprecated-register

LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_LDFLAGS += -Wl,--version-script,${LOCAL_PATH}/libRS.map

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Now build a host version for serialization
include $(CLEAR_VARS)
LOCAL_MODULE:= libRS
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_IS_HOST_MODULE := true

intermediates := $(call local-generated-sources-dir)

# Generate custom headers

GEN := $(addprefix $(intermediates)/, \
            rsgApiStructs.h \
            rsgApiFuncDecl.h \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(intermediates)/%.h : $(LOCAL_PATH)/%.h.rsg
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)

# Generate custom source files

GEN := $(addprefix $(intermediates)/, \
            rsgApi.cpp \
            rsgApiReplay.cpp \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec
$(GEN): $(intermediates)/%.cpp : $(LOCAL_PATH)/%.cpp.rsg
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CFLAGS += -DANDROID_RS_SERIALIZE
LOCAL_CFLAGS += -fPIC
LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_SRC_FILES:= \
	rsAllocation.cpp \
	rsAnimation.cpp \
	rsComponent.cpp \
	rsContext.cpp \
	rsClosure.cpp \
	rsDevice.cpp \
	rsDriverLoader.cpp \
	rsElement.cpp \
	rsFBOCache.cpp \
	rsFifoSocket.cpp \
	rsFileA3D.cpp \
	rsFont.cpp \
	rsObjectBase.cpp \
	rsMatrix2x2.cpp \
	rsMatrix3x3.cpp \
	rsMatrix4x4.cpp \
	rsMesh.cpp \
	rsMutex.cpp \
	rsProgram.cpp \
	rsProgramFragment.cpp \
	rsProgramStore.cpp \
	rsProgramRaster.cpp \
	rsProgramVertex.cpp \
	rsSampler.cpp \
	rsScript.cpp \
	rsScriptC.cpp \
	rsScriptC_Lib.cpp \
	rsScriptC_LibGL.cpp \
	rsScriptGroup.cpp \
	rsScriptGroup2.cpp \
	rsScriptIntrinsic.cpp \
	rsSignal.cpp \
	rsStream.cpp \
	rsThreadIO.cpp \
	rsType.cpp

LOCAL_STATIC_LIBRARIES := libcutils libutils liblog

LOCAL_CLANG := true

include $(BUILD_HOST_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
