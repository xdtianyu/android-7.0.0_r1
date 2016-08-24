/*
 * Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>

#if !defined(__i386__) && !defined(__x86_64__)
int main(void)
{
    fprintf(stderr, "Unsupported architecture\n");
    return EXIT_FAILURE;
}
#else

#include <pci/pci.h>
#include <sys/io.h>

#define D_LCK   (1 << 4)
#define D_OPEN  (1 << 6)

int check_smram(struct pci_dev *northbridge, int offset)
{
    unsigned char smram_value;
    int code = EXIT_SUCCESS;

    smram_value = pci_read_byte(northbridge, offset);

    if (smram_value & D_OPEN) {
        fprintf(stderr, "FAIL: D_OPEN is set\n");
        code = EXIT_FAILURE;
    } else {
        printf("ok: D_OPEN is unset\n");
    }

    if (smram_value & D_LCK) {
        printf("ok: D_LCK is set\n");
    } else {
        fprintf(stderr, "FAIL: D_LCK is unset\n");
        code = EXIT_FAILURE;
    }

    return code;
}

int guess_offset(struct pci_dev *northbridge)
{
    unsigned int id;
    int offset = 0;

    id = pci_read_word(northbridge, 2);
    switch (id) {
    case 0xa010:
        printf("Detected Pineview Mobile\n");
        offset = 0x9d;
        break;
    case 0x0100:
        printf("Detected Sandybridge Desktop\n");
        offset = 0x88;
        break;
    case 0x0104:
        printf("Detected Sandybridge Mobile\n");
        offset = 0x88;
        break;
    case 0x0154:
        printf("Detected Ivybridge Mobile\n");
        offset = 0x88;
        break;
    case 0x0c04:
        printf("Detected Haswell Mobile\n");
        offset = 0x88;
        break;
    case 0x0a04:
        printf("Detected Haswell ULT\n");
        offset = 0x88;
        break;
    case 0x0f00:
        printf("Detected Baytrail, skipping test\n");
        exit(EXIT_SUCCESS);
    case 0x1604:
        printf("Detected Broadwell ULT\n");
        offset = 0x88;
        break;
    default:
        fprintf(stderr, "FAIL: unknown Northbridge 0x%04x\n", id);
        exit(1);
    }

    return offset;
}

int main(int argc, char *argv[])
{
    int offset;
    struct pci_access *handle;
    struct pci_dev *device;

    handle = pci_alloc();
    if (!handle) {
        fprintf(stderr, "Failed to allocate PCI resource.\n");
        return EXIT_FAILURE;
    }
    pci_init(handle);

    device = pci_get_dev(handle, 0, 0, 0, 0);
    if (!device) {
        fprintf(stderr, "Failed to fetch PCI device.\n");
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        offset = strtoul(argv[1], NULL, 0);
    } else {
        offset = guess_offset(device);
    }
    printf("Using SMRAM offset 0x%02x:\n", offset);

    return check_smram(device, offset);
}

#endif /* x86 */
