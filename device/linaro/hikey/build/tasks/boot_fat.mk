ifneq ($(filter hikey%, $(TARGET_DEVICE)),)

$(PRODUCT_OUT)/boot_fat.uefi.img: $(PRODUCT_OUT)/kernel $(PRODUCT_OUT)/hi6220-hikey.dtb $(PRODUCT_OUT)/ramdisk.img
# $@ is referring to $(PRODUCT_OUT)/boot_fat.uefi.img
	dd if=/dev/zero of=$@ bs=512 count=98304
	mkfs.fat -n "boot" $@
	mcopy -i $@ $(PRODUCT_OUT)/kernel ::Image
	mcopy -i $@ $(PRODUCT_OUT)/hi6220-hikey.dtb ::hi6220-hikey.dtb
	mcopy -s -i $@ device/linaro/hikey/bootloader/* ::
	mcopy -i $@ $(PRODUCT_OUT)/ramdisk.img ::ramdisk.img

droidcore: $(PRODUCT_OUT)/boot_fat.uefi.img

endif
