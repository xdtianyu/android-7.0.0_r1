
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := media_resource_manager
LOCAL_COPY_HEADERS := \
    arbitrator/MediaResourceArbitrator.h \
    omx_adaptor/OMX_adaptor.h

include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
MEDIA_RESOURCE_MANAGER_ROOT := $(LOCAL_PATH)

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
