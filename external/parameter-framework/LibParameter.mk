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

ifeq ($(LOCAL_IS_HOST_MODULE),true)
SUFFIX := _host
else
SUFFIX :=
endif

LOCAL_MODULE := libparameter$(SUFFIX)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := \
    upstream/parameter/ParameterMgrPlatformConnector.cpp \
    upstream/parameter/LoggingElementBuilderTemplate.cpp \
    upstream/parameter/StringParameterType.cpp \
    upstream/parameter/SyncerSet.cpp \
    upstream/parameter/BitParameter.cpp \
    upstream/parameter/BaseParameter.cpp \
    upstream/parameter/ParameterBlockType.cpp \
    upstream/parameter/FloatingPointParameterType.cpp \
    upstream/parameter/SelectionCriteriaDefinition.cpp \
    upstream/parameter/EnumValuePair.cpp \
    upstream/parameter/SelectionCriteria.cpp \
    upstream/parameter/SelectionCriterionRule.cpp \
    upstream/parameter/AreaConfiguration.cpp \
    upstream/parameter/BitParameterBlockType.cpp \
    upstream/parameter/ConfigurationAccessContext.cpp \
    upstream/parameter/BitwiseAreaConfiguration.cpp \
    upstream/parameter/ArrayParameter.cpp \
    upstream/parameter/ParameterBlackboard.cpp \
    upstream/parameter/InstanceConfigurableElement.cpp \
    upstream/parameter/LogarithmicParameterAdaptation.cpp \
    upstream/parameter/ConfigurableDomain.cpp \
    upstream/parameter/FormattedSubsystemObject.cpp \
    upstream/parameter/MappingData.cpp \
    upstream/parameter/SubsystemElementBuilder.cpp \
    upstream/parameter/BooleanParameterType.cpp \
    upstream/parameter/FixedPointParameterType.cpp \
    upstream/parameter/ComponentType.cpp \
    upstream/parameter/EnumParameterType.cpp \
    upstream/parameter/RuleParser.cpp \
    upstream/parameter/VirtualSubsystem.cpp \
    upstream/parameter/Element.cpp \
    upstream/parameter/ParameterFrameworkConfiguration.cpp \
    upstream/parameter/SelectionCriterionLibrary.cpp \
    upstream/parameter/StringParameter.cpp \
    upstream/parameter/CompoundRule.cpp \
    upstream/parameter/ConfigurableDomains.cpp \
    upstream/parameter/VirtualSyncer.cpp \
    upstream/parameter/MappingContext.cpp \
    upstream/parameter/LinearParameterAdaptation.cpp \
    upstream/parameter/ComponentLibrary.cpp \
    upstream/parameter/BitParameterBlock.cpp \
    upstream/parameter/ParameterMgrFullConnector.cpp \
    upstream/parameter/ConfigurableElement.cpp \
    upstream/parameter/ConfigurableElementAggregator.cpp \
    upstream/parameter/SubsystemObject.cpp \
    upstream/parameter/TypeElement.cpp \
    upstream/parameter/PathNavigator.cpp \
    upstream/parameter/ElementLocator.cpp \
    upstream/parameter/SimulatedBackSynchronizer.cpp \
    upstream/parameter/Parameter.cpp \
    upstream/parameter/ComponentInstance.cpp \
    upstream/parameter/InstanceDefinition.cpp \
    upstream/parameter/SubsystemObjectCreator.cpp \
    upstream/parameter/ParameterType.cpp \
    upstream/parameter/DomainConfiguration.cpp \
    upstream/parameter/PluginLocation.cpp \
    upstream/parameter/HardwareBackSynchronizer.cpp \
    upstream/parameter/SystemClass.cpp \
    upstream/parameter/ElementLibrary.cpp \
    upstream/parameter/ParameterAccessContext.cpp \
    upstream/parameter/XmlParameterSerializingContext.cpp \
    upstream/parameter/ElementHandle.cpp \
    upstream/parameter/ParameterMgr.cpp \
    upstream/parameter/SelectionCriterionType.cpp \
    upstream/parameter/Subsystem.cpp \
    upstream/parameter/IntegerParameterType.cpp \
    upstream/parameter/BitParameterType.cpp \
    upstream/parameter/SelectionCriterion.cpp \
    upstream/parameter/XmlElementSerializingContext.cpp \
    upstream/parameter/ElementLibrarySet.cpp \
    upstream/parameter/FrameworkConfigurationLocation.cpp \
    upstream/parameter/ParameterAdaptation.cpp \
    upstream/parameter/XmlFileIncluderElement.cpp \
    upstream/xmlserializer/XmlElement.cpp \
    upstream/xmlserializer/XmlSerializingContext.cpp \
    upstream/xmlserializer/XmlMemoryDocSource.cpp \
    upstream/xmlserializer/XmlDocSource.cpp \
    upstream/xmlserializer/XmlMemoryDocSink.cpp \
    upstream/xmlserializer/XmlStreamDocSink.cpp \
    upstream/parameter/CommandHandlerWrapper.cpp

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/upstream/parameter/ \
    $(LOCAL_PATH)/upstream/parameter/log/include \
    $(LOCAL_PATH)/upstream/parameter/include \
    $(LOCAL_PATH)/upstream/xmlserializer/ \
    $(LOCAL_PATH)/upstream/remote-processor/ \
    $(LOCAL_PATH)/support/android/parameter/

common_copy_headers_to := parameter
common_copy_headers := \
    upstream/parameter/include/ParameterMgrLoggerForward.h \
    upstream/parameter/include/ParameterMgrPlatformConnector.h \
    upstream/parameter/include/ParameterMgrFullConnector.h \
    upstream/parameter/include/SelectionCriterionTypeInterface.h \
    upstream/parameter/include/SelectionCriterionInterface.h \
    upstream/parameter/include/ParameterHandle.h \
    support/android/parameter/parameter_export.h \
    upstream/parameter/include/ElementHandle.h

LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDE_DIRS)

LOCAL_COPY_HEADERS_TO := $(common_copy_headers_to)
LOCAL_COPY_HEADERS := $(common_copy_headers)

LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDE_DIRS)

LOCAL_SHARED_LIBRARIES := libremote-processor$(PFW_NETWORKING_SUFFIX)$(SUFFIX)
LOCAL_C_INCLUDES += \
    external/libxml2/include \
    external/icu/icu4c/source/common

LOCAL_CFLAGS := -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := \
    libpfw_utility$(SUFFIX) \
    libxml2

LOCAL_CLANG := true
