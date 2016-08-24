# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Python library to interact with TPM module for testing.

Background
 - TPM stands for Trusted Platform Module, a piece of security device
 - TPM specification is the work of Trusted Computing Group
 - As of September 2011, the current TPM specification is version 1.2

Dependency
 - This library depends on a C shared library called "libtspi.so", which
   contains a set of APIs for interacting with TPM module

Notes:
 - An exception is raised if it doesn't make logical sense to continue program
   flow (e.g. I/O error prevents test case from executing)
 - An exception is caught and then converted to an error code if the caller
   expects to check for error code per API definition
"""

import datetime, logging

from autotest_lib.client.common_lib import smogcheck_ttci
# Use explicit import to make code more readable
from ctypes import c_uint, c_uint32, cdll, c_bool, Structure, POINTER, \
    c_ubyte, c_byte, byref, c_uint16, cast, create_string_buffer, c_uint64, \
    c_char_p, addressof, c_char, pointer

# TPM flags
# TODO(tgao): possible to import from trousers/src/include/tss/tss_defines.h?
TSS_KEY_AUTHORIZATION = c_uint32(0x00000001)
TSS_KEY_TSP_SRK = c_uint32(0x04000000)
TSS_POLICY_USAGE = c_uint32(0x00000001)
TSS_OBJECT_TYPE_RSAKEY = c_uint(0x02)
TSS_SECRET_MODE_SHA1 = c_uint32(0x00001000)
TSS_SECRET_MODE_PLAIN = c_uint32(0x00001800)
TSS_TPMCAP_PROP_MANUFACTURER = c_uint(0x12)
TSS_TPMCAP_PROPERTY = c_uint(0x13)
TSS_TPMCAP_VERSION = c_uint(0x14)
TSS_TPMCAP_VERSION_VAL = c_uint32(0x15)
TSS_TPMSTATUS_DISABLEOWNERCLEAR = c_uint32(0x00000001)
TSS_TPMSTATUS_DISABLEFORCECLEAR = c_uint32(0x00000002)
TSS_TPMSTATUS_PHYSICALSETDEACTIVATED = c_uint32(0x00000010)
TSS_TPMSTATUS_SETTEMPDEACTIVATED = c_uint32(0x00000011)

# TODO(tgao): possible to import from trousers/src/include/tss/tpm.h?
TPM_SHA1_160_HASH_LEN = c_uint(0x14)

# Path to TSPI shared library.
TSPI_C_LIB = "/usr/lib/libtspi.so.1"

# Valid operation of tpmSetActive(). Equivalent CLI commands:
# 'status' = tpm_setactive --well-known --status
# 'activate' = tpm_setactive --well-known --active
# 'deactivate' = tpm_setactive --well-known --inactive
# 'temp' = tpm_setactive --well-known --temp
TPM_SETACTIVE_OP = ['status', 'activate', 'deactivate', 'temp']

# Valid operation of tpmSetClearable(). Equivalent CLI commands:
# 'status' = tpm_setclearable --well-known --status
# 'owner' = tpm_setclearable --well-known --owner
# 'force' = tpm_setclearable --well-known --force
TPM_SETCLEARABLE_OP = ['status', 'owner', 'force']

# Secret mode for setPolicySecret()
TSS_SECRET_MODE = dict(sha1=TSS_SECRET_MODE_SHA1,
                       plain=TSS_SECRET_MODE_PLAIN)


class SmogcheckError(Exception):
    """Base class for all smogcheck API errors."""


class TpmVersion(Structure):
    """Defines TPM version string struct.

    Declared in tss/tpm.h and named TPM_VERSION.
    """
    _fields_ = [('major', c_ubyte),
                ('minor', c_ubyte),
                ('revMajor', c_ubyte),
                ('revMinor', c_ubyte)]


class TpmCapVersionInfo(Structure):
    """Defines TPM version info struct.

    Declared in tss/tpm.h and named TPM_CAP_VERSION_INFO.
    """
    _fields_ = [('tag', c_uint16),
                ('version', TpmVersion),
                ('specLevel', c_uint16),
                ('errataRev', c_ubyte),
                ('tpmVendorID', c_char*4),
                ('vendorSpecific', POINTER(c_ubyte))]


def InitVersionInfo(vi):
    """Utility method to allocate memory for TPM version info.

    Args:
      vi: a TpmCapVerisonInfo object, just created.
    """
    vi.tpmVendorId = create_string_buffer(4)  # Allocate 4 bytes
    vendorDetail = create_string_buffer(64)   # Allocate 64 bytes
    vi.vendorSpecific = cast(pointer(vendorDetail), POINTER(c_ubyte))


def PrintVersionInfo(vi):
    """Utility method to print TPM version info.

    Args:
      vi: a TpmCapVerisonInfo object.
    """
    logging.info('  TPM 1.2 Version Info:\n')
    logging.info('  Chip Version:  %d.%d.%d.%d.', vi.version.major,
                 vi.version.minor, vi.version.revMajor, vi.version.revMinor)
    logging.info('  Spec Level:  %d', vi.specLevel)
    logging.info('  Errata Revision:  %d', vi.errataRev)
    vendorId = [i for i in vi.tpmVendorID if i]
    logging.info('  TPM Vendor ID:  %s', ''.join(vendorId))
    # TODO(tgao): handle the case when there's no vendor specific data.
    logging.info('  Vendor Specific data (first 4 bytes in Hex):  '
                 '%.2x %.2x %.2x %.2x', vi.vendorSpecific[0],
                 vi.vendorSpecific[1], vi.vendorSpecific[2],
                 vi.vendorSpecific[3])


def PrintSelfTestResult(str_len, pResult):
    """Utility method to print TPM self test result.

    Args:
      str_len: an integer, length of string pointed to by pResult.
      pResult: a c_char_p, pointer to result.
    """
    out = []
    for i in range(str_len):
        if i and not i % 32:
            out.append('\t')
        if not i % 4:
            out.append(' ')
        b = pResult.value[i]
        out.append('%02x' % ord(b))
    logging.info('  TPM Test Results: %s', ''.join(out))


class TpmController(object):
    """Object to interact with TPM module for testing."""

    def __init__(self):
        """Constructor.

        Mandatory params:
          hContext: a c_uint32, context object handle.
          _contextSet: a boolean, True if TPM context is set.
          hTpm: a c_uint32, TPM object handle.
          hTpmPolicy: a c_uint32, TPM policy object handle.
          tspi_lib: a shared library object (libtspi.so).

        Raises:
          SmogcheckError: if error initializing TpmController.
        """
        self.hContext = c_uint32(0)
        self._contextSet = False
        self.hTpm = c_uint32(0)
        self.hTpmPolicy = c_uint32(0)

        logging.info('Attempt to load shared library %s', TSPI_C_LIB)
        try:
            self.tspi_lib = cdll.LoadLibrary(TSPI_C_LIB)
        except OSError, e:
            raise SmogcheckError('Error loading C library %s: %r' %
                                 (TSPI_C_LIB, e))
        logging.info('Successfully loaded shared library %s', TSPI_C_LIB)

    def closeContext(self):
        """Closes TPM context and cleans up.

        Returns:
          an integer, 0 for success and -1 for error.
        """
        if not self._contextSet:
            logging.debug('TPM context NOT set.')
            return 0

        ret = -1
        # Calling the pointer type without an argument creates a NULL pointer
        if self.tspi_lib.Tspi_Context_FreeMemory(self.hContext,
                                                 POINTER(c_byte)()) != 0:
            logging.error('Error freeing memory when closing TPM context')
        else:
            logging.debug('Tspi_Context_FreeMemory() success')

        if self.tspi_lib.Tspi_Context_Close(self.hContext) != 0:
            logging.error('Error closing TPM context')
        else:
            logging.debug('Tspi_Context_Close() success')
            ret = 0
            self._contextSet = False

        return ret

    def _closeContextObject(self, hObject):
        """Closes TPM context object.

        Args:
          hObject: an integer, basic object handle.

        Raises:
          SmogcheckError: if an error is encountered.
        """
        if self.tspi_lib.Tspi_Context_CloseObject(self.hContext, hObject) != 0:
            raise SmogcheckError('Error closing TPM context object')

        logging.debug('Tspi_Context_CloseObject() success')

    def setupContext(self):
        """Sets up tspi context for TPM access.

        TPM context cannot be reused. Therefore, each new Tspi_* command would
        require a new context to be set up before execution and closing that
        context after execution (or error).

        Raises:
          SmogcheckError: if an error is encountered.
        """
        if self._contextSet:
            logging.debug('TPM context already set.')
            return

        if self.tspi_lib.Tspi_Context_Create(byref(self.hContext)) != 0:
            raise SmogcheckError('Error creating tspi context')

        logging.info('Created tspi context = 0x%x', self.hContext.value)

        if self.tspi_lib.Tspi_Context_Connect(self.hContext,
                                              POINTER(c_uint16)()) != 0:
            raise SmogcheckError('Error connecting to tspi context')

        logging.info('Connected to tspi context')

        if self.tspi_lib.Tspi_Context_GetTpmObject(self.hContext,
                                                   byref(self.hTpm)) != 0:
            raise SmogcheckError('Error getting TPM object from tspi context')

        logging.info('Got tpm object from tspi context = 0x%x', self.hTpm.value)
        self._contextSet = True

    def _getTpmStatus(self, flag, bValue):
        """Wrapper function to call Tspi_TPM_GetStatus().

        Args:
          flag: a c_uint, TPM status info flag, values defined in C header file
                "tss/tss_defines.h".
          bValue: a c_bool, place holder for specific TPM flag bit value (0/1).

        Raises:
          SmogcheckError: if an error is encountered.
        """
        result = self.tspi_lib.Tspi_TPM_GetStatus(self.hTpm, flag,
                                                  byref(bValue))
        if result != 0:
            msg = ('Error (0x%x) getting status for flag 0x%x' %
                   (result, flag.value))
            raise SmogcheckError(msg)

        logging.info('Tspi_TPM_GetStatus(): success for flag 0x%x',
                     flag.value)

    def _setTpmStatus(self, flag, bValue):
        """Wrapper function to call Tspi_TPM_GetStatus().

        Args:
          flag: a c_uint, TPM status info flag.
          bValue: a c_bool, place holder for specific TPM flag bit value (0/1).

        Raises:
          SmogcheckError: if an error is encountered.
        """
        result = self.tspi_lib.Tspi_TPM_SetStatus(self.hTpm, flag, bValue)
        if result != 0:
            msg = ('Error (0x%x) setting status for flag 0x%x' %
                   (result, flag.value))
            raise SmogcheckError(msg)

        logging.info('Tspi_TPM_SetStatus(): success for flag 0x%x',
                     flag.value)

    def getPolicyObject(self, hTpm=None, hPolicy=None):
        """Get TPM policy object.

        Args:
          hTpm: a c_uint, TPM object handle.
          hPolicy: a c_uint, TPM policy object handle.

        Raises:
          SmogcheckError: if an error is encountered.
        """
        if hTpm is None:
            hTpm = self.hTpm

        if hPolicy is None:
            hPolicy = self.hTpmPolicy

        logging.debug('Tspi_GetPolicyObject(): hTpm = 0x%x, hPolicy = 0x%x',
                      hTpm.value, hPolicy.value)
        result = self.tspi_lib.Tspi_GetPolicyObject(hTpm, TSS_POLICY_USAGE,
                                                    byref(hPolicy))
        if result != 0:
            msg = 'Error (0x%x) getting TPM policy object' % result
            raise SmogcheckError(msg)

        logging.debug('Tspi_GetPolicyObject() success hTpm = 0x%x, '
                      'hPolicy = 0x%x', hTpm.value, hPolicy.value)

    def setPolicySecret(self, hPolicy=None, pSecret=None, secret_mode=None):
        """Sets TPM policy secret.

        Args:
          hPolicy: a c_uint, TPM policy object handle.
          pSecret: a pointer to a byte array, which holds the TSS secret.
          secret_mode: a string, valid values are keys of TSS_SECRET_MODE.

        Raises:
          SmogcheckError: if an error is encountered.
        """
        if hPolicy is None:
            hPolicy = self.hTpmPolicy

        if pSecret is None:
            raise SmogcheckError('setPolicySecret(): pSecret cannot be None')

        if secret_mode is None or secret_mode not in TSS_SECRET_MODE:
            raise SmogcheckError('setPolicySecret(): invalid secret_mode')

        logging.debug('Tspi_Policy_SetSecret(): hPolicy = 0x%x, secret_mode '
                      '(%r) = %r', hPolicy.value, secret_mode,
                      TSS_SECRET_MODE[secret_mode])

        result = self.tspi_lib.Tspi_Policy_SetSecret(
            hPolicy, TSS_SECRET_MODE[secret_mode], TPM_SHA1_160_HASH_LEN,
            pSecret)
        if result != 0:
            msg = 'Error (0x%x) setting TPM policy secret' % result
            raise SmogcheckError(msg)

        logging.debug('Tspi_Policy_SetSecret() success, hPolicy = 0x%x',
                      hPolicy.value)

    def getTpmVersion(self):
        """Gets TPM version info.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_version.c
        Downloaded from:
          http://sourceforge.net/projects/trousers/files/tpm-tools/1.3.4/\
          tpm-tools-1.3.4.tar.gz

        Raises:
          SmogcheckError: if an error is encountered.
        """
        uiResultLen = c_uint32(0)
        pResult = c_char_p()
        offset = c_uint64(0)
        versionInfo = TpmCapVersionInfo()
        InitVersionInfo(versionInfo)

        logging.debug('Successfully set up tspi context: hTpm = %r', self.hTpm)

        result = self.tspi_lib.Tspi_TPM_GetCapability(
            self.hTpm, TSS_TPMCAP_VERSION_VAL, 0, POINTER(c_byte)(),
            byref(uiResultLen), byref(pResult))
        if result != 0:
            msg = 'Error (0x%x) getting TPM capability, pResult = %r' % (
                result, pResult.value)
            raise SmogcheckError(msg)

        logging.info('Successfully received TPM capability: '
                     'uiResultLen = %d, pResult=%r', uiResultLen.value,
                     pResult.value)
        result = self.tspi_lib.Trspi_UnloadBlob_CAP_VERSION_INFO(
            byref(offset), pResult, cast(byref(versionInfo),
                                         POINTER(c_byte)))
        if result != 0:
            msg = 'Error (0x%x) unloading TPM CAP version info' % result
            raise SmogcheckError(msg)

        PrintVersionInfo(versionInfo)

    def runTpmSelfTest(self):
        """Executes TPM self test.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_selftest.c

        Raises:
          SmogcheckError: if an error is encountered.
        """
        uiResultLen = c_uint32(0)
        pResult = c_char_p()
        self.setupContext()

        logging.debug('Successfully set up tspi context: hTpm = 0x%x',
                      self.hTpm.value)

        result = self.tspi_lib.Tspi_TPM_SelfTestFull(self.hTpm)
        if result != 0:
            self.closeContext()
            raise SmogcheckError('Error (0x%x) with TPM self test' % result)

        logging.info('Successfully executed TPM self test: hTpm = 0x%x',
                     self.hTpm.value)
        result = self.tspi_lib.Tspi_TPM_GetTestResult(
            self.hTpm, byref(uiResultLen), byref(pResult))
        if result != 0:
            self.closeContext()
            raise SmogcheckError('Error (0x%x) getting test results' % result)

        logging.info('TPM self test results: uiResultLen = %d, pResult=%r',
                     uiResultLen.value, pResult.value)
        PrintSelfTestResult(uiResultLen.value, pResult)
        self.closeContext()

    def takeTpmOwnership(self):
        """Take TPM ownership.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_takeownership.c

        Raises:
          SmogcheckError: if an error is encountered.
        """
        hSrk = c_uint32(0)  # TPM Storage Root Key
        hSrkPolicy = c_uint32(0)
        # Defaults each byte value to 0x00
        well_known_secret = create_string_buffer(20)
        pSecret = c_char_p(addressof(well_known_secret))

        self.setupContext()
        logging.debug('Successfully set up tspi context: hTpm = 0x%x',
                      self.hTpm.value)

        try:
            self.getPolicyObject()
            self.setPolicySecret(pSecret=pSecret, secret_mode='sha1')
        except SmogcheckError:
            if hSrk != 0:
                self._closeContextObject(hSrk)
            self.closeContext()
            raise  # re-raise

        flag = TSS_KEY_TSP_SRK.value | TSS_KEY_AUTHORIZATION.value
        result = self.tspi_lib.Tspi_Context_CreateObject(
            self.hContext, TSS_OBJECT_TYPE_RSAKEY, flag, byref(hSrk))
        if result != 0:
            raise SmogcheckError('Error (0x%x) creating context object' %
                                 result)
        logging.debug('hTpm = 0x%x, flag = 0x%x, hSrk = 0x%x',
                      self.hTpm.value, flag, hSrk.value)  # DEBUG

        try:
            self.getPolicyObject(hTpm=hSrk, hPolicy=hSrkPolicy)
            self.setPolicySecret(hPolicy=hSrkPolicy, pSecret=pSecret,
                                 secret_mode='sha1')
        except SmogcheckError:
            if hSrk != 0:
                self._closeContextObject(hSrk)
            self.closeContext()
            raise  # re-raise

        logging.debug('Successfully set up SRK policy: secret = %r, '
                      'hSrk = 0x%x, hSrkPolicy = 0x%x',
                      well_known_secret.value, hSrk.value, hSrkPolicy.value)

        start_time = datetime.datetime.now()
        result = self.tspi_lib.Tspi_TPM_TakeOwnership(self.hTpm, hSrk,
                                                      c_uint(0))
        end_time = datetime.datetime.now()
        if result != 0:
            logging.info('Tspi_TPM_TakeOwnership error')
            self._closeContextObject(hSrk)
            self.closeContext()
            raise SmogcheckError('Error (0x%x) taking TPM ownership' % result)

        logging.info('Successfully took TPM ownership')
        self._closeContextObject(hSrk)
        self.closeContext()
        return smogcheck_ttci.computeTimeElapsed(end_time, start_time)

    def clearTpm(self):
        """Return TPM to default state.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_clear.c

        Raises:
          SmogcheckError: if an error is encountered.
        """
        logging.debug('Successfully set up tspi context: hTpm = %r', self.hTpm)

        result = self.tspi_lib.Tspi_TPM_ClearOwner(self.hTpm, True)
        if result != 0:
            raise SmogcheckError('Error (0x%x) clearing TPM' % result)

        logging.info('Successfully cleared TPM')

    def setTpmActive(self, op):
        """Change TPM active state.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_activate.c

        Args:
          op: a string, desired operation. Valid values are defined in
              TPM_SETACTIVE_OP.

        Raises:
          SmogcheckError: if an error is encountered.
        """
        bValue = c_bool()
        # Defaults each byte value to 0x00
        well_known_secret = create_string_buffer(20)
        pSecret = c_char_p(addressof(well_known_secret))

        if op not in TPM_SETACTIVE_OP:
            msg = ('Invalid op (%s) for tpmSetActive(). Valid values are %r' %
                   (op, TPM_SETACTIVE_OP))
            raise SmogcheckError(msg)

        logging.debug('Successfully set up tspi context: hTpm = %r', self.hTpm)

        if op == 'status':
            self.getPolicyObject()
            self.setPolicySecret(pSecret=pSecret, secret_mode='sha1')

            self._getTpmStatus(
                TSS_TPMSTATUS_PHYSICALSETDEACTIVATED, bValue)
            logging.info('Persistent Deactivated Status: %s', bValue.value)

            self._getTpmStatus(
                TSS_TPMSTATUS_SETTEMPDEACTIVATED, bValue)
            logging.info('Volatile Deactivated Status: %s', bValue.value)
        elif op == 'activate':
            self._setTpmStatus(
                TSS_TPMSTATUS_PHYSICALSETDEACTIVATED, False)
            logging.info('Successfully activated TPM')
        elif op == 'deactivate':
            self._setTpmStatus(
                TSS_TPMSTATUS_PHYSICALSETDEACTIVATED, True)
            logging.info('Successfully deactivated TPM')
        elif op == 'temp':
            self._setTpmStatus(
                TSS_TPMSTATUS_SETTEMPDEACTIVATED, True)
            logging.info('Successfully deactivated TPM for current boot')

    def setTpmClearable(self, op):
        """Disable TPM clear operations.

        Implementation based on tpm-tools-1.3.4/src/tpm_mgmt/tpm_clearable.c

        Args:
          op: a string, desired operation. Valid values are defined in
              TPM_SETCLEARABLE_OP.

        Raises:
          SmogcheckError: if an error is encountered.
        """
        bValue = c_bool()
        # Defaults each byte value to 0x00
        well_known_secret = create_string_buffer(20)
        pSecret = c_char_p(addressof(well_known_secret))

        if op not in TPM_SETCLEARABLE_OP:
            msg = ('Invalid op (%s) for tpmSetClearable(). Valid values are %r'
                   % (op, TPM_SETCLEARABLE_OP))
            raise SmogcheckError(msg)

        logging.debug('Successfully set up tspi context: hTpm = %r', self.hTpm)

        if op == 'status':
            self.getPolicyObject()
            self.setPolicySecret(pSecret=pSecret, secret_mode='sha1')

            self._getTpmStatus(
                TSS_TPMSTATUS_DISABLEOWNERCLEAR, bValue)
            logging.info('Owner Clear Disabled: %s', bValue.value)

            self._getTpmStatus(
                TSS_TPMSTATUS_DISABLEFORCECLEAR, bValue)
            logging.info('Force Clear Disabled: %s', bValue.value)
        elif op == 'owner':
            self.getPolicyObject()
            self.setPolicySecret(pSecret=pSecret, secret_mode='sha1')

            self._setTpmStatus(
                TSS_TPMSTATUS_DISABLEOWNERCLEAR, False)
            logging.info('Successfully disabled Owner Clear')
        elif op == 'force':
            self._setTpmStatus(
                TSS_TPMSTATUS_DISABLEFORCECLEAR, True)
            logging.info('Successfully disabled Force Clear')
