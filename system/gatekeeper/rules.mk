LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/gatekeeper_messages.cpp \
	$(LOCAL_DIR)/gatekeeper.cpp

GLOBAL_INCLUDES += $(LOCAL_DIR)/include/

MODULE_DEPS := \
	lib/libc \
	lib/libc-trusty \

include make/module.mk
