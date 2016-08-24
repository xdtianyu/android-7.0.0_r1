LOCAL_PATH:=$(call my-dir)

# Only build our tests if we doing a top-level build. Do not build the
# tests if we are just doing an mm or mmm in frameworks/rs.
ifeq (,$(ONE_SHOT_MAKEFILE))
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
