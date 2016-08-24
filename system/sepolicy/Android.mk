LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# SELinux policy version.
# Must be <= /sys/fs/selinux/policyvers reported by the Android kernel.
# Must be within the compatibility range reported by checkpolicy -V.
POLICYVERS ?= 30

MLS_SENS=1
MLS_CATS=1024

ifdef BOARD_SEPOLICY_REPLACE
$(error BOARD_SEPOLICY_REPLACE is no longer supported; please remove from your BoardConfig.mk or other .mk file.)
endif

ifdef BOARD_SEPOLICY_IGNORE
$(error BOARD_SEPOLICY_IGNORE is no longer supported; please remove from your BoardConfig.mk or other .mk file.)
endif

ifdef BOARD_SEPOLICY_UNION
$(warning BOARD_SEPOLICY_UNION is no longer required - all files found in BOARD_SEPOLICY_DIRS are implicitly unioned; please remove from your BoardConfig.mk or other .mk file.)
endif

ifdef BOARD_SEPOLICY_M4DEFS
LOCAL_ADDITIONAL_M4DEFS := $(addprefix -D, $(BOARD_SEPOLICY_M4DEFS))
endif

# Builds paths for all policy files found in BOARD_SEPOLICY_DIRS and the LOCAL_PATH.
# $(1): the set of policy name paths to build
build_policy = $(foreach type, $(1), $(foreach file, $(addsuffix /$(type), $(LOCAL_PATH) $(BOARD_SEPOLICY_DIRS)), $(sort $(wildcard $(file)))))

# Builds paths for all policy files found in BOARD_SEPOLICY_DIRS.
# $(1): the set of policy name paths to build
build_device_policy = $(foreach type, $(1), $(foreach file, $(addsuffix /$(type), $(BOARD_SEPOLICY_DIRS)), $(sort $(wildcard $(file)))))

# Add a file containing only a newline in-between each policy configuration
# 'contexts' file. This will allow OEM policy configuration files without a
# final newline (0x0A) to be built correctly by the m4(1) macro processor.
# $(1): the set of contexts file names.
# $(2): the file containing only 0x0A.
add_nl = $(foreach entry, $(1), $(subst $(entry), $(entry) $(2), $(entry)))

sepolicy_build_files := security_classes \
                        initial_sids \
                        access_vectors \
                        global_macros \
                        neverallow_macros \
                        mls_macros \
                        mls \
                        policy_capabilities \
                        te_macros \
                        attributes \
                        ioctl_defines \
                        ioctl_macros \
                        *.te \
                        roles \
                        users \
                        initial_sid_contexts \
                        fs_use \
                        genfs_contexts \
                        port_contexts

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := sectxfile_nl
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

# Create a file containing newline only to add between context config files
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE):
	@mkdir -p $(dir $@)
	$(hide) echo > $@

built_nl := $(LOCAL_BUILT_MODULE)

#################################
include $(CLEAR_VARS)

LOCAL_MODULE := sepolicy
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

sepolicy_policy.conf := $(intermediates)/policy.conf
$(sepolicy_policy.conf): PRIVATE_MLS_SENS := $(MLS_SENS)
$(sepolicy_policy.conf): PRIVATE_MLS_CATS := $(MLS_CATS)
$(sepolicy_policy.conf): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(sepolicy_policy.conf): $(call build_policy, $(sepolicy_build_files))
	@mkdir -p $(dir $@)
	$(hide) m4 $(PRIVATE_ADDITIONAL_M4DEFS) \
		-D mls_num_sens=$(PRIVATE_MLS_SENS) -D mls_num_cats=$(PRIVATE_MLS_CATS) \
		-D target_build_variant=$(TARGET_BUILD_VARIANT) \
		-s $^ > $@
	$(hide) sed '/dontaudit/d' $@ > $@.dontaudit

$(LOCAL_BUILT_MODULE): $(sepolicy_policy.conf) $(HOST_OUT_EXECUTABLES)/checkpolicy $(HOST_OUT_EXECUTABLES)/sepolicy-analyze
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkpolicy -M -c $(POLICYVERS) -o $@.tmp $<
	$(hide) $(HOST_OUT_EXECUTABLES)/checkpolicy -M -c $(POLICYVERS) -o $(dir $<)/$(notdir $@).dontaudit $<.dontaudit
	$(hide) $(HOST_OUT_EXECUTABLES)/sepolicy-analyze $@.tmp permissive > $@.permissivedomains
	$(hide) if [ "$(TARGET_BUILD_VARIANT)" = "user" -a -s $@.permissivedomains ]; then \
		echo "==========" 1>&2; \
		echo "ERROR: permissive domains not allowed in user builds" 1>&2; \
		echo "List of invalid domains:" 1>&2; \
		cat $@.permissivedomains 1>&2; \
		exit 1; \
		fi
	$(hide) mv $@.tmp $@

built_sepolicy := $(LOCAL_BUILT_MODULE)
sepolicy_policy.conf :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := sepolicy.recovery
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := eng

include $(BUILD_SYSTEM)/base_rules.mk

sepolicy_policy_recovery.conf := $(intermediates)/policy_recovery.conf
$(sepolicy_policy_recovery.conf): PRIVATE_MLS_SENS := $(MLS_SENS)
$(sepolicy_policy_recovery.conf): PRIVATE_MLS_CATS := $(MLS_CATS)
$(sepolicy_policy_recovery.conf): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(sepolicy_policy_recovery.conf): $(call build_policy, $(sepolicy_build_files))
	@mkdir -p $(dir $@)
	$(hide) m4 $(PRIVATE_ADDITIONAL_M4DEFS) \
		-D mls_num_sens=$(PRIVATE_MLS_SENS) -D mls_num_cats=$(PRIVATE_MLS_CATS) \
		-D target_build_variant=$(TARGET_BUILD_VARIANT) \
		-D target_recovery=true \
		-s $^ > $@

$(LOCAL_BUILT_MODULE): $(sepolicy_policy_recovery.conf) $(HOST_OUT_EXECUTABLES)/checkpolicy $(HOST_OUT_EXECUTABLES)/sepolicy-analyze
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkpolicy -M -c $(POLICYVERS) -o $@.tmp $<
	$(hide) $(HOST_OUT_EXECUTABLES)/sepolicy-analyze $@.tmp permissive > $@.permissivedomains
	$(hide) if [ "$(TARGET_BUILD_VARIANT)" = "user" -a -s $@.permissivedomains ]; then \
		echo "==========" 1>&2; \
		echo "ERROR: permissive domains not allowed in user builds" 1>&2; \
		echo "List of invalid domains:" 1>&2; \
		cat $@.permissivedomains 1>&2; \
		exit 1; \
		fi
	$(hide) mv $@.tmp $@

built_sepolicy_recovery := $(LOCAL_BUILT_MODULE)
sepolicy_policy_recovery.conf :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := general_sepolicy.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

exp_sepolicy_build_files :=\
  $(foreach file, $(addprefix $(LOCAL_PATH)/, $(sepolicy_build_files)), $(sort $(wildcard $(file))))

$(LOCAL_BUILT_MODULE): PRIVATE_MLS_SENS := $(MLS_SENS)
$(LOCAL_BUILT_MODULE): PRIVATE_MLS_CATS := $(MLS_CATS)
$(LOCAL_BUILT_MODULE): $(exp_sepolicy_build_files)
	mkdir -p $(dir $@)
	$(hide) m4 -D mls_num_sens=$(PRIVATE_MLS_SENS) -D mls_num_cats=$(PRIVATE_MLS_CATS) \
		-D target_build_variant=user \
		-s $^ > $@
	$(hide) sed '/dontaudit/d' $@ > $@.dontaudit

built_general_sepolicy.conf := $(LOCAL_BUILT_MODULE)
exp_sepolicy_build_files :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := sepolicy.general
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): PRIVATE_BUILT_SEPOLICY.CONF := $(built_general_sepolicy.conf)
$(LOCAL_BUILT_MODULE): $(built_general_sepolicy.conf) $(HOST_OUT_EXECUTABLES)/checkpolicy
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkpolicy -M -c $(POLICYVERS) -o $@ $(PRIVATE_BUILT_SEPOLICY.CONF)

built_general_sepolicy := $(LOCAL_BUILT_MODULE)
##################################
include $(CLEAR_VARS)

LOCAL_MODULE := file_contexts.bin
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

# The file_contexts.bin is built in the following way:
# 1. Collect all file_contexts files in THIS repository and process them with
#    m4 into a tmp file called file_contexts.local.tmp.
# 2. Collect all device specific file_contexts files and process them with m4
#    into a tmp file called file_contexts.device.tmp.
# 3. Run checkfc -e (allow no device fc entries ie empty) and fc_sort on
#    file_contexts.device.tmp and output to file_contexts.device.sorted.tmp.
# 4. Concatenate file_contexts.local.tmp and file_contexts.device.tmp into
#    file_contexts.concat.tmp.
# 5. Run checkfc and sefcontext_compile on file_contexts.concat.tmp to produce
#    file_contexts.bin.
#
#  Note: That a newline file is placed between each file_context file found to
#        ensure a proper build when an fc file is missing an ending newline.

local_fc_files := $(LOCAL_PATH)/file_contexts
ifneq ($(filter address,$(SANITIZE_TARGET)),)
  local_fc_files := $(local_fc_files) $(LOCAL_PATH)/file_contexts_asan
endif
local_fcfiles_with_nl := $(call add_nl, $(local_fc_files), $(built_nl))

file_contexts.local.tmp := $(intermediates)/file_contexts.local.tmp
$(file_contexts.local.tmp): $(local_fcfiles_with_nl)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $^ > $@

device_fc_files := $(call build_device_policy, file_contexts)
device_fcfiles_with_nl := $(call add_nl, $(device_fc_files), $(built_nl))

file_contexts.device.tmp := $(intermediates)/file_contexts.device.tmp
$(file_contexts.device.tmp): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(file_contexts.device.tmp): $(device_fcfiles_with_nl)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $(PRIVATE_ADDITIONAL_M4DEFS) $^ > $@

file_contexts.device.sorted.tmp := $(intermediates)/file_contexts.device.sorted.tmp
$(file_contexts.device.sorted.tmp): PRIVATE_SEPOLICY := $(built_sepolicy)
$(file_contexts.device.sorted.tmp): $(file_contexts.device.tmp) $(built_sepolicy) $(HOST_OUT_EXECUTABLES)/fc_sort $(HOST_OUT_EXECUTABLES)/checkfc
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc -e $(PRIVATE_SEPOLICY) $<
	$(hide) $(HOST_OUT_EXECUTABLES)/fc_sort $< $@

file_contexts.concat.tmp := $(intermediates)/file_contexts.concat.tmp
$(file_contexts.concat.tmp): $(file_contexts.local.tmp) $(file_contexts.device.sorted.tmp)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $^ > $@

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_sepolicy)
$(LOCAL_BUILT_MODULE): $(file_contexts.concat.tmp) $(built_sepolicy) $(HOST_OUT_EXECUTABLES)/sefcontext_compile $(HOST_OUT_EXECUTABLES)/checkfc
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc $(PRIVATE_SEPOLICY) $<
	$(hide) $(HOST_OUT_EXECUTABLES)/sefcontext_compile -o $@ $<

built_fc := $(LOCAL_BUILT_MODULE)
local_fc_files :=
local_fcfiles_with_nl :=
device_fc_files :=
device_fcfiles_with_nl :=
file_contexts.concat.tmp :=
file_contexts.device.sorted.tmp :=
file_contexts.device.tmp :=
file_contexts.local.tmp :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := general_file_contexts.bin
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

general_file_contexts.tmp := $(intermediates)/general_file_contexts.tmp
$(general_file_contexts.tmp): $(addprefix $(LOCAL_PATH)/, file_contexts)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $< > $@

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_general_sepolicy)
$(LOCAL_BUILT_MODULE): $(general_file_contexts.tmp) $(built_general_sepolicy) $(HOST_OUT_EXECUTABLES)/sefcontext_compile $(HOST_OUT_EXECUTABLES)/checkfc
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc $(PRIVATE_SEPOLICY) $<
	$(hide) $(HOST_OUT_EXECUTABLES)/sefcontext_compile -o $@ $<

general_file_contexts.tmp :=

##################################
include $(CLEAR_VARS)
LOCAL_MODULE := seapp_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

all_sc_files := $(call build_policy, seapp_contexts)

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_sepolicy)
$(LOCAL_BUILT_MODULE): PRIVATE_SC_FILES := $(all_sc_files)
$(LOCAL_BUILT_MODULE): $(built_sepolicy) $(all_sc_files) $(HOST_OUT_EXECUTABLES)/checkseapp
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkseapp -p $(PRIVATE_SEPOLICY) -o $@ $(PRIVATE_SC_FILES)

built_sc := $(LOCAL_BUILT_MODULE)
all_sc_files :=

##################################
include $(CLEAR_VARS)
LOCAL_MODULE := general_seapp_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

all_sc_files := $(addprefix $(LOCAL_PATH)/, seapp_contexts)

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_general_sepolicy)
$(LOCAL_BUILT_MODULE): PRIVATE_SC_FILE := $(all_sc_files)
$(LOCAL_BUILT_MODULE): $(built_general_sepolicy) $(all_sc_files) $(HOST_OUT_EXECUTABLES)/checkseapp
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkseapp -p $(PRIVATE_SEPOLICY) -o $@ $(PRIVATE_SC_FILE)

all_sc_files :=

##################################
include $(CLEAR_VARS)
LOCAL_MODULE := general_seapp_neverallows
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(addprefix $(LOCAL_PATH)/, seapp_contexts)
	@mkdir -p $(dir $@)
	- $(hide) grep -ie '^neverallow' $< > $@


##################################
include $(CLEAR_VARS)

LOCAL_MODULE := property_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

all_pc_files := $(call build_policy, property_contexts)
all_pcfiles_with_nl := $(call add_nl, $(all_pc_files), $(built_nl))

property_contexts.tmp := $(intermediates)/property_contexts.tmp
$(property_contexts.tmp): PRIVATE_PC_FILES := $(all_pcfiles_with_nl)
$(property_contexts.tmp): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(property_contexts.tmp): $(all_pcfiles_with_nl)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $(PRIVATE_ADDITIONAL_M4DEFS) $(PRIVATE_PC_FILES) > $@


$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_sepolicy)
$(LOCAL_BUILT_MODULE): $(property_contexts.tmp) $(built_sepolicy) $(HOST_OUT_EXECUTABLES)/checkfc $(ACP)
	@mkdir -p $(dir $@)
	$(hide) $(ACP) $< $@
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc -p $(PRIVATE_SEPOLICY) $<

built_pc := $(LOCAL_BUILT_MODULE)
all_pc_files :=
all_pcfiles_with_nl :=
property_contexts.tmp :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := general_property_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

general_property_contexts.tmp := $(intermediates)/general_property_contexts.tmp
$(general_property_contexts.tmp): $(addprefix $(LOCAL_PATH)/, property_contexts)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $< > $@

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_general_sepolicy)
$(LOCAL_BUILT_MODULE): $(general_property_contexts.tmp) $(built_general_sepolicy) $(HOST_OUT_EXECUTABLES)/checkfc $(ACP)
	@mkdir -p $(dir $@)
	$(hide) $(ACP) $< $@
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc -p $(PRIVATE_SEPOLICY) $<

general_property_contexts.tmp :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := service_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

all_svc_files := $(call build_policy, service_contexts)
all_svcfiles_with_nl := $(call add_nl, $(all_svc_files), $(built_nl))

service_contexts.tmp := $(intermediates)/service_contexts.tmp
$(service_contexts.tmp): PRIVATE_SVC_FILES := $(all_svcfiles_with_nl)
$(service_contexts.tmp): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(service_contexts.tmp): $(all_svcfiles_with_nl)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $(PRIVATE_ADDITIONAL_M4DEFS) $(PRIVATE_SVC_FILES) > $@

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_sepolicy)
$(LOCAL_BUILT_MODULE): $(service_contexts.tmp) $(built_sepolicy) $(HOST_OUT_EXECUTABLES)/checkfc $(ACP)
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc -s $(PRIVATE_SEPOLICY) $<
	$(hide) $(ACP) $< $@

built_svc := $(LOCAL_BUILT_MODULE)
all_svc_files :=
all_svcfiles_with_nl :=
service_contexts.tmp :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := general_service_contexts
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

general_service_contexts.tmp := $(intermediates)/general_service_contexts.tmp
$(general_service_contexts.tmp): $(addprefix $(LOCAL_PATH)/, service_contexts)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $< > $@

$(LOCAL_BUILT_MODULE): PRIVATE_SEPOLICY := $(built_general_sepolicy)
$(LOCAL_BUILT_MODULE): $(general_service_contexts.tmp) $(built_general_sepolicy) $(HOST_OUT_EXECUTABLES)/checkfc $(ACP)
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/checkfc -s $(PRIVATE_SEPOLICY) $<
	$(hide) $(ACP) $< $@

general_service_contexts.tmp :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := mac_permissions.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/security

include $(BUILD_SYSTEM)/base_rules.mk

# Build keys.conf
mac_perms_keys.tmp := $(intermediates)/keys.tmp
$(mac_perms_keys.tmp): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$(mac_perms_keys.tmp): $(call build_policy, keys.conf)
	@mkdir -p $(dir $@)
	$(hide) m4 -s $(PRIVATE_ADDITIONAL_M4DEFS) $^ > $@

all_mac_perms_files := $(call build_policy, $(LOCAL_MODULE))

$(LOCAL_BUILT_MODULE): PRIVATE_MAC_PERMS_FILES := $(all_mac_perms_files)
$(LOCAL_BUILT_MODULE): $(mac_perms_keys.tmp) $(HOST_OUT_EXECUTABLES)/insertkeys.py $(all_mac_perms_files)
	@mkdir -p $(dir $@)
	$(hide) DEFAULT_SYSTEM_DEV_CERTIFICATE="$(dir $(DEFAULT_SYSTEM_DEV_CERTIFICATE))" \
		$(HOST_OUT_EXECUTABLES)/insertkeys.py -t $(TARGET_BUILD_VARIANT) -c $(TOP) $< -o $@ $(PRIVATE_MAC_PERMS_FILES)

mac_perms_keys.tmp :=
all_mac_perms_files :=

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := selinux_version
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): $(built_sepolicy) $(built_pc) $(built_fc) $(built_sc) $(built_svc)
	@mkdir -p $(dir $@)
	$(hide) echo -n $(BUILD_FINGERPRINT_FROM_FILE) > $@

##################################

build_policy :=
build_device_policy :=
sepolicy_build_files :=
built_sepolicy :=
built_sepolicy_recovery :=
built_sc :=
built_fc :=
built_pc :=
built_svc :=
built_general_sepolicy :=
built_general_sepolicy.conf :=
built_nl :=
add_nl :=

include $(call all-makefiles-under,$(LOCAL_PATH))
