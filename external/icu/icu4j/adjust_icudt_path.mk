# In order to append $(icu4c_data) to the dataPath line in ICUConfig.properties
# this hack here removes the path to that file in the source tree and instead
# appends the path to a dynamically generated modified file to the list of
# arguments passed to the jar tool.
#
# Prerequisites : $(icu4c_data) must be set. This variable will be cleared
# after it's used.
#
# Usage : include this makefile after your $(BUILD*) rule.

ifeq (,$(icu4c_data))
$(error Must set icu4c_data before including adjust_icudt_path.mk)
endif

ifeq (,$(icu4j_config_root))
$(error Must set icu4j_config_root before including adjust_icudt_path.mk)
endif

config_path := com/ibm/icu/ICUConfig.properties
tmp_resource_dir := $(intermediates.COMMON)/tmp

$(tmp_resource_dir)/$(config_path): private_icu4c_data := $(subst /,\/,$(icu4c_data))
$(tmp_resource_dir)/$(config_path): $(icu4j_config_root)/$(config_path)
	$(hide) mkdir -p $(dir $@)
	$(hide) sed "/\.dataPath =/s/$$/ $(private_icu4c_data)/" $< > $@

$(LOCAL_INTERMEDIATE_TARGETS): $(tmp_resource_dir)/$(config_path)
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_EXTRA_JAR_ARGS := \
    $(subst -C "$(icu4j_config_root)" "$(config_path)",,$(extra_jar_args)) \
    -C "$(tmp_resource_dir)" "$(config_path)"

icu4c_data :=
icu4c_config_root :=
config_path :=
tmp_resource_dir :=

