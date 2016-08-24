OPENCV := $(call my-dir)

LOCAL_C_INCLUDES := \
	$(OPENCV)/cv/include  \
	$(OPENCV)/cxcore/include  \
	$(OPENCV)/cvaux/include  \
	$(OPENCV)/ml/include  \
	$(OPENCV)/otherlibs/highgui  \
	$(LOCAL_C_INCLUDES)
