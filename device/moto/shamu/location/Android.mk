LOCAL_PATH := $(call my-dir)

#only /etc/gps.conf to begin with
DIR_LIST := $(LOCAL_PATH)/loc_api/
include $(addsuffix Android.mk, $(DIR_LIST))
