// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef         _VENDOR_STRING_H
#define         _VENDOR_STRING_H
//
//     Define up to 4-byte values for MANUFACTURER. This value defines the
//     response for TPM_PT_MANUFACTURER in TPM2_GetCapability(). The
//     definition should be replaced as appropriate when this code is used for
//     actual implementations.
//
#define        MANUFACTURER       "CROS"
//
//     The following #if macro may be deleted after a proper MANUFACTURER is provided.
//
#ifndef MANUFACTURER
#error MANUFACTURER is not provided. \
Please modify VendorString.h to provide a specific \
manufacturer name.
#endif
//
//     Define up to 4, 4-byte values. The values must each be 4 bytes long and the last value used may contain
//     trailing zeros. These values define the response for TPM_PT_VENDOR_STRING_(1-4) in
//     TPM2_GetCapability(). The following line should be un-commented and a vendor specific string should
//     be provided here. The vendor strings 2-4 may also be defined as appropriately.
//
#define           VENDOR_STRING_1             "xCG "
#define           VENDOR_STRING_2             "fTPM"
// #define           VENDOR_STRING_3
// #define           VENDOR_STRING_4
//
//     The following #if macro may be deleted after a proper VENDOR_STRING_1 is provided.
//
#ifndef VENDOR_STRING_1
#error VENDOR_STRING_1 is not provided. \
Please modify include\VendorString.h to provide a vednor specific \
string.
//
#endif
//
//     the more significant 32-bits of a vendor-specific value indicating the
//     version of the firmware. Some instrumentation could be added to replace
//     the following definition(s) with some release tag, SHA1, build date,
//     etc.
//
#define     FIRMWARE_V1               (0)
//
//     the optional less significant 32-bits of a vendor-specific value
//     indicating the version of the firmware.
//
#define     FIRMWARE_V2               (1)
//
//     The following #if macro may be deleted after a proper FIRMWARE_V1 is provided.
//
#ifndef FIRMWARE_V1
#error FIRMWARE_V1 is not provided. \
Please modify include\VendorString.h to provide a vendor specific firmware \
version
#endif
#endif
