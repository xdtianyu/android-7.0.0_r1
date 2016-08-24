# Rule to make a distribution zipfile with all that is needed to flash the Edison

ifeq ($(TARGET_DEVICE),edison)

edisonname := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  edisonname := $(edisonname)_debug
endif
edisonname := $(edisonname)-flashfiles-$(FILE_NAME_TAG)

EDISON_ZIP    :=  $(TARGET_OUT_INTERMEDIATES)/$(edisonname).zip
EDISON_VENDOR := vendor/bsp/intel/edison
EDISON_DEVICE := device/intel/edison
EDISON_IFWI   := $(EDISON_VENDOR)/ifwi_firmware
EDISON_UBOOT  := $(EDISON_VENDOR)/uboot_firmware
EDISON_TOOLS  := $(EDISON_DEVICE)/flash_tools

EDISON_FLASHFILES := $(INSTALLED_BOOTIMAGE_TARGET)
EDISON_FLASHFILES += $(INSTALLED_SYSTEMIMAGE)
EDISON_FLASHFILES += $(INSTALLED_USERDATAIMAGE_TARGET)
EDISON_FLASHFILES += $(PRODUCT_OUT)/gpt.bin
EDISON_FLASHFILES += $(EDISON_IFWI)/edison_ifwi-dbg-00.bin \
                     $(EDISON_IFWI)/edison_dnx_fwr.bin \
                     $(EDISON_IFWI)/edison_dnx_osr.bin
EDISON_FLASHFILES += $(EDISON_UBOOT)/u-boot-edison.bin $(EDISON_UBOOT)/u-boot-edison.img
EDISON_FLASHFILES += $(EDISON_TOOLS)/FlashEdison.json \
                     $(EDISON_TOOLS)/brillo-flashall-edison.bat \
                     $(EDISON_TOOLS)/brillo-flashall-edison.sh \
                     $(EDISON_TOOLS)/README


$(EDISON_ZIP): $(EDISON_FLASHFILES)
	$(hide) echo "Package flashfiles: $@"
	$(hide) rm -rf $@
	$(hide) mkdir -p $(dir $@)
	$(hide) zip -j $@ $(EDISON_FLASHFILES)

$(call dist-for-goals, dist_files, $(EDISON_ZIP))

endif
