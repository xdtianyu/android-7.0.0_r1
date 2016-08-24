/*
 * Copyright (C) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* THE HAL BOOTCTRL HEADER MUST BE IN SYNC WITH THE UBOOT BOOTCTRL HEADER */

#ifndef _BOOTCTRL_H_
#define _BOOTCTRL_H_

#include <stdint.h>

/* struct boot_ctrl occupies the slot_suffix field of
 * struct bootloader_message */
#define OFFSETOF_SLOT_SUFFIX 864

#define BOOTCTRL_MAGIC 0x42424100
#define BOOTCTRL_SUFFIX_A           "_a"
#define BOOTCTRL_SUFFIX_B           "_b"

#define BOOT_CONTROL_VERSION    1

typedef struct slot_metadata {
    uint8_t priority : 4;
    uint8_t tries_remaining : 3;
    uint8_t successful_boot : 1;
} slot_metadata_t;

typedef struct boot_ctrl {
    /* Magic for identification - '\0ABB' (Boot Contrl Magic) */
    uint32_t magic;

    /* Version of struct. */
    uint8_t version;

    /* Information about each slot. */
    slot_metadata_t slot_info[2];

    uint8_t recovery_tries_remaining;
} boot_ctrl_t;
#endif /* _BOOTCTRL_H_ */
