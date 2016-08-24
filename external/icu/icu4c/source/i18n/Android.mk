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

include $(CLEAR_VARS)

src_files := \
	decContext.c \
	decNumber.c \
	ucln_in.cpp \
	ulocdata.c \
	utmscale.c

src_files += \
	affixpatternparser.cpp \
	alphaindex.cpp \
	anytrans.cpp \
	astro.cpp \
	basictz.cpp \
	bocsu.cpp \
	brktrans.cpp \
	buddhcal.cpp \
	calendar.cpp \
	casetrn.cpp \
	cecal.cpp \
	chnsecal.cpp \
	choicfmt.cpp \
	coleitr.cpp \
	collationbuilder.cpp \
	collationcompare.cpp \
	collation.cpp \
	collationdatabuilder.cpp \
	collationdata.cpp \
	collationdatareader.cpp \
	collationdatawriter.cpp \
	collationfastlatinbuilder.cpp \
	collationfastlatin.cpp \
	collationfcd.cpp \
	collationiterator.cpp \
	collationkeys.cpp \
	collationroot.cpp \
	collationrootelements.cpp \
	collationruleparser.cpp \
	collationsets.cpp \
	collationsettings.cpp \
	collationtailoring.cpp \
	collationweights.cpp \
	coll.cpp \
	compactdecimalformat.cpp \
	coptccal.cpp \
	cpdtrans.cpp \
	csdetect.cpp \
	csmatch.cpp \
	csr2022.cpp \
	csrecog.cpp \
	csrmbcs.cpp \
	csrsbcs.cpp \
	csrucode.cpp \
	csrutf8.cpp \
	curramt.cpp \
	currfmt.cpp \
	currpinf.cpp \
	currunit.cpp \
	dangical.cpp \
	datefmt.cpp \
	dcfmtsym.cpp \
	decfmtst.cpp \
	decimalformatpattern.cpp \
	decimfmt.cpp \
	decimfmtimpl.cpp \
	digitaffix.cpp \
	digitaffixesandpadding.cpp \
	digitformatter.cpp \
	digitgrouping.cpp \
	digitinterval.cpp \
	digitlst.cpp \
	dtfmtsym.cpp \
	dtitvfmt.cpp \
	dtitvinf.cpp \
	dtptngen.cpp \
	dtrule.cpp \
	esctrn.cpp \
	ethpccal.cpp \
	fmtable_cnv.cpp \
	fmtable.cpp \
	format.cpp \
	fphdlimp.cpp \
	fpositer.cpp\
	funcrepl.cpp \
	gender.cpp \
	gregocal.cpp \
	gregoimp.cpp \
	hebrwcal.cpp \
	identifier_info.cpp \
	indiancal.cpp \
	inputext.cpp \
	islamcal.cpp \
	japancal.cpp \
	locdspnm.cpp \
	measfmt.cpp \
	measunit.cpp \
	measure.cpp \
	msgfmt.cpp \
	name2uni.cpp \
	nfrs.cpp \
	nfrule.cpp \
	nfsubs.cpp \
	nortrans.cpp \
	nultrans.cpp \
	numfmt.cpp \
	numsys.cpp \
	olsontz.cpp \
	persncal.cpp \
	pluralaffix.cpp \
	plurfmt.cpp \
	plurrule.cpp \
	precision.cpp \
	quant.cpp \
	quantityformatter.cpp \
	rbnf.cpp \
	rbt.cpp \
	rbt_data.cpp \
	rbt_pars.cpp \
	rbt_rule.cpp \
	rbt_set.cpp \
	rbtz.cpp \
	regexcmp.cpp \
	regeximp.cpp \
	regexst.cpp \
	regextxt.cpp \
	region.cpp \
	reldatefmt.cpp \
	reldtfmt.cpp \
	rematch.cpp \
	remtrans.cpp \
	repattrn.cpp \
	rulebasedcollator.cpp \
	scientificnumberformatter.cpp \
	scriptset.cpp \
	search.cpp \
	selfmt.cpp \
	sharedbreakiterator.cpp \
	simpletz.cpp \
	smallintformatter.cpp \
	smpdtfmt.cpp \
	smpdtfst.cpp \
	sortkey.cpp \
	standardplural.cpp \
	strmatch.cpp \
	strrepl.cpp \
	stsearch.cpp \
	taiwncal.cpp \
	timezone.cpp \
	titletrn.cpp \
	tmunit.cpp \
	tmutamt.cpp \
	tmutfmt.cpp \
	tolowtrn.cpp \
	toupptrn.cpp \
	translit.cpp \
	transreg.cpp \
	tridpars.cpp \
	tzfmt.cpp \
	tzgnames.cpp \
	tznames.cpp \
	tznames_impl.cpp \
	tzrule.cpp \
	tztrans.cpp \
	ucal.cpp \
	ucol.cpp \
	ucoleitr.cpp \
	ucol_res.cpp \
	ucol_sit.cpp \
	ucsdet.cpp \
	ucurr.cpp \
	udat.cpp \
	udateintervalformat.cpp \
	udatpg.cpp \
	ufieldpositer.cpp \
	uitercollationiterator.cpp \
	umsg.cpp \
	unesctrn.cpp \
	uni2name.cpp \
	unum.cpp \
	upluralrules.cpp \
	uregexc.cpp \
	uregex.cpp \
	usearch.cpp \
	uspoof_build.cpp \
	uspoof_conf.cpp \
	uspoof.cpp \
	uspoof_impl.cpp \
	uspoof_wsconf.cpp \
	utf16collationiterator.cpp \
	utf8collationiterator.cpp \
	utrans.cpp \
	valueformatter.cpp \
	windtfmt.cpp \
	winnmfmt.cpp \
	wintzimpl.cpp \
	visibledigits.cpp \
	vtzone.cpp \
	vzone.cpp \
	zonemeta.cpp \
	zrule.cpp \
	ztrans.cpp

c_includes = \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../common

local_cflags := -D_REENTRANT
local_cflags += -DU_I18N_IMPLEMENTATION
local_cflags += -O3 -fvisibility=hidden


#
# Build for the target (device).
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES += $(src_files)
LOCAL_C_INCLUDES += $(c_includes) $(optional_android_logging_includes)
LOCAL_CFLAGS += $(local_cflags) -DPIC -fPIC
LOCAL_RTTI_FLAG := -frtti
LOCAL_SHARED_LIBRARIES += libicuuc $(optional_android_logging_libraries)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicui18n
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_CLANG := true
include $(BUILD_SHARED_LIBRARY)


#
# Build for the host.
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES += $(src_files)
LOCAL_C_INCLUDES += $(c_includes) $(optional_android_logging_includes)
LOCAL_CFLAGS += $(local_cflags)
LOCAL_SHARED_LIBRARIES += libicuuc-host $(optional_android_logging_libraries)
LOCAL_LDLIBS += -lm -lpthread
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicui18n-host
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
LOCAL_SRC_FILES += $(src_files)
LOCAL_C_INCLUDES += $(c_includes) $(optional_android_logging_includes)
LOCAL_SHARED_LIBRARIES += $(optional_android_logging_libraries)
LOCAL_STATIC_LIBRARIES += libicuuc_static
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)
LOCAL_RTTI_FLAG := -frtti
LOCAL_CFLAGS += $(local_cflags) -DPIC -fPIC
# Using -Os over -O3 actually cuts down the final executable size by a few dozen kilobytes
LOCAL_CFLAGS += -Os
LOCAL_EXPORT_CFLAGS += -DU_STATIC_IMPLEMENTATION=1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libicui18n_static
LOCAL_CLANG := true
include $(BUILD_STATIC_LIBRARY)
