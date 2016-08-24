// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * The purpose of this test is to exercise the GPU failure path.
 * We craft an erroneous GPU command packet and send it to the GPU,
 * and wait for a udev event notifying us of a GPU hang.
 * If the event doesn't come back, the test fails.
 *
 * This test must run with ui stopped.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void OUTPUT_INFO(char *msg) {
	printf("INFO: %s\n", msg);
	fflush(0);
}
void OUTPUT_WARNING(char *msg) {
	printf("WARNING: %s\n", msg);
	fflush(0);
}
void OUTPUT_ERROR(char *msg) {
	printf("ERROR: %s\n", msg);
	fflush(0);
}
void OUTPUT_RUN() {
	printf("[ RUN      ] graphics_GpuReset\n");
	fflush(0);
}
void EXIT(int code) {
	// Sleep a bit. This is not strictly required but will avoid the case where
	// we call the test back to back and the kernel thinks the GPU is toast.
	OUTPUT_INFO("sleep(10) to prevent the kernel from thinking the GPU is completely locked.");
	sleep(10);
	exit(code);
}
void OUTPUT_PASS_AND_EXIT() {
	printf("[       OK ] graphics_GpuReset\n");
	fflush(0);
	EXIT(0);
}
void OUTPUT_FAIL_AND_EXIT(char *msg) {
	printf("[  FAILED  ] graphics_GpuReset %s\n", msg);
	fflush(0);
	EXIT(-1);
}

#if !defined(__INTEL_GPU__)

#pragma message "Compiling for GPU other than Intel."

int main(int argc, char **argv)
{
	OUTPUT_RUN();
	OUTPUT_WARNING("The gpureset test is defined for some Intel GPUs only.");
	OUTPUT_PASS_AND_EXIT();
	return 0;
}

#else

#pragma message "Compiling for Intel GPU."

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#define LIBUDEV_I_KNOW_THE_API_IS_SUBJECT_TO_CHANGE
#include <libudev.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "xf86drm.h"
#include "i915_drm.h"
#include "intel_bufmgr.h"

#define DRM_TEST_MASTER 0x01


static int is_master(int fd)
{
	drm_client_t client;
	int ret;

	/* Check that we're the only opener and authed. */
	client.idx = 0;
	ret = ioctl(fd, DRM_IOCTL_GET_CLIENT, &client);
	assert (ret == 0);
	if (!client.auth)
		return 0;
	client.idx = 1;
	ret = ioctl(fd, DRM_IOCTL_GET_CLIENT, &client);
	if (ret != -1 || errno != EINVAL)
		return 0;

	return 1;
}

/** Open the first DRM device matching the criteria. */
int drm_open_matching(const char *pci_glob, int flags)
{
	struct udev *udev;
	struct udev_enumerate *e;
	struct udev_device *device, *parent;
        struct udev_list_entry *entry;
	const char *pci_id, *path;
	const char *usub, *dnode;
	int fd;

	udev = udev_new();
	if (udev == NULL)
		return -1;

	fd = -1;
	e = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(e, "drm");
        udev_enumerate_scan_devices(e);
        udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(udev, path);
		parent = udev_device_get_parent(device);
		usub = udev_device_get_subsystem(parent);
		/* Filter out KMS output devices. */
		if (!usub || (strcmp(usub, "pci") != 0))
			continue;
		pci_id = udev_device_get_property_value(parent, "PCI_ID");
		if (fnmatch(pci_glob, pci_id, 0) != 0)
			continue;
		dnode = udev_device_get_devnode(device);
		if (strstr(dnode, "control"))
			continue;
		fd = open(dnode, O_RDWR);
		if (fd < 0)
			continue;
		if ((flags & DRM_TEST_MASTER) && !is_master(fd)) {
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}
        udev_enumerate_unref(e);
	udev_unref(udev);

	return fd;
}

struct udev_monitor* udev_init()
{
	char* subsystem = "drm";
	struct udev* udev;
	// Create the udev object.
	udev = udev_new();
	if (!udev) {
		OUTPUT_ERROR("Can't get create udev object.");
		return NULL;
	}

	// Create the udev monitor structure.
	struct udev_monitor* monitor = udev_monitor_new_from_netlink(udev, "udev");
	if (!monitor) {
		OUTPUT_ERROR("Can't get create udev monitor");
		udev_unref(udev);
		return NULL;
	}

	udev_monitor_filter_add_match_subsystem_devtype(monitor,
			subsystem,
			NULL);
	udev_monitor_enable_receiving(monitor);

	return monitor;
}

int udev_wait(struct udev_monitor* monitor)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	int fd = udev_monitor_get_fd(monitor);

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	// Wait for at most 20 seconds for the event to come back.
	tv.tv_sec = 20;
	tv.tv_usec = 0;

	ret = select(fd+1, &fds, NULL, NULL, &tv);

	if (ret>0)
	{
		struct udev_device* dev = udev_monitor_receive_device(monitor);
		if (dev) {
		  // TODO(ihf): variable args to INFO function.
			printf("INFO: Event on (%s|%s|%s) Action %s\n",
					udev_device_get_devnode(dev),
					udev_device_get_subsystem(dev),
					udev_device_get_devtype(dev),
					udev_device_get_action(dev));
			udev_device_unref(dev);
			return 1;
		} else {
			OUTPUT_ERROR("Can't get receive_device().");
			return 0;
		}
	} else {
		OUTPUT_ERROR("Timed out waiting for udev event to come back.");
		return 0;
	}
}

int main(int argc, char **argv)
{
	int fd;
	int ret;
	drmVersionPtr v;

	OUTPUT_RUN();
	OUTPUT_INFO("The GPU reset test *must* be run with 'stop ui'.");
	OUTPUT_INFO("Otherwise following tests will likely hang/crash the machine.");
	OUTPUT_INFO("sleep(10) to make sure UI has time to stop.");
	sleep(10);

	fd = drm_open_matching("*:*", 0);

	if (fd < 0) {
		OUTPUT_FAIL_AND_EXIT("Failed to open any drm device.");
	}

	v = drmGetVersion(fd);
	assert(strlen(v->name) != 0);
	if (strcmp(v->name, "i915") == 0) {
		assert(v->version_major >= 1);
	} else {
		OUTPUT_WARNING("Can't find Intel GPU.");
		OUTPUT_PASS_AND_EXIT();
	}

	unsigned int pci_id;
	struct drm_i915_getparam gp;
	gp.param = I915_PARAM_CHIPSET_ID;
	gp.value = (int*)&pci_id;
	ret = ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp, sizeof(gp));

	if (ret) {
		OUTPUT_FAIL_AND_EXIT("Can't get the i915 pci_id.");
	}

	// TODO(ihf): variable args to INFO function.
	printf("INFO: i915 pci_id=0x%x.\n", pci_id);
	switch(pci_id) {
		// sandy bridge
		case 0x102:
                case 0x106: // Butterfly, Lumpy.
		case 0x116:
		case 0x126:
		// ivy bridge
                case 0x156: // Stout.
                case 0x166: // Link.
                // haswell
                case 0xa06: // GT1, Peppy, Falco.
                case 0xa16: // GT2.
                case 0xa26: // GT3.
			break;
		default:
		{
			OUTPUT_WARNING("Intel GPU detected, but model doesn't support reset.");
			OUTPUT_PASS_AND_EXIT();
		}
	}

	struct udev_monitor* monitor = udev_init();
	if (!monitor) {
		OUTPUT_FAIL_AND_EXIT("udev init failed.");
	}

	drm_intel_bufmgr* bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);

	drm_intel_bo* bo;
	bo = drm_intel_bo_alloc(bufmgr, "bogus cmdbuffer", 4096, 4096);

	uint32_t invalid_buf[8] =
	{
		0x00000000, // NOOP
		0xd00dd00d, // invalid command
		0x00000000, // NOOP
		0x00000000, // NOOP
		0x05000000, // BATCHBUFFER_END
		0x05000000, // BATCHBUFFER_END
		0x05000000, // BATCHBUFFER_END
		0x05000000, // BATCHBUFFER_END
	};

	// Copy our invalid cmd buffer into the bo.
	ret = drm_intel_bo_subdata(bo, 0, sizeof(invalid_buf), invalid_buf);
	if (ret != 0) {
		OUTPUT_FAIL_AND_EXIT("bo_subdata failed.");
	}

	// Submit our invalid buffer.
	ret = drm_intel_bo_exec(bo, sizeof(invalid_buf), NULL, 0, 0);
	if (ret != 0) {
		OUTPUT_FAIL_AND_EXIT("bo_exec failed.");
	}
	OUTPUT_INFO("Sent bogus buffer, waiting for event.");
	// Submit our invalid buffer.
	drm_intel_bo_wait_rendering(bo);

	int res = udev_wait(monitor);

	drmFree(v);
	close(fd);

	if (res) {
		OUTPUT_PASS_AND_EXIT();
	}
	else {
		OUTPUT_FAIL_AND_EXIT("GPU reset event did not come back.");
	}

	return 0;
}

#endif // defined(__arm__) ||  !defined(__INTEL_GPU__)
