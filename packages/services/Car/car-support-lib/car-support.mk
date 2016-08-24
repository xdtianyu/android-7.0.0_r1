#
# Include this make file to build your application with car friendly UI.
#
# Make sure to include it after you've set all your desired LOCAL variables.
# Note that you must explicitly set your LOCAL_RESOURCE_DIR before including this file.
#
# For example:
#
#   LOCAL_RESOURCE_DIR := \
#        $(LOCAL_PATH)/res
#
#   include packages/services/Car/car-support-lib/car-support.mk
#

# Check that LOCAL_RESOURCE_DIR is defined
ifeq (,$(LOCAL_RESOURCE_DIR))
$(error LOCAL_RESOURCE_DIR must be defined)
endif

# Add --auto-add-overlay flag if not present
ifeq (,$(findstring --auto-add-overlay, $(LOCAL_AAPT_FLAGS)))
LOCAL_AAPT_FLAGS += --auto-add-overlay
endif

# Include car ui library, if not already included
ifeq (,$(findstring android.support.car, $(LOCAL_STATIC_JAVA_LIBRARIES)))
LOCAL_RESOURCE_DIR += \
    packages/services/Car/car-support-lib/res
LOCAL_AAPT_FLAGS += --extra-packages android.support.car.ui
LOCAL_STATIC_JAVA_LIBRARIES += android.support.car
endif

## Include transitive dependencies below

# Include support-v7-appcompat, if not already included
ifeq (,$(findstring android-support-v7-appcompat,$(LOCAL_STATIC_JAVA_LIBRARIES)))
LOCAL_RESOURCE_DIR += frameworks/support/v7/appcompat/res
LOCAL_AAPT_FLAGS += --extra-packages android.support.v7.appcompat
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v7-appcompat
endif

# Include support-v7-recyclerview, if not already included
ifeq (,$(findstring android-support-v7-recyclerview,$(LOCAL_STATIC_JAVA_LIBRARIES)))
LOCAL_RESOURCE_DIR += frameworks/support/v7/recyclerview/res
LOCAL_AAPT_FLAGS += --extra-packages android.support.v7.recyclerview
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v7-recyclerview
endif

# Include support-v7-cardview, if not already included
ifeq (,$(findstring android-support-v7-cardview,$(LOCAL_STATIC_JAVA_LIBRARIES)))
LOCAL_RESOURCE_DIR += frameworks/support/v7/cardview/res
LOCAL_AAPT_FLAGS += --extra-packages android.support.v7.cardview
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v7-cardview
endif
LOCAL_JAVA_LIBRARIES += android.car

