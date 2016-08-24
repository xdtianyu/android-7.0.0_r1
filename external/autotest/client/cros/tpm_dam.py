# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Provides a utility function for working with TPM DAM logic.

Dictionary Attack Mitigation (DAM) logic causes TPMs to enter a locked down
state to defend against dictionary attacks. Authentication failures cause a
counter to increment and when the counter exceeds some threshold, the defense
mechanism is triggered.
"""

import os, re

from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros import service_stopper

def get_dictionary_attack_counter():
    """Returns the current dictionary attack counter."""
    tpm_command_info = {
        '0x49465800': {  # Infineon
            'command': ('00 c1 '         # Tag = TPM_TAG_RQU_COMMAND
                        '00 00 00 16 '   # Size = 22
                        '00 00 00 65 '   # Ordinal = TPM_ORD_GetCapability
                        '00 00 00 10 '   # Capability Area = TPM_CAP_MFR
                        '00 00 00 04 '   # Size = 4
                        '00 00 08 02'),  # Vendor-specific
            'response_offset': 23},      # Vendor-specific
        '0x57454300': {  # Nuvoton
            'command': ('00 c1 '         # Tag = TPM_TAG_RQU_COMMAND
                        '00 00 00 14 '   # Size = 20
                        '00 00 00 65 '   # Ordinal = TPM_ORD_GetCapability
                        '00 00 00 19 '   # Capability Area = TPM_CAP_DA_LOGIC
                        '00 00 00 02 '   # Size = 2
                        '00 04'),        # Entity Type = TPM_ET_SRK
            'response_offset': 18},      # TPM_DA_INFO.currentCount LSB
        '0x53544d20': {  # STMicro
            'command': ('00 c1 '         # Tag = TPM_TAG_RQU_COMMAND
                        '00 00 00 14 '   # Size = 20
                        '00 00 00 65 '   # Ordinal = TPM_ORD_GetCapability
                        '00 00 00 19 '   # Capability Area = TPM_CAP_DA_LOGIC
                        '00 00 00 02 '   # Size = 2
                        '00 04'),        # Entity Type = TPM_ET_SRK
            'response_offset': 18}}      # TPM_DA_INFO.currentCount LSB
    caps_file='/sys/class/misc/tpm0/device/caps'
    if not os.path.exists(caps_file):
        caps_file='/sys/class/tpm/tpm0/device/caps'
    try:
        with open(caps_file, 'r') as fp:
            caps = fp.read()
    except IOError:
        return 'Could not read TPM device caps.'
    match = re.search(r'Manufacturer: (0x[0-9A-Fa-f]*)', caps)
    if not match:
        return 'Could not find TPM manufacturer.'
    manufacturer = match.group(1)
    if manufacturer not in tpm_command_info:
        return 'TPM manufacturer not supported.'
    with service_stopper.ServiceStopper(['cryptohomed',
                                         'chapsd',
                                         'tcsd']):
        # The output of 'tpmc raw' is a series of bytes in the form
        # '0x00 0x01 0x02 ...'.
        tpm_response = utils.system_output(
                'tpmc raw %s' % tpm_command_info[manufacturer]['command'],
                ignore_status=True).split()
    offset = tpm_command_info[manufacturer]['response_offset']
    if (len(tpm_response) <= offset):
        return 'Unexpected TPM response (length = %d).' % len(tpm_response)
    return int(tpm_response[offset], base=16)


