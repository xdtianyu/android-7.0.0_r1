# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH:= $(call my-dir)

#
# Common definitions.
#

src_files := \
	cmemory.c \
	cstring.c \
	cstr.cpp \
	cwchar.c \
	filteredbrk.cpp \
	icuplug.cpp \
	loadednormalizer2impl.cpp \
	locmap.c \
	pluralmap.cpp \
	propsvec.c \
	punycode.cpp \
	putil.cpp \
	resource.cpp \
	sharedobject.cpp \
	simplepatternformatter.cpp \
	uarrsort.c \
	ubidi.c \
	ubidiln.c \
	ubidi_props.c \
	ubidiwrt.c \
	ucase.cpp \
	ucasemap.cpp \
	ucat.c \
	uchar.c \
	ucln_cmn.cpp \
	ucmndata.c \
	ucnv2022.cpp \
	ucnv_bld.cpp \
	ucnvbocu.cpp \
	ucnv.c \
	ucnv_cb.c \
	ucnv_cnv.c \
	ucnv_ct.c \
	ucnvdisp.c \
	ucnv_err.c \
	ucnv_ext.cpp \
	ucnvhz.c \
	ucnv_io.cpp \
	ucnvisci.c \
	ucnvlat1.c \
	ucnv_lmb.c \
	ucnvmbcs.cpp \
	ucnvscsu.c \
	ucnv_set.c \
	ucnv_u16.c \
	ucnv_u32.c \
	ucnv_u7.c \
	ucnv_u8.c \
	udatamem.c \
	udataswp.c \
	uenum.c \
	uhash.c \
	uinit.cpp \
	uinvchar.c \
	ulist.c \
	ulistformatter.cpp \
	uloc.cpp \
	uloc_keytype.cpp \
	uloc_tag.c \
	umapfile.c \
	umath.c \
	umutex.cpp \
	unames.cpp \
	uresbund.cpp \
	ures_cnv.c \
	uresdata.cpp \
	usc_impl.c \
	uscript.c \
	uscript_props.cpp \
	ushape.cpp \
	ustrcase.cpp \
	ustr_cnv.cpp \
	ustrfmt.c \
	ustring.cpp \
	ustrtrns.cpp \
	ustr_wcs.cpp \
	utf_impl.c \
	utrace.c \
	utrie2_builder.cpp \
	utrie.cpp \
	utypes.c \
	wintz.c

src_files += \
	appendable.cpp \
	bmpset.cpp \
	brkeng.cpp \
	brkiter.cpp \
	bytestream.cpp \
	bytestriebuilder.cpp \
	bytestrie.cpp \
	bytestrieiterator.cpp \
	caniter.cpp \
	chariter.cpp \
	charstr.cpp \
	dictbe.cpp \
	dictionarydata.cpp \
	dtintrv.cpp \
	errorcode.cpp \
	filterednormalizer2.cpp \
	listformatter.cpp \
	locavailable.cpp \
	locbased.cpp \
	locdispnames.cpp \
	locid.cpp \
	loclikely.cpp \
	locresdata.cpp \
	locutil.cpp \
	messagepattern.cpp \
	normalizer2.cpp \
	normalizer2impl.cpp \
	normlzr.cpp \
	parsepos.cpp \
	patternprops.cpp \
	propname.cpp \
	rbbi.cpp \
	rbbidata.cpp \
	rbbinode.cpp \
	rbbirb.cpp \
	rbbiscan.cpp \
	rbbisetb.cpp \
	rbbistbl.cpp \
	rbbitblb.cpp \
	resbund_cnv.cpp \
	resbund.cpp \
	ruleiter.cpp \
	schriter.cpp \
	serv.cpp \
	servlk.cpp \
	servlkf.cpp \
	servls.cpp \
	servnotf.cpp \
	servrbf.cpp \
	servslkf.cpp \
	stringpiece.cpp \
	stringtriebuilder.cpp \
	ubrk.cpp \
	ucasemap_titlecase_brkiter.cpp \
	ucharstriebuilder.cpp \
	ucharstrie.cpp \
	ucharstrieiterator.cpp \
	uchriter.cpp \
	ucnvsel.cpp \
	ucol_swp.cpp \
	udata.cpp \
	uhash_us.cpp \
	uidna.cpp \
	uiter.cpp \
	unifiedcache.cpp \
	unifilt.cpp \
	unifunct.cpp \
	uniset_closure.cpp \
	uniset.cpp \
	uniset_props.cpp \
	unisetspan.cpp \
	unistr_case.cpp \
	unistr_case_locale.cpp \
	unistr_cnv.cpp \
	unistr.cpp \
	unistr_props.cpp \
	unistr_titlecase_brkiter.cpp \
	unormcmp.cpp \
	unorm.cpp \
	uobject.cpp \
	uprops.cpp \
	uset.cpp \
	usetiter.cpp \
	uset_props.cpp \
	usprep.cpp \
	ustack.cpp \
	ustrcase_locale.cpp \
	ustrenum.cpp \
	ustr_titlecase_brkiter.cpp \
	utext.cpp \
	util.cpp \
	util_props.cpp \
	utrie2.cpp \
	uts46.cpp \
	uvector.cpp \
	uvectr32.cpp \
	uvectr64.cpp

# This is the empty compiled-in icu data structure
# that we need to satisfy the linker.
src_files += ../stubdata/stubdata.c

c_includes := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../i18n

# We deliberately do not set -DICU_DATA_DIR: ICU4C is configured on Android
# using udata_setCommonData.

local_cflags += -D_REENTRANT
local_cflags += -DU_COMMON_IMPLEMENTATION

local_cflags += -O3 -fvisibility=hidden

local_cflags += -Wno-unused-parameter \
                -Wno-missing-field-initializers \
                -Wno-sign-compare \
                -Wno-deprecated-declarations

#
# Build for the target (device).
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES += $(src_files)
LOCAL_C_INCLUDES += $(c_includes) $(optional_android_logging_includes)
LOCAL_CFLAGS += $(local_cflags) -DPIC -fPIC
LOCAL_SHARED_LIBRARIES += libdl $(optional_android_logging_libraries)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicuuc
LOCAL_RTTI_FLAG := -frtti
LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.mk
ifndef BRILLO
LOCAL_REQUIRED_MODULES += icu-data
endif
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
ifneq ($(TARGET_ARCH),mips64)
LOCAL_CLANG := true
endif
include $(BUILD_SHARED_LIBRARY)

#
# Build for the host.
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES += $(src_files)
LOCAL_C_INCLUDES += $(c_includes) $(optional_android_logging_includes)
LOCAL_CFLAGS += $(local_cflags)
LOCAL_SHARED_LIBRARIES += $(optional_android_logging_libraries)
LOCAL_LDLIBS += -ldl -lm -lpthread
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicuuc-host
LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.mk
LOCAL_REQUIRED_MODULES += icu-data-host
LOCAL_MULTILIB := both
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_CLANG := true
include $(BUILD_HOST_SHARED_LIBRARY)

#
# Build as a static library against the NDK
#

include $(CLEAR_VARS)
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := stlport_static
LOCAL_C_INCLUDES += $(c_includes)
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)
LOCAL_RTTI_FLAG := -frtti
LOCAL_CFLAGS += $(local_cflags) -DPIC -fPIC
# Using -Os over -O3 actually cuts down the final executable size by a few dozen kilobytes
LOCAL_CFLAGS += -Os
LOCAL_EXPORT_CFLAGS += -DU_STATIC_IMPLEMENTATION=1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicuuc_static
LOCAL_SRC_FILES += $(src_files)
ifndef BRILLO
LOCAL_REQUIRED_MODULES += icu-data
endif
ifneq ($(TARGET_ARCH),mips64)
LOCAL_CLANG := true
endif
include $(BUILD_STATIC_LIBRARY)
