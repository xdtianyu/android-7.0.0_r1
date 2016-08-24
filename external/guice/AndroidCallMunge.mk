# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Build support for guice within the Android Open Source Project
# See https://source.android.com/source/building.html for more information
#

# Factored-out implementation of calling munge to post-process guice java files.
#
# Arguments:
#  (Constant)
#    munge_host_jar = Path to munge-host.jar (built by munge-host rule)
#    munge_zip_location = Path to lib/build/munge.jar source archive
#  (Varying)
#    munge_src_arguments = List of files that need to be munged
#    guice_munge_flags = List of flags to pass to munge (e.g. guice_munge_flags := -DNO_AOP)
#

LOCAL_MODULE_CLASS := JAVA_LIBRARIES
# Run munge over every single java file.
intermediates:= $(local-generated-sources-dir)
GEN := $(addprefix $(intermediates)/, $(munge_src_arguments)) # List of all files that need to be munged.
$(GEN) : PRIVATE_ZIP_LOCATION := $(munge_zip_location)
$(GEN) : PRIVATE_HOST_JAR := $(munge_host_jar)
$(GEN) : PRIVATE_MUNGE_FLAGS := $(guice_munge_flags)
$(GEN) : PRIVATE_CUSTOM_TOOL = java -cp $(PRIVATE_HOST_JAR) Munge $(PRIVATE_MUNGE_FLAGS) $< > $@
$(GEN): $(intermediates)/%.java : $(LOCAL_PATH)/%.java $(LOCAL_PATH)/$(munge_zip_location) $(munge_host_jar)
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
