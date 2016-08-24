This directory contains kernel headers that must be modified by hand
before they are used to generate Bionic's "cleaned-up" user-land headers.

Currently this includes scsi/scsi.h from 3.18, which mixes userspace-facing
and kernel-facing definitions even after the uapi cleanup.

They are mostly covered by the GPLv2 + exception, and thus cannot be
distributed as part of the platform itself.
(NOTE: The cleaned up headers do not contain copyrightable information and are
distributed with Bionic)

Importing modified kernel headers files is done using scripts found in bionic.

For more details, please read bionic/libc/kernel/README.TXT.
