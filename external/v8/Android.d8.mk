LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := d8
LOCAL_MODULE_CLASS := EXECUTABLES

generated_sources := $(call local-generated-sources-dir)

LOCAL_CXX_STL := libc++

LOCAL_SRC_FILES := \
    src/d8.cc \
    src/d8-posix.cc

LOCAL_JS_D8_FILES := \
	$(LOCAL_PATH)/src/d8.js \
	$(LOCAL_PATH)/src/js/macros.py

# Copy js2c.py to generated sources directory and invoke there to avoid
# generating jsmin.pyc in the source directory
JS2C_PY := $(generated_sources)/js2c.py $(generated_sources)/jsmin.py
$(JS2C_PY): $(generated_sources)/%.py : $(LOCAL_PATH)/tools/%.py | $(ACP)
	@echo "Copying $@"
	$(copy-file-to-target)

# Generate d8-js.cc
D8_GEN := $(generated_sources)/d8-js.cc
$(D8_GEN): SCRIPT := $(generated_sources)/js2c.py
$(D8_GEN): $(LOCAL_JS_D8_FILES) $(JS2C_PY)
	@echo "Generating d8-js.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ D8 $(LOCAL_JS_D8_FILES)
LOCAL_GENERATED_SOURCES += $(D8_GEN)

LOCAL_CPP_EXTENSION := .cc

LOCAL_STATIC_LIBRARIES := libv8
LOCAL_SHARED_LIBRARIES += liblog libicuuc libicui18n

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := \
	-DV8_I18N_SUPPORT \
	-Wno-unused-parameter \
	-std=gnu++0x \
	-O0

LOCAL_MODULE_TARGET_ARCH_WARN := $(V8_SUPPORTED_ARCH)

include $(BUILD_EXECUTABLE)
