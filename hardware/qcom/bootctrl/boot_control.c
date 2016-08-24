/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>
#define LOG_TAG "bootcontrolhal"
#include <cutils/log.h>
#include <hardware/boot_control.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "gpt-utils.h"

#define BOOTDEV_DIR "/dev/block/bootdevice/by-name"
#define BOOT_IMG_PTN_NAME "boot"
#define LUN_NAME_END_LOC 14

const char *slot_suffix_arr[] = {
	AB_SLOT_A_SUFFIX,
	AB_SLOT_B_SUFFIX,
	NULL};

enum part_attr_type {
	ATTR_SLOT_ACTIVE = 0,
	ATTR_BOOT_SUCCESSFUL,
	ATTR_UNBOOTABLE,
};

void boot_control_init(struct boot_control_module *module)
{
	if (!module) {
		ALOGE("Invalid argument passed to %s", __func__);
		return;
	}
	return;
}

//Get the value of one of the attribute fields for a partition.
static int get_partition_attribute(char *partname,
		enum part_attr_type part_attr)
{
	struct gpt_disk *disk = NULL;
	uint8_t *pentry = NULL;
	int retval = -1;
	uint8_t *attr = NULL;
	if (!partname)
		goto error;
	disk = gpt_disk_alloc();
	if (!disk) {
		ALOGE("%s: Failed to alloc disk struct", __func__);
		goto error;
	}
	if (gpt_disk_get_disk_info(partname, disk)) {
		ALOGE("%s: Failed to get disk info", __func__);
		goto error;
	}
	pentry = gpt_disk_get_pentry(disk, partname, PRIMARY_GPT);
	if (!pentry) {
		ALOGE("%s: pentry does not exist in disk struct",
				__func__);
		goto error;
	}
	attr = pentry + AB_FLAG_OFFSET;
	if (part_attr == ATTR_SLOT_ACTIVE)
		retval = !!(*attr & AB_PARTITION_ATTR_SLOT_ACTIVE);
	else if (part_attr == ATTR_BOOT_SUCCESSFUL)
		retval = !!(*attr & AB_PARTITION_ATTR_BOOT_SUCCESSFUL);
	else if (part_attr == ATTR_UNBOOTABLE)
		retval = !!(*attr & AB_PARTITION_ATTR_UNBOOTABLE);
	else
		retval = -1;
	gpt_disk_free(disk);
	return retval;
error:
	if (disk)
		gpt_disk_free(disk);
	return retval;
}

//Set a particular attribute for all the partitions in a
//slot
static int update_slot_attribute(const char *slot,
		enum part_attr_type ab_attr)
{
	unsigned int i = 0;
	char buf[PATH_MAX];
	struct stat st;
	struct gpt_disk *disk = NULL;
	uint8_t *pentry = NULL;
	uint8_t *pentry_bak = NULL;
	int rc = -1;
	uint8_t *attr = NULL;
	uint8_t *attr_bak = NULL;
	char partName[MAX_GPT_NAME_SIZE + 1] = {0};
	const char ptn_list[][MAX_GPT_NAME_SIZE] = { AB_PTN_LIST };
	int slot_name_valid = 0;
	if (!slot) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	for (i = 0; slot_suffix_arr[i] != NULL; i++)
	{
		if (!strncmp(slot, slot_suffix_arr[i],
					strlen(slot_suffix_arr[i])))
				slot_name_valid = 1;
	}
	if (!slot_name_valid) {
		ALOGE("%s: Invalid slot name", __func__);
		goto error;
	}
	for (i=0; i < ARRAY_SIZE(ptn_list); i++) {
		memset(buf, '\0', sizeof(buf));
		//Check if A/B versions of this ptn exist
		snprintf(buf, sizeof(buf) - 1,
                                        "%s/%s%s",
                                        BOOT_DEV_DIR,
                                        ptn_list[i],
					AB_SLOT_A_SUFFIX
					);
		if (stat(buf, &st)) {
			//partition does not have _a version
			continue;
		}
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, sizeof(buf) - 1,
                                        "%s/%s%s",
                                        BOOT_DEV_DIR,
                                        ptn_list[i],
					AB_SLOT_B_SUFFIX
					);
		if (stat(buf, &st)) {
			//partition does not have _a version
			continue;
		}
		memset(partName, '\0', sizeof(partName));
		snprintf(partName,
				sizeof(partName) - 1,
				"%s%s",
				ptn_list[i],
				slot);
		disk = gpt_disk_alloc(disk);
		if (!disk) {
			ALOGE("%s: Failed to alloc disk struct",
					__func__);
			goto error;
		}
		rc = gpt_disk_get_disk_info(partName, disk);
		if (rc != 0) {
			ALOGE("%s: Failed to get disk info for %s",
					__func__,
					partName);
			goto error;
		}
		pentry = gpt_disk_get_pentry(disk, partName, PRIMARY_GPT);
		pentry_bak = gpt_disk_get_pentry(disk, partName, SECONDARY_GPT);
		if (!pentry || !pentry_bak) {
			ALOGE("%s: Failed to get pentry/pentry_bak for %s",
					__func__,
					partName);
			goto error;
		}
		attr = pentry + AB_FLAG_OFFSET;
		attr_bak = pentry_bak + AB_FLAG_OFFSET;
		if (ab_attr == ATTR_BOOT_SUCCESSFUL) {
			*attr = (*attr) | AB_PARTITION_ATTR_BOOT_SUCCESSFUL;
			*attr_bak = (*attr_bak) |
				AB_PARTITION_ATTR_BOOT_SUCCESSFUL;
		} else if (ab_attr == ATTR_UNBOOTABLE) {
			*attr = (*attr) | AB_PARTITION_ATTR_UNBOOTABLE;
			*attr_bak = (*attr_bak) | AB_PARTITION_ATTR_UNBOOTABLE;
		} else if (ab_attr == ATTR_SLOT_ACTIVE) {
			*attr = (*attr) | AB_PARTITION_ATTR_SLOT_ACTIVE;
			*attr_bak = (*attr) | AB_PARTITION_ATTR_SLOT_ACTIVE;
		} else {
			ALOGE("%s: Unrecognized attr", __func__);
			goto error;
		}
		if (gpt_disk_update_crc(disk)) {
			ALOGE("%s: Failed to update crc for %s",
					__func__,
					partName);
			goto error;
		}
		if (gpt_disk_commit(disk)) {
			ALOGE("%s: Failed to write back entry for %s",
					__func__,
					partName);
			goto error;
		}
		gpt_disk_free(disk);
		disk = NULL;
	}
	return 0;
error:
	if (disk)
		gpt_disk_free(disk);
	return -1;
}

unsigned get_number_slots(struct boot_control_module *module)
{
	struct dirent *de = NULL;
	DIR *dir_bootdev = NULL;
	unsigned slot_count = 0;
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	dir_bootdev = opendir(BOOTDEV_DIR);
	if (!dir_bootdev) {
		ALOGE("%s: Failed to open bootdev dir (%s)",
				__func__,
				strerror(errno));
		goto error;
	}
	while ((de = readdir(dir_bootdev))) {
		if (de->d_name[0] == '.')
			continue;
		if (!strncmp(de->d_name, BOOT_IMG_PTN_NAME,
					strlen(BOOT_IMG_PTN_NAME)))
			slot_count++;
	}
	closedir(dir_bootdev);
	return slot_count;
error:
	if (dir_bootdev)
		closedir(dir_bootdev);
	return 0;
}

unsigned get_current_slot(struct boot_control_module *module)
{
	uint32_t num_slots = 0;
	char bootPartition[MAX_GPT_NAME_SIZE + 1];
	unsigned i = 0;
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	num_slots = get_number_slots(module);
	if (num_slots <= 1) {
		//Slot 0 is the only slot around.
		return 0;
	}
	//Iterate through a list of partitons named as boot+suffix
	//and see which one is currently active.
	for (i = 0; slot_suffix_arr[i] != NULL ; i++) {
		memset(bootPartition, '\0', sizeof(bootPartition));
		snprintf(bootPartition, sizeof(bootPartition) - 1,
				"boot%s",
				slot_suffix_arr[i]);
		if (get_partition_attribute(bootPartition,
					ATTR_SLOT_ACTIVE) == 1)
			return i;
	}
error:
	//The HAL spec requires that we return a number between
	//0 to num_slots - 1. Since something went wrong here we
	//are just going to return the default slot.
	return 0;
}

int mark_boot_successful(struct boot_control_module *module)
{
	unsigned cur_slot = 0;
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	cur_slot = get_current_slot(module);
	if (update_slot_attribute(slot_suffix_arr[cur_slot],
				ATTR_BOOT_SUCCESSFUL)) {
		goto error;
	}
	return 0;
error:
	ALOGE("%s: Failed to mark boot successful", __func__);
	return -1;
}

const char *get_suffix(struct boot_control_module *module, unsigned slot)
{
	unsigned num_slots = 0;
	if (!module) {
		ALOGE("%s: Invalid arg", __func__);
	}
	num_slots = get_number_slots(module);
	if (num_slots < 1 || slot > num_slots - 1)
		return NULL;
	else
		return slot_suffix_arr[slot];
}

int set_active_boot_slot(struct boot_control_module *module, unsigned slot)
{
	const char ptn_list[][MAX_GPT_NAME_SIZE] = { AB_PTN_LIST };
	char slotA[MAX_GPT_NAME_SIZE + 1] = {0};
	char slotB[MAX_GPT_NAME_SIZE + 1] = {0};
	char active_guid[TYPE_GUID_SIZE + 1] = {0};
	char inactive_guid[TYPE_GUID_SIZE + 1] = {0};
	struct gpt_disk *disk = NULL;
	//Pointer to partition entry of current 'A' partition
	uint8_t *pentryA = NULL;
	uint8_t *pentryA_bak = NULL;
	//Pointer to partition entry of current 'B' partition
	uint8_t *pentryB = NULL;
	uint8_t *pentryB_bak = NULL;
	uint8_t *slot_info = NULL;
	uint32_t i;
	int rc = -1;
	char buf[PATH_MAX] = {0};
	struct stat st;
	unsigned num_slots = 0;
	unsigned current_slot = 0;
	int is_ufs = gpt_utils_is_ufs_device();

	if (!module) {
		ALOGE("%s: Invalid arg", __func__);
		goto error;
	}
	num_slots = get_number_slots(module);
	if ((num_slots < 1) || (slot > num_slots - 1)) {
		ALOGE("%s: Unable to get num slots/Invalid slot value",
				__func__);
		goto error;
	}
        current_slot = get_current_slot(module);
	if (current_slot == slot) {
		//Nothing to do here. Just return
		return 0;
	}
	for (i=0; i < ARRAY_SIZE(ptn_list); i++) {
		//XBL is handled differrently for ufs devices
		if (is_ufs && !strncmp(ptn_list[i], PTN_XBL, strlen(PTN_XBL)))
				continue;
		memset(buf, '\0', sizeof(buf));
		//Check if A/B versions of this ptn exist
		snprintf(buf, sizeof(buf) - 1,
                                        "%s/%s%s",
                                        BOOT_DEV_DIR,
                                        ptn_list[i],
					AB_SLOT_A_SUFFIX
					);
		if (stat(buf, &st)) {
			//partition does not have _a version
			continue;
		}
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, sizeof(buf) - 1,
                                        "%s/%s%s",
                                        BOOT_DEV_DIR,
                                        ptn_list[i],
					AB_SLOT_B_SUFFIX
					);
		if (stat(buf, &st)) {
			//partition does not have _a version
			continue;
		}
		disk = gpt_disk_alloc();
		if (!disk)
			goto error;
		memset(slotA, 0, sizeof(slotA));
		memset(slotB, 0, sizeof(slotB));
		snprintf(slotA, sizeof(slotA) - 1, "%s%s",
				ptn_list[i],
				AB_SLOT_A_SUFFIX);
		snprintf(slotB, sizeof(slotB) - 1,"%s%s",
				ptn_list[i],
				AB_SLOT_B_SUFFIX);
		//It is assumed that both the A and B slots reside on the
		//same physical disk
		if (gpt_disk_get_disk_info(slotA, disk))
			goto error;
		//Get partition entry for slot A from primary table
		pentryA = gpt_disk_get_pentry(disk, slotA, PRIMARY_GPT);
		//Get partition entry for slot A from backup table
		pentryA_bak = gpt_disk_get_pentry(disk, slotA, SECONDARY_GPT);
		//Get partition entry for slot B from primary table
		pentryB = gpt_disk_get_pentry(disk, slotB, PRIMARY_GPT);
		//Get partition entry for slot B from backup table
		pentryB_bak = gpt_disk_get_pentry(disk, slotB, SECONDARY_GPT);
		if ( !pentryA || !pentryA_bak || !pentryB || !pentryB_bak) {
			//Something has gone wrong here.We know that we have
			//_a and _b versions of this partition due to the
			//check at the start of the loop so none of these
			//should be NULL.
			ALOGE("Slot pentries for %s not found.",
					ptn_list[i]);
			goto error;
		}
		memset(active_guid, '\0', sizeof(active_guid));
		memset(inactive_guid, '\0', sizeof(inactive_guid));
		if (get_partition_attribute(slotA, ATTR_SLOT_ACTIVE) == 1) {
			//A is the current active slot
			memcpy((void*)active_guid,
					(const void*)pentryA,
					TYPE_GUID_SIZE);
			memcpy((void*)inactive_guid,
					(const void*)pentryB,
					TYPE_GUID_SIZE);

		} else if (get_partition_attribute(slotB,
					ATTR_SLOT_ACTIVE) == 1) {
			//B is the current active slot
			memcpy((void*)active_guid,
					(const void*)pentryB,
					TYPE_GUID_SIZE);
			memcpy((void*)inactive_guid,
					(const void*)pentryA,
					TYPE_GUID_SIZE);
		} else {
			ALOGE("Both A & B are inactive..Aborting");
			goto error;
		}
		if (!strncmp(slot_suffix_arr[slot], AB_SLOT_A_SUFFIX,
					strlen(AB_SLOT_A_SUFFIX))){
			//Mark A as active in primary table
			memcpy(pentryA, active_guid, TYPE_GUID_SIZE);
			slot_info = pentryA + AB_FLAG_OFFSET;
			*slot_info = AB_SLOT_ACTIVE_VAL;

			//Mark A as active in backup table
			memcpy(pentryA_bak, active_guid, TYPE_GUID_SIZE);
			slot_info = pentryA_bak + AB_FLAG_OFFSET;
			*slot_info = AB_SLOT_ACTIVE_VAL;

			//Mark B as inactive in primary table
			memcpy(pentryB, inactive_guid, TYPE_GUID_SIZE);
			slot_info = pentryB + AB_FLAG_OFFSET;
			*slot_info = *(slot_info) &
				~AB_PARTITION_ATTR_SLOT_ACTIVE;

			//Mark B as inactive in backup table
			memcpy(pentryB_bak, inactive_guid, TYPE_GUID_SIZE);
			slot_info = pentryB_bak + AB_FLAG_OFFSET;
			*slot_info = *(slot_info) &
				~AB_PARTITION_ATTR_SLOT_ACTIVE;
		} else if (!strncmp(slot_suffix_arr[slot], AB_SLOT_B_SUFFIX,
					strlen(AB_SLOT_B_SUFFIX))){
			//Mark B as active in primary table
			memcpy(pentryB, active_guid, TYPE_GUID_SIZE);
			slot_info = pentryB + AB_FLAG_OFFSET;
			*slot_info = AB_SLOT_ACTIVE_VAL;

			//Mark B as active in backup table
			memcpy(pentryB_bak, active_guid, TYPE_GUID_SIZE);
			slot_info = pentryB_bak + AB_FLAG_OFFSET;
			*slot_info = AB_SLOT_ACTIVE_VAL;

			//Mark A as inavtive in primary table
			memcpy(pentryA, inactive_guid, TYPE_GUID_SIZE);
			slot_info = pentryA + AB_FLAG_OFFSET;
			*slot_info = *(slot_info) &
				~AB_PARTITION_ATTR_SLOT_ACTIVE;

			//Mark A as inactive in backup table
			memcpy(pentryA_bak, inactive_guid, TYPE_GUID_SIZE);
			slot_info = pentryA_bak + AB_FLAG_OFFSET;
			*slot_info = *(slot_info) &
				~AB_PARTITION_ATTR_SLOT_ACTIVE;
		} else {
			//Something has gone terribly terribly wrong
			ALOGE("%s: Unknown slot suffix!", __func__);
			goto error;
		}
		if (gpt_disk_update_crc(disk) != 0) {
			ALOGE("%s: Failed to update gpt_disk crc", __func__);
			goto error;
		}
		if (gpt_disk_commit(disk) != 0) {
			ALOGE("%s: Failed to commit disk info", __func__);
			goto error;
		}
		gpt_disk_free(disk);
		disk = NULL;
	}
	if (is_ufs) {
		if (!strncmp(slot_suffix_arr[slot], AB_SLOT_A_SUFFIX,
					strlen(AB_SLOT_A_SUFFIX))){
			//Set xbl_a as the boot lun
			rc = gpt_utils_set_xbl_boot_partition(NORMAL_BOOT);
		} else if (!strncmp(slot_suffix_arr[slot], AB_SLOT_B_SUFFIX,
					strlen(AB_SLOT_B_SUFFIX))){
			//Set xbl_b as the boot lun
			rc = gpt_utils_set_xbl_boot_partition(BACKUP_BOOT);
		} else {
			//Something has gone terribly terribly wrong
			ALOGE("%s: Unknown slot suffix!", __func__);
			goto error;
		}
		if (rc) {
			ALOGE("%s: Failed to switch xbl boot partition",
					__func__);
			goto error;
		}
	}
	return 0;
error:
	if (disk)
		gpt_disk_free(disk);
	return -1;
}

int set_slot_as_unbootable(struct boot_control_module *module, unsigned slot)
{
	unsigned num_slots = 0;
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	num_slots = get_number_slots(module);
	if (num_slots < 1 || slot > num_slots - 1) {
		ALOGE("%s: Unable to get num_slots/Invalid slot value",
				__func__);
		goto error;
	}
	if (update_slot_attribute(slot_suffix_arr[slot],
				ATTR_UNBOOTABLE)) {
		goto error;
	}
	return 0;
error:
	ALOGE("%s: Failed to mark slot unbootable", __func__);
	return -1;
}

int is_slot_bootable(struct boot_control_module *module, unsigned slot)
{
	unsigned num_slots = 0;
	int attr = 0;
	char bootPartition[MAX_GPT_NAME_SIZE + 1] = {0};
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	num_slots = get_number_slots(module);
	if (num_slots < 1 || slot > num_slots - 1) {
		ALOGE("%s: Unable to get num_slots/Invalid slot value",
				__func__);
		goto error;
	}
	snprintf(bootPartition,
			sizeof(bootPartition) - 1, "boot%s",
			slot_suffix_arr[slot]);
	attr = get_partition_attribute(bootPartition, ATTR_UNBOOTABLE);
	if (attr >= 0)
		return !attr;
error:
	return -1;
}

int is_slot_marked_successful(struct boot_control_module *module, unsigned slot)
{
	unsigned num_slots = 0;
	int attr = 0;
	char bootPartition[MAX_GPT_NAME_SIZE + 1] = {0};
	if (!module) {
		ALOGE("%s: Invalid argument", __func__);
		goto error;
	}
	num_slots = get_number_slots(module);
	if (num_slots < 1 || slot > num_slots - 1) {
		ALOGE("%s: Unable to get num_slots/Invalid slot value",
				__func__);
		goto error;
	}
	snprintf(bootPartition,
			sizeof(bootPartition) - 1,
			"boot%s", slot_suffix_arr[slot]);
	attr = get_partition_attribute(bootPartition, ATTR_BOOT_SUCCESSFUL);
	if (attr >= 0)
		return attr;
error:
	return -1;
}

static hw_module_methods_t boot_control_module_methods = {
	.open = NULL,
};

boot_control_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = 1,
		.hal_api_version = 0,
		.id = BOOT_CONTROL_HARDWARE_MODULE_ID,
		.name = "Boot control HAL",
		.author = "Code Aurora Forum",
		.methods = &boot_control_module_methods,
	},
	.init = boot_control_init,
	.getNumberSlots = get_number_slots,
	.getCurrentSlot = get_current_slot,
	.markBootSuccessful = mark_boot_successful,
	.setActiveBootSlot = set_active_boot_slot,
	.setSlotAsUnbootable = set_slot_as_unbootable,
	.isSlotBootable = is_slot_bootable,
	.getSuffix = get_suffix,
	.isSlotMarkedSuccessful = is_slot_marked_successful,
};
