# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The constants of verified boot.

# Recovery reason codes, copied from:
#     vboot_reference/firmware/include/vboot_nvstorage.h
#     vboot_reference/firmware/include/vboot_struct.h
RECOVERY_REASON = {
    # Recovery not requested
    'NOT_REQUESTED':      '0',   # 0x00
    # Recovery requested from legacy utility
    'LEGACY':             '1',   # 0x01
    # User manually requested recovery via recovery button
    'RO_MANUAL':          '2',   # 0x02
    # RW firmware failed signature check
    'RO_INVALID_RW':      '3',   # 0x03
    # S3 resume failed
    'RO_S3_RESUME':       '4',   # 0x04
    # TPM error in read-only firmware (deprecated)
    'DEP_RO_TPM_ERROR':   '5',   # 0x05
    # Shared data error in read-only firmware
    'RO_SHARED_DATA':     '6',   # 0x06
    # Test error from S3Resume()
    'RO_TEST_S3':         '7',   # 0x07
    # Test error from LoadFirmwareSetup()
    'RO_TEST_LFS':        '8',   # 0x08
    # Test error from LoadFirmware()
    'RO_TEST_LF':         '9',   # 0x09
    # RW firmware failed signature check
    'RW_NOT_DONE':        '16',  # 0x10
    'RW_DEV_MISMATCH':    '17',  # 0x11
    'RW_REC_MISMATCH':    '18',  # 0x12
    'RW_VERIFY_KEYBLOCK': '19',  # 0x13
    'RW_KEY_ROLLBACK':    '20',  # 0x14
    'RW_DATA_KEY_PARSE':  '21',  # 0x15
    'RW_VERIFY_PREAMBLE': '22',  # 0x16
    'RW_FW_ROLLBACK':     '23',  # 0x17
    'RW_HEADER_VALID':    '24',  # 0x18
    'RW_GET_FW_BODY':     '25',  # 0x19
    'RW_HASH_WRONG_SIZE': '26',  # 0x1A
    'RW_VERIFY_BODY':     '27',  # 0x1B
    'RW_VALID':           '28',  # 0x1C
    # Read-only normal path requested by firmware preamble, but
    # unsupported by firmware.
    'RW_NO_RO_NORMAL':    '29',  # 0x1D
    # Firmware boot failure outside of verified boot
    'RO_FIRMWARE':        '32',  # 0x20
    # Recovery mode TPM initialization requires a system reboot.
    # The system was already in recovery mode for some other reason
    # when this happened.
    'RO_TPM_REBOOT':      '33',  # 0x21
    # EC software sync - other error
    'EC_SOFTWARE_SYNC':   '34',  # 0x22
    # EC software sync - unable to determine active EC image
    'EC_UNKNOWN_IMAGE':   '35',  # 0x23
    # EC software sync - error obtaining EC image hash (deprecated)
    'DEP_EC_HASH':        '36',  # 0x24
    # EC software sync - error obtaining expected EC image
    'EC_EXPECTED_IMAGE':  '37',  # 0x25
    # EC software sync - error updating EC
    'EC_UPDATE':          '38',  # 0x26
    # EC software sync - unable to jump to EC-RW
    'EC_JUMP_RW':         '39',  # 0x27
    # EC software sync - unable to protect / unprotect EC-RW
    'EC_PROTECT':         '40',  # 0x28
    # Unspecified/unknown error in read-only firmware
    'RO_UNSPECIFIED':     '63',  # 0x3F
    # User manually requested recovery by pressing a key at developer
    # warning screen.
    'RW_DEV_SCREEN':      '65',  # 0x41
    # No OS kernel detected
    'RW_NO_OS':           '66',  # 0x42
    # OS kernel failed signature check
    'RW_INVALID_OS':      '67',  # 0x43
    # TPM error in rewritable firmware (deprecated)
    'DEP_RW_TPM_ERROR':   '68',  # 0x44
    # RW firmware in dev mode, but dev switch is off.
    'RW_DEV_MISMATCH':    '69',  # 0x45
    # Shared data error in rewritable firmware
    'RW_SHARED_DATA':     '70',  # 0x46
    # Test error from LoadKernel()
    'RW_TEST_LK':         '71',  # 0x47
    # No bootable disk found (deprecated)
    'DEP_RW_NO_DISK':     '72',  # 0x48
    # Rebooting did not correct TPM_E_FAIL or TPM_E_FAILEDSELFTEST
    'TPM_E_FAIL':         '73',  # 0x49
    # TPM setup error in read-only firmware
    'RO_TPM_S_ERROR':     '80',  # 0x50
    # TPM write error in read-only firmware
    'RO_TPM_W_ERROR':     '81',  # 0x51
    # TPM lock error in read-only firmware
    'RO_TPM_L_ERROR':     '82',  # 0x52
    # TPM update error in read-only firmware
    'RO_TPM_U_ERROR':     '83',  # 0x53
    # TPM read error in rewritable firmware
    'RW_TPM_R_ERROR':     '84',  # 0x54
    # TPM write error in rewritable firmware
    'RW_TPM_W_ERROR':     '85',  # 0x55
    # TPM lock error in rewritable firmware
    'RW_TPM_L_ERROR':     '86',  # 0x56
    # EC software sync unable to get EC image hash
    'EC_HASH_FAILED':     '87',  # 0x57
    # EC software sync invalid image hash size
    'EC_HASH_SIZE':       '88',  # 0x58
    # Unspecified error while trying to load kernel
    'LK_UNSPECIFIED':     '89',  # 0x59
    # No bootable storage device in system
    'RW_NO_DISK':         '90',  # 0x5A
    # No bootable kernel found on disk
    'RW_NO_KERNEL':       '91',  # 0x5B
    # Unspecified/unknown error in rewritable firmware
    'RW_UNSPECIFIED':     '127', # 0x7F
    # DM-verity error
    'KE_DM_VERITY':       '129', # 0x81
    # Unspecified/unknown error in kernel
    'KE_UNSPECIFIED':     '191', # 0xBF
    # Recovery mode test from user-mode
    'US_TEST':            '193', # 0xC1
    # Unspecified/unknown error in user-mode
    'US_UNSPECIFIED':     '255', # 0xFF
}

# GBB flags, copied from:
#     vboot_reference/firmware/include/gbb_header.h
GBB_FLAG_DEV_SCREEN_SHORT_DELAY    = 0x00000001
GBB_FLAG_LOAD_OPTION_ROMS          = 0x00000002
GBB_FLAG_ENABLE_ALTERNATE_OS       = 0x00000004
GBB_FLAG_FORCE_DEV_SWITCH_ON       = 0x00000008
GBB_FLAG_FORCE_DEV_BOOT_USB        = 0x00000010
GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK = 0x00000020
GBB_FLAG_ENTER_TRIGGERS_TONORM     = 0x00000040
GBB_FLAG_FORCE_DEV_BOOT_LEGACY     = 0x00000080
GBB_FLAG_FAFT_KEY_OVERIDE          = 0x00000100
GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC  = 0x00000200
GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY   = 0x00000400
GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC  = 0x00000800
GBB_FLAG_DISABLE_LID_SHUTDOWN      = 0x00001000
GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP = 0x00002000


# VbSharedData flags, copied from:
#     vboot_reference/firmware/include/vboot_struct.h
VDAT_FLAG_FWB_TRIED                = 0x00000001
VDAT_FLAG_KERNEL_KEY_VERIFIED      = 0x00000002
VDAT_FLAG_LF_DEV_SWITCH_ON         = 0x00000004
VDAT_FLAG_LF_USE_RO_NORMAL         = 0x00000008
VDAT_FLAG_BOOT_DEV_SWITCH_ON       = 0x00000010
VDAT_FLAG_BOOT_REC_SWITCH_ON       = 0x00000020
VDAT_FLAG_BOOT_FIRMWARE_WP_ENABLED = 0x00000040
VDAT_FLAG_BOOT_S3_RESUME           = 0x00000100
VDAT_FLAG_BOOT_RO_NORMAL_SUPPORT   = 0x00000200
VDAT_FLAG_HONOR_VIRT_DEV_SWITCH    = 0x00000400
VDAT_FLAG_EC_SOFTWARE_SYNC         = 0x00000800
VDAT_FLAG_EC_SLOW_UPDATE           = 0x00001000

# Firmware preamble flags, copied from:
#     vboot_reference/firmware/include/vboot_struct.h
PREAMBLE_USE_RO_NORMAL             = 0x00000001
