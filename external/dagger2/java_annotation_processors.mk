# Copyright (C) 2015 The Android Open Source Project
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

# Manages use of annotation processors.
#
# At the moment both the -processorpath and the -processor
# flags must be specified in order to use annotation processors
# as a code indexing tool that wraps javac doesn't as yet support
# the same behaviour as standard javac with regard to use of
# annotation processors. In particular it:
# - doesn't default -processorpath to be the same as -classpath
# - doesn't scan processorpath to automatically discover processors
# - doesn't support a comma separated list of processor class names
#   on a single -processor option so need on option per class name.
#
# Input variables:
#
#   PROCESSOR_LIBRARIES := <list of library names>
#     Similar to names added to LOCAL_JAVA_LIBRARIES.
#
#   PROCESSOR_CLASSES := <list of processor class names>
#
# Upon exit various LOCAL_ variables have been updated and the
# input variables have been cleared.

# Map the library names to actual JARs.
PROCESSOR_JARS := $(call java-lib-deps, $(PROCESSOR_LIBRARIES), true)

# Add a javac -processorpath flag.
LOCAL_JAVACFLAGS += -processorpath $(call normalize-path-list,$(PROCESSOR_JARS))

# Specify only one processor class per -processor option as
# the indexing tool does not parse the -processor value as a
# comma separated list.
LOCAL_JAVACFLAGS += $(foreach class,$(PROCESSOR_CLASSES),-processor $(class))

# Create a source directory into which the code will be generated.
GENERATED_SOURCE_DIR := $(local-generated-sources-dir)/annotation_processor_output/

# Tell javac to generate source files in the source directory.
LOCAL_JAVACFLAGS += -s $(GENERATED_SOURCE_DIR)
LOCAL_GENERATED_SOURCES := $(GENERATED_SOURCE_DIR)

# Add dependency between the jar being built and the processor jars so that
# they are built before this one.
LOCAL_ADDITIONAL_DEPENDENCIES += $(PROCESSOR_JARS) $(GENERATED_SOURCE_DIR)

$(GENERATED_SOURCE_DIR):
	mkdir -p $@

# Clean up all the extra variables to make sure that they don't escape to
# another module.
PROCESSOR_LIBRARIES :=
PROCESSOR_CLASSES :=
PROCESSOR_JARS :=
GENERATED_SOURCE_DIR :=
