# Copyright (c) 2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Resources are not compiled so the prebuild mechanism is used to export them.
# Schemas are only used by host, in order to validate xml files
##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfiguration.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ConfigurableDomain.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    ParameterSettings.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ConfigurableDomains.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    ConfigurableDomain.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := SystemClass.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    FileIncluder.xsd \
    Subsystem.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ParameterSettings.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := FileIncluder.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Subsystem.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    ComponentLibrary.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ComponentLibrary.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    ComponentTypeSet.xsd \
    W3cXmlAttributes.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ComponentTypeSet.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
LOCAL_REQUIRED_MODULES := \
    Parameter.xsd \
    W3cXmlAttributes.xsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := W3cXmlAttributes.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Parameter.xsd
LOCAL_MODULE_OWNER := intel
LOCAL_SRC_FILES := upstream/schemas/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/etc/parameter-framework/Schemas
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)
##################################################
