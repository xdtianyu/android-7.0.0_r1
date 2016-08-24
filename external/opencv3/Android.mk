LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_core

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/opencv2 \
    $(LOCAL_PATH)/modules/hal/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_LDLIBS := -llog -lz -ldl

# cxmathfuncs.cpp has implicit cast of int struct fields.
LOCAL_CLANG_CFLAGS += -Wno-c++11-narrowing

LOCAL_SRC_FILES := \
    modules/core/src/algorithm.cpp \
    modules/core/src/copy.cpp \
    modules/core/src/lda.cpp \
    modules/core/src/opengl.cpp \
    modules/core/src/stat.cpp \
    modules/core/src/alloc.cpp \
    modules/core/src/downhill_simplex.cpp \
    modules/core/src/lpsolver.cpp \
    modules/core/src/out.cpp \
    modules/core/src/stl.cpp \
    modules/core/src/arithm.cpp \
    modules/core/src/cuda_gpu_mat.cpp \
    modules/core/src/dxt.cpp \
    modules/core/src/mathfuncs.cpp \
    modules/core/src/parallel.cpp \
    modules/core/src/system.cpp \
    modules/core/src/array.cpp \
    modules/core/src/cuda_host_mem.cpp \
    modules/core/src/matmul.cpp \
    modules/core/src/parallel_pthreads.cpp \
    modules/core/src/tables.cpp \
    modules/core/src/cuda_info.cpp \
    modules/core/src/matop.cpp \
    modules/core/src/pca.cpp \
    modules/core/src/types.cpp \
    modules/core/src/command_line_parser.cpp \
    modules/core/src/cuda_stream.cpp \
    modules/core/src/glob.cpp \
    modules/core/src/matrix.cpp \
    modules/core/src/persistence.cpp \
    modules/core/src/umatrix.cpp \
    modules/core/src/conjugate_gradient.cpp \
    modules/core/src/datastructs.cpp \
    modules/core/src/kmeans.cpp \
    modules/core/src/ocl.cpp \
    modules/core/src/convert.cpp \
    modules/core/src/directx.cpp \
    modules/core/src/lapack.cpp \
    modules/core/src/rand.cpp

LOCAL_STATIC_LIBRARIES += libopencv_hal

include $(BUILD_SHARED_LIBRARY)


# Build dls.cpp separately without optimizations to avoid slow compile times.
# We only need to pass -O1 for arm64. Everything else works fine with the defaults.
# Bug: http://b/25691376
include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_fix_dls

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/calib3d/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/calib3d/src \
    $(LOCAL_PATH)/modules/calib3d \
    $(LOCAL_PATH)/opencv2 \
    $(LOCAL_PATH)/modules/java/generator/src/cpp/common.h

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS
LOCAL_CFLAGS_arm64 += -O1

LOCAL_SRC_FILES := \
    modules/calib3d/src/dls.cpp \

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_calib3d

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/calib3d/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/calib3d/src \
    $(LOCAL_PATH)/modules/calib3d \
    $(LOCAL_PATH)/opencv2 \
    $(LOCAL_PATH)/modules/java/generator/src/cpp/common.h

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/calib3d/src/calibinit.cpp \
    modules/calib3d/src/calibration.cpp \
    modules/calib3d/src/checkchessboard.cpp \
    modules/calib3d/src/circlesgrid.cpp \
    modules/calib3d/src/compat_ptsetreg.cpp \
    modules/calib3d/src/compat_stereo.cpp \
    modules/calib3d/src/epnp.cpp \
    modules/calib3d/src/fisheye.cpp \
    modules/calib3d/src/five-point.cpp \
    modules/calib3d/src/fundam.cpp \
    modules/calib3d/src/homography_decomp.cpp \
    modules/calib3d/src/levmarq.cpp \
    modules/calib3d/src/p3p.cpp \
    modules/calib3d/src/polynom_solver.cpp \
    modules/calib3d/src/posit.cpp \
    modules/calib3d/src/ptsetreg.cpp \
    modules/calib3d/src/quadsubpix.cpp \
    modules/calib3d/src/rho.cpp \
    modules/calib3d/src/solvepnp.cpp \
    modules/calib3d/src/stereobm.cpp \
    modules/calib3d/src/stereosgbm.cpp \
    modules/calib3d/src/triangulate.cpp \
    modules/calib3d/src/upnp.cpp \
    modules/calib3d/opencl_kernels_calib3d.cpp

LOCAL_SHARED_LIBRARIES := libopencv_imgproc libopencv_flann libopencv_core libopencv_ml libopencv_imgcodecs libopencv_videoio libopencv_highgui libopencv_features2d
LOCAL_STATIC_LIBRARIES := libopencv_hal

# Bug: http://b/25691376
LOCAL_STATIC_LIBRARIES += libopencv_fix_dls

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_features2d

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/features2d \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/opencv2 \
    $(LOCAL_PATH)/features2d/src/kaze \
    $(LOCAL_PATH)/modules/java/generator/src/cpp

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/features2d/src/agast.cpp \
    modules/features2d/src/agast_score.cpp \
    modules/features2d/src/akaze.cpp \
    modules/features2d/src/bagofwords.cpp \
    modules/features2d/src/blobdetector.cpp \
    modules/features2d/src/brisk.cpp \
    modules/features2d/src/draw.cpp \
    modules/features2d/src/dynamic.cpp \
    modules/features2d/src/evaluation.cpp \
    modules/features2d/src/fast.cpp \
    modules/features2d/src/fast_score.cpp \
    modules/features2d/src/feature2d.cpp \
    modules/features2d/src/gftt.cpp \
    modules/features2d/src/kaze.cpp \
    modules/features2d/src/keypoint.cpp \
    modules/features2d/src/matchers.cpp \
    modules/features2d/src/mser.cpp \
    modules/features2d/src/orb.cpp \
    modules/features2d/src/kaze/KAZEFeatures.cpp \
    modules/features2d/src/kaze/nldiffusion_functions.cpp \
    modules/features2d/src/kaze/AKAZEFeatures.cpp \
    modules/features2d/src/kaze/fed.cpp \
    modules/features2d/opencl_kernels_features2d.cpp \
    modules/features2d/misc/java/src/cpp/features2d_converters.cpp \
    modules/java/generator/src/cpp/converters.cpp


LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_flann libopencv_imgproc libopencv_ml libopencv_imgcodecs libopencv_videoio libopencv_highgui
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_LDLIBS := -ldl

LOCAL_MODULE := libopencv_flann

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/opencv2

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/flann/src/miniflann.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_hal

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/hal \
    $(LOCAL_PATH)/modules/hal/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/hal/src/arithm.cpp \
    modules/hal/src/color.cpp \
    modules/hal/src/filter.cpp \
    modules/hal/src/mathfuncs.cpp \
    modules/hal/src/matrix.cpp \
    modules/hal/src/resize.cpp \
    modules/hal/src/stat.cpp \
    modules/hal/src/warp.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core

include $(BUILD_STATIC_LIBRARY)



include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_highgui

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/highgui/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/highgui \
    $(LOCAL_PATH)/opencv2

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/highgui/src/window.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libopencv_imgcodecs libopencv_videoio
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21
LOCAL_MODULE := libjasper

LOCAL_RTTI_FLAG := -frtti

LOCAL_CFLAGS := -DEXCLUDE_MIF_SUPPORT -DEXCLUDE_PNM_SUPPORT -DEXCLUDE_BMP_SUPPORT -DEXCLUDE_RAS_SUPPORT  -DEXCLUDE_JPG_SUPPORT -DEXCLUDE_PGX_SUPPORT -Wno-implicit-function-declaration

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/3rdparty/libjasper \
    $(LOCAL_PATH)/3rdparty/libjasper/jasper

LOCAL_SRC_FILES := \
    3rdparty/libjasper/jpc_enc.c \
    3rdparty/libjasper/jas_getopt.c \
    3rdparty/libjasper/jas_stream.c \
    3rdparty/libjasper/jas_string.c \
    3rdparty/libjasper/jpc_util.c \
    3rdparty/libjasper/jpc_bs.c \
    3rdparty/libjasper/jpc_tsfb.c \
    3rdparty/libjasper/jpc_math.c \
    3rdparty/libjasper/jas_version.c \
    3rdparty/libjasper/jpc_t2cod.c \
    3rdparty/libjasper/jpc_t2enc.c \
    3rdparty/libjasper/jpc_qmfb.c \
    3rdparty/libjasper/jas_init.c \
    3rdparty/libjasper/jpc_mct.c \
    3rdparty/libjasper/jp2_dec.c \
    3rdparty/libjasper/jas_iccdata.c \
    3rdparty/libjasper/jpc_cs.c \
    3rdparty/libjasper/jpc_t2dec.c \
    3rdparty/libjasper/jas_cm.c \
    3rdparty/libjasper/jpc_t1cod.c \
    3rdparty/libjasper/jas_tvp.c \
    3rdparty/libjasper/jp2_cod.c \
    3rdparty/libjasper/jpc_mqenc.c \
    3rdparty/libjasper/jp2_enc.c \
    3rdparty/libjasper/jas_seq.c \
    3rdparty/libjasper/jas_icc.c \
    3rdparty/libjasper/jpc_t1enc.c \
    3rdparty/libjasper/jas_malloc.c \
    3rdparty/libjasper/jas_debug.c \
    3rdparty/libjasper/jpc_tagtree.c \
    3rdparty/libjasper/jpc_mqdec.c \
    3rdparty/libjasper/jpc_mqcod.c \
    3rdparty/libjasper/jas_image.c \
    3rdparty/libjasper/jas_tmr.c \
    3rdparty/libjasper/jpc_dec.c \
    3rdparty/libjasper/jpc_t1dec.c

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21
LOCAL_MODULE := opencv_libjpeg

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/3rdparty/libjpeg

LOCAL_SRC_FILES := \
    3rdparty/libjpeg/jdapistd.c \
    3rdparty/libjpeg/jquant2.c \
    3rdparty/libjpeg/jdmerge.c \
    3rdparty/libjpeg/jdmaster.c \
    3rdparty/libjpeg/jmemmgr.c \
    3rdparty/libjpeg/jfdctint.c \
    3rdparty/libjpeg/jcmainct.c \
    3rdparty/libjpeg/jdapimin.c \
    3rdparty/libjpeg/jdatasrc.c \
    3rdparty/libjpeg/jdmarker.c \
    3rdparty/libjpeg/jdcolor.c \
    3rdparty/libjpeg/jctrans.c \
    3rdparty/libjpeg/jcapimin.c \
    3rdparty/libjpeg/jmemnobs.c \
    3rdparty/libjpeg/jchuff.c \
    3rdparty/libjpeg/jdpostct.c \
    3rdparty/libjpeg/jdcoefct.c \
    3rdparty/libjpeg/jcapistd.c \
    3rdparty/libjpeg/jutils.c \
    3rdparty/libjpeg/jdmainct.c \
    3rdparty/libjpeg/jdatadst.c \
    3rdparty/libjpeg/jquant1.c \
    3rdparty/libjpeg/jcinit.c \
    3rdparty/libjpeg/jddctmgr.c \
    3rdparty/libjpeg/jdinput.c \
    3rdparty/libjpeg/jidctfst.c \
    3rdparty/libjpeg/jcarith.c \
    3rdparty/libjpeg/jcomapi.c \
    3rdparty/libjpeg/jidctint.c \
    3rdparty/libjpeg/jcmarker.c \
    3rdparty/libjpeg/jdtrans.c \
    3rdparty/libjpeg/jccolor.c \
    3rdparty/libjpeg/jfdctfst.c \
    3rdparty/libjpeg/jdsample.c \
    3rdparty/libjpeg/jcmaster.c \
    3rdparty/libjpeg/jccoefct.c \
    3rdparty/libjpeg/jcparam.c \
    3rdparty/libjpeg/jaricom.c \
    3rdparty/libjpeg/jdhuff.c \
    3rdparty/libjpeg/jdarith.c \
    3rdparty/libjpeg/jfdctflt.c \
    3rdparty/libjpeg/jcprepct.c \
    3rdparty/libjpeg/jcsample.c \
    3rdparty/libjpeg/jidctflt.c \
    3rdparty/libjpeg/jcdctmgr.c \
    3rdparty/libjpeg/jerror.c

include $(BUILD_STATIC_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21
LOCAL_MODULE := libtiff

LOCAL_RTTI_FLAG := -frtti

LOCAL_LDLIBS := -lz

LOCAL_CFLAGS := -Wno-implicit-function-declaration

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/3rdparty/libtiff

LOCAL_SRC_FILES := \
    3rdparty/libtiff/tif_jpeg.c \
    3rdparty/libtiff/tif_write.c \
    3rdparty/libtiff/tif_error.c \
    3rdparty/libtiff/tif_swab.c \
    3rdparty/libtiff/tif_strip.c \
    3rdparty/libtiff/tif_extension.c \
    3rdparty/libtiff/tif_jpeg_12.c \
    3rdparty/libtiff/tif_pixarlog.c \
    3rdparty/libtiff/tif_dirwrite.c \
    3rdparty/libtiff/tif_dirread.c \
    3rdparty/libtiff/tif_flush.c \
    3rdparty/libtiff/tif_lzma.c \
    3rdparty/libtiff/tif_packbits.c \
    3rdparty/libtiff/tif_luv.c \
    3rdparty/libtiff/tif_next.c \
    3rdparty/libtiff/tif_aux.c \
    3rdparty/libtiff/tif_thunder.c \
    3rdparty/libtiff/tif_compress.c \
    3rdparty/libtiff/tif_codec.c \
    3rdparty/libtiff/tif_print.c \
    3rdparty/libtiff/tif_dumpmode.c \
    3rdparty/libtiff/tif_open.c \
    3rdparty/libtiff/tif_close.c \
    3rdparty/libtiff/tif_dir.c \
    3rdparty/libtiff/tif_fax3sm.c \
    3rdparty/libtiff/tif_read.c \
    3rdparty/libtiff/tif_zip.c \
    3rdparty/libtiff/tif_lzw.c \
    3rdparty/libtiff/tif_tile.c \
    3rdparty/libtiff/tif_warning.c \
    3rdparty/libtiff/tif_color.c \
    3rdparty/libtiff/tif_dirinfo.c \
    3rdparty/libtiff/tif_version.c \
    3rdparty/libtiff/tif_jbig.c \
    3rdparty/libtiff/tif_fax3.c \
    3rdparty/libtiff/tif_ojpeg.c \
    3rdparty/libtiff/tif_predict.c \
    3rdparty/libtiff/tif_getimage.c \
    3rdparty/libtiff/tif_unix.c

include $(BUILD_STATIC_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21
LOCAL_MODULE := libIlmImf

LOCAL_RTTI_FLAG := -frtti

LOCAL_CFLAGS := -fexceptions

LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/3rdparty/openexr \
    $(LOCAL_PATH)/3rdparty/openexr/IlmImf \
    $(LOCAL_PATH)/3rdparty/openexr/Half \
    $(LOCAL_PATH)/3rdparty/openexr/Iex \
    $(LOCAL_PATH)/3rdparty/openexr/IlmThread \
    $(LOCAL_PATH)/3rdparty/openexr/Imath


LOCAL_SRC_FILES := \
    3rdparty/openexr/IlmImf/ImfChannelList.cpp \
    3rdparty/openexr/IlmImf/ImfStdIO.cpp \
    3rdparty/openexr/IlmImf/ImfPreviewImageAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfFloatAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfLineOrderAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfTestFile.cpp \
    3rdparty/openexr/IlmImf/ImfInputFile.cpp \
    3rdparty/openexr/IlmImf/ImfTiledRgbaFile.cpp \
    3rdparty/openexr/IlmImf/ImfVecAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfRationalAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfBoxAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfRgbaFile.cpp \
    3rdparty/openexr/IlmImf/ImfTiledOutputFile.cpp \
    3rdparty/openexr/IlmImf/ImfCRgbaFile.cpp \
    3rdparty/openexr/IlmImf/ImfChromaticitiesAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfRleCompressor.cpp \
    3rdparty/openexr/IlmImf/ImfStandardAttributes.cpp \
    3rdparty/openexr/IlmImf/ImfChannelListAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfOpaqueAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfIntAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfThreading.cpp \
    3rdparty/openexr/IlmImf/ImfTiledInputFile.cpp \
    3rdparty/openexr/IlmImf/ImfEnvmapAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfKeyCodeAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfRgbaYca.cpp \
    3rdparty/openexr/IlmImf/ImfHuf.cpp \
    3rdparty/openexr/IlmImf/ImfTileDescriptionAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfVersion.cpp \
    3rdparty/openexr/IlmImf/ImfChromaticities.cpp \
    3rdparty/openexr/IlmImf/ImfStringVectorAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfStringAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfKeyCode.cpp \
    3rdparty/openexr/IlmImf/ImfOutputFile.cpp \
    3rdparty/openexr/IlmImf/ImfMatrixAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfAcesFile.cpp \
    3rdparty/openexr/IlmImf/ImfScanLineInputFile.cpp \
    3rdparty/openexr/IlmImf/ImfCompressor.cpp \
    3rdparty/openexr/IlmImf/ImfHeader.cpp \
    3rdparty/openexr/IlmImf/ImfFramesPerSecond.cpp \
    3rdparty/openexr/IlmImf/ImfEnvmap.cpp \
    3rdparty/openexr/IlmImf/ImfZipCompressor.cpp \
    3rdparty/openexr/IlmImf/ImfMultiView.cpp \
    3rdparty/openexr/IlmImf/ImfPizCompressor.cpp \
    3rdparty/openexr/IlmImf/ImfMisc.cpp \
    3rdparty/openexr/IlmImf/ImfRational.cpp \
    3rdparty/openexr/IlmImf/ImfAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfDoubleAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfFrameBuffer.cpp \
    3rdparty/openexr/IlmImf/ImfTiledMisc.cpp \
    3rdparty/openexr/IlmImf/ImfB44Compressor.cpp \
    3rdparty/openexr/IlmImf/ImfPxr24Compressor.cpp \
    3rdparty/openexr/IlmImf/ImfTimeCode.cpp \
    3rdparty/openexr/IlmImf/ImfLut.cpp \
    3rdparty/openexr/IlmImf/ImfTileOffsets.cpp \
    3rdparty/openexr/IlmImf/ImfConvert.cpp \
    3rdparty/openexr/IlmImf/ImfIO.cpp \
    3rdparty/openexr/IlmImf/ImfPreviewImage.cpp \
    3rdparty/openexr/IlmImf/ImfCompressionAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfTimeCodeAttribute.cpp \
    3rdparty/openexr/IlmImf/ImfWav.cpp \
    3rdparty/openexr/Half/eLut.cpp \
    3rdparty/openexr/Half/toFloat.cpp \
    3rdparty/openexr/Half/half.cpp \
    3rdparty/openexr/Iex/IexThrowErrnoExc.cpp \
    3rdparty/openexr/Iex/IexBaseExc.cpp \
    3rdparty/openexr/IlmThread/IlmThreadMutex.cpp \
    3rdparty/openexr/IlmThread/IlmThreadPool.cpp \
    3rdparty/openexr/IlmThread/IlmThreadPosix.cpp \
    3rdparty/openexr/IlmThread/IlmThreadMutexPosix.cpp \
    3rdparty/openexr/IlmThread/IlmThreadSemaphorePosixCompat.cpp \
    3rdparty/openexr/IlmThread/IlmThreadSemaphore.cpp \
    3rdparty/openexr/IlmThread/IlmThreadSemaphorePosix.cpp \
    3rdparty/openexr/IlmThread/IlmThread.cpp \
    3rdparty/openexr/Imath/ImathRandom.cpp \
    3rdparty/openexr/Imath/ImathFun.cpp \
    3rdparty/openexr/Imath/ImathMatrixAlgo.cpp \
    3rdparty/openexr/Imath/ImathVec.cpp \
    3rdparty/openexr/Imath/ImathColorAlgo.cpp


include $(BUILD_STATIC_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_imgcodecs

LOCAL_LDLIBS := -lz -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/imgcodecs \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/opencv2 \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/3rdparty/openexr/IlmImf \
    $(LOCAL_PATH)/3rdparty/openexr/Imath \
    $(LOCAL_PATH)/3rdparty/openexr/Iex \
    $(LOCAL_PATH)/3rdparty/openexr/Half \
    $(LOCAL_PATH)/3rdparty/libjasper \
    $(LOCAL_PATH)/3rdparty/libjasper/jasper \
    $(LOCAL_PATH)/3rdparty/libjpeg \
    $(LOCAL_PATH)/3rdparty/libtiff


LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/imgcodecs/src/bitstrm.cpp \
    modules/imgcodecs/src/grfmt_base.cpp \
    modules/imgcodecs/src/grfmt_bmp.cpp \
    modules/imgcodecs/src/grfmt_exr.cpp \
    modules/imgcodecs/src/grfmt_gdal.cpp \
    modules/imgcodecs/src/grfmt_hdr.cpp \
    modules/imgcodecs/src/grfmt_jpeg2000.cpp \
    modules/imgcodecs/src/grfmt_jpeg.cpp \
    modules/imgcodecs/src/grfmt_png.cpp \
    modules/imgcodecs/src/grfmt_pxm.cpp \
    modules/imgcodecs/src/grfmt_sunras.cpp \
    modules/imgcodecs/src/grfmt_tiff.cpp \
    modules/imgcodecs/src/grfmt_webp.cpp \
    modules/imgcodecs/src/loadsave.cpp \
    modules/imgcodecs/src/rgbe.cpp \
    modules/imgcodecs/src/utils.cpp

LOCAL_STATIC_LIBRARIES += libopencv_hal libjasper opencv_libjpeg libtiff libIlmImf
LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libpng

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_imgproc

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/imgproc \
    $(LOCAL_PATH)/modules/imgproc/src \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/opencv2

LOCAL_LDLIBS := -ldl

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/imgproc/src/accum.cpp \
    modules/imgproc/src/approx.cpp \
    modules/imgproc/src/blend.cpp \
    modules/imgproc/src/canny.cpp \
    modules/imgproc/src/clahe.cpp \
    modules/imgproc/src/color.cpp \
    modules/imgproc/src/colormap.cpp \
    modules/imgproc/src/connectedcomponents.cpp \
    modules/imgproc/src/contours.cpp \
    modules/imgproc/src/convhull.cpp \
    modules/imgproc/src/corner.cpp \
    modules/imgproc/src/cornersubpix.cpp \
    modules/imgproc/src/demosaicing.cpp \
    modules/imgproc/src/deriv.cpp \
    modules/imgproc/src/distransform.cpp \
    modules/imgproc/src/drawing.cpp \
    modules/imgproc/src/emd.cpp \
    modules/imgproc/src/featureselect.cpp \
    modules/imgproc/src/filter.cpp \
    modules/imgproc/src/floodfill.cpp \
    modules/imgproc/src/gabor.cpp \
    modules/imgproc/src/generalized_hough.cpp \
    modules/imgproc/src/geometry.cpp \
    modules/imgproc/src/grabcut.cpp \
    modules/imgproc/src/hershey_fonts.cpp \
    modules/imgproc/src/histogram.cpp \
    modules/imgproc/src/hough.cpp \
    modules/imgproc/src/imgwarp.cpp \
    modules/imgproc/src/intersection.cpp \
    modules/imgproc/src/linefit.cpp \
    modules/imgproc/src/lsd.cpp \
    modules/imgproc/src/matchcontours.cpp \
    modules/imgproc/src/min_enclosing_triangle.cpp \
    modules/imgproc/src/moments.cpp \
    modules/imgproc/src/morph.cpp \
    modules/imgproc/src/phasecorr.cpp \
    modules/imgproc/src/pyramids.cpp \
    modules/imgproc/src/rotcalipers.cpp \
    modules/imgproc/src/samplers.cpp \
    modules/imgproc/src/segmentation.cpp \
    modules/imgproc/src/shapedescr.cpp \
    modules/imgproc/src/smooth.cpp \
    modules/imgproc/src/subdivision2d.cpp \
    modules/imgproc/src/sumpixels.cpp \
    modules/imgproc/src/tables.cpp \
    modules/imgproc/src/templmatch.cpp \
    modules/imgproc/src/thresh.cpp \
    modules/imgproc/src/undistort.cpp \
    modules/imgproc/src/utils.cpp \
    modules/imgproc/opencl_kernels_imgproc.cpp \

LOCAL_SHARED_LIBRARIES := libopencv_core
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_ml

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/ml \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/ml/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/ml/src/ann_mlp.cpp \
    modules/ml/src/boost.cpp \
    modules/ml/src/data.cpp \
    modules/ml/src/em.cpp \
    modules/ml/src/gbt.cpp \
    modules/ml/src/inner_functions.cpp \
    modules/ml/src/kdtree.cpp \
    modules/ml/src/knearest.cpp \
    modules/ml/src/lr.cpp \
    modules/ml/src/nbayes.cpp \
    modules/ml/src/rtrees.cpp \
    modules/ml/src/svm.cpp \
    modules/ml/src/testset.cpp \
    modules/ml/src/tree.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

#Use true to build with renderscript, false to build without
WITH_RENDERSCRIPT = true

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_objdetect

LOCAL_RTTI_FLAG := -frtti

LOCAL_LDLIBS := -llog -ldl

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/objdetect \
    $(LOCAL_PATH)/modules/objdetect/src \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/objdetect/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/objdetect \
    $(LOCAL_PATH)/modules/ml/include \
    $(LOCAL_PATH)/modules/highgui/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/opencv2

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/objdetect/src/cascadedetect_convert.cpp \
    modules/objdetect/src/cascadedetect.cpp \
    modules/objdetect/src/detection_based_tracker.cpp \
    modules/objdetect/src/haar.cpp \
    modules/objdetect/src/hog.cpp \
    modules/objdetect/opencl_kernels_objdetect.cpp \
    modules/java/generator/src/cpp/converters.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libopencv_ml libopencv_imgcodecs libopencv_videoio libopencv_highgui

ifeq ($(WITH_RENDERSCRIPT), true)
LOCAL_SHARED_LIBRARIES += libopencv_rsobjdetect
LOCAL_CFLAGS += -DRENDERSCRIPT=1
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/rsobjdetect/src
endif

LOCAL_STATIC_LBIRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)




ifeq ($(WITH_RENDERSCRIPT),true)
include $(CLEAR_VARS)
LOCAL_MODULE := libopencv_rsobjdetect

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21
LOCAL_RENDERSCRIPT_TARGET_API := 21

LOCAL_C_INCLUDES := \
        modules/rsobjdetect/src

LOCAL_SRC_FILES:= \
        modules/rsobjdetect/src/rs/detectAt.rs \
        modules/rsobjdetect/src/innerloop.cpp

LOCAL_LDLIBS := -llog -ldl

LOCAL_RENDERSCRIPT_COMPATIBILITY := 21

LOCAL_C_INCLUDES := frameworks/rs/cpp
LOCAL_C_INCLUDES += frameworks/rs
LOCAL_C_INCLUDES += $(call intermediates-dir-for,STATIC_LIBRARIES,libRS,TARGET,)

LOCAL_STATIC_LIBRARIES := libRScpp_static

LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)
endif




include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_photo

LOCAL_LDLIBS := -ldl -lz

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/photo \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/photo/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/highgui/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/photo

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/photo/src/align.cpp \
    modules/photo/src/calibrate.cpp \
    modules/photo/src/contrast_preserve.cpp \
    modules/photo/src/denoise_tvl1.cpp \
    modules/photo/src/denoising.cpp \
    modules/photo/src/denoising.cuda.cpp \
    modules/photo/src/hdr_common.cpp \
    modules/photo/src/inpaint.cpp \
    modules/photo/src/merge.cpp \
    modules/photo/src/npr.cpp \
    modules/photo/src/seamless_cloning.cpp \
    modules/photo/src/seamless_cloning_impl.cpp \
    modules/photo/src/tonemap.cpp \
    modules/photo/opencl_kernels_photo.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libpng libjpeg
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_LDLIBS := -ldl

LOCAL_MODULE := libopencv_shape

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/shape \
    $(LOCAL_PATH)/modules/video/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/shape/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/shape/src/aff_trans.cpp \
    modules/shape/src/emdL1.cpp \
    modules/shape/src/haus_dis.cpp \
    modules/shape/src/hist_cost.cpp \
    modules/shape/src/precomp.cpp \
    modules/shape/src/sc_dis.cpp \
    modules/shape/src/tps_trans.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libopencv_video
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_stitching

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/stitching \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/calib3d/include \
    $(LOCAL_PATH)/modules/stitching/include \
    $(LOCAL_PATH)/modules/stitching

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/stitching/src/autocalib.cpp \
    modules/stitching/src/blenders.cpp \
    modules/stitching/src/camera.cpp \
    modules/stitching/src/exposure_compensate.cpp \
    modules/stitching/src/matchers.cpp \
    modules/stitching/src/motion_estimators.cpp \
    modules/stitching/src/seam_finders.cpp \
    modules/stitching/src/stitcher.cpp \
    modules/stitching/src/timelapsers.cpp \
    modules/stitching/src/util.cpp \
    modules/stitching/src/warpers.cpp \
    modules/stitching/src/warpers_cuda.cpp \
    modules/stitching/opencl_kernels_stitching.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_flann libopencv_imgproc libopencv_ml libopencv_imgcodecs libopencv_videoio libopencv_highgui libopencv_objdetect libopencv_features2d libopencv_calib3d
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_superres

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/superres \
    $(LOCAL_PATH)/modules/video/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/superres/include \
    $(LOCAL_PATH)/modules/superres/src

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/superres/src/btv_l1.cpp \
    modules/superres/src/btv_l1_cuda.cpp \
    modules/superres/src/frame_source.cpp \
    modules/superres/src/input_array_utility.cpp \
    modules/superres/src/optical_flow.cpp \
    modules/superres/src/super_resolution.cpp \
    modules/superres/opencl_kernels_superres.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libopencv_video libopencv_imgcodecs libopencv_videoio
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_ts

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/ts \
    $(LOCAL_PATH)/modules/highgui/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/ts/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/ts/src/cuda_perf.cpp \
    modules/ts/src/cuda_test.cpp \
    modules/ts/src/ocl_perf.cpp \
    modules/ts/src/ocl_test.cpp \
    modules/ts/src/ts_arrtest.cpp \
    modules/ts/src/ts.cpp \
    modules/ts/src/ts_func.cpp \
    modules/ts/src/ts_gtest.cpp \
    modules/ts/src/ts_perf.cpp

include $(BUILD_STATIC_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_video

LOCAL_LDLIBS := -lz -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/video \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/video/include \
    $(LOCAL_PATH)/modules/imgcodecs/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/video/src/bgfg_gaussmix2.cpp \
    modules/video/src/bgfg_KNN.cpp \
    modules/video/src/camshift.cpp \
    modules/video/src/compat_video.cpp \
    modules/video/src/ecc.cpp \
    modules/video/src/kalman.cpp \
    modules/video/src/lkpyramid.cpp \
    modules/video/src/optflowgf.cpp \
    modules/video/src/tvl1flow.cpp \
    modules/video/opencl_kernels_video.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_videoio

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/videoio \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/video

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/videoio/src/cap_cmu.cpp \
    modules/videoio/src/cap.cpp \
    modules/videoio/src/cap_dc1394.cpp \
    modules/videoio/src/cap_dc1394_v2.cpp \
    modules/videoio/src/cap_dshow.cpp \
    modules/videoio/src/cap_ffmpeg.cpp \
    modules/videoio/src/cap_gphoto2.cpp \
    modules/videoio/src/cap_images.cpp \
    modules/videoio/src/cap_intelperc.cpp \
    modules/videoio/src/cap_libv4l.cpp \
    modules/videoio/src/cap_mjpeg_decoder.cpp \
    modules/videoio/src/cap_mjpeg_encoder.cpp \
    modules/videoio/src/cap_msmf.cpp \
    modules/videoio/src/cap_openni2.cpp \
    modules/videoio/src/cap_openni.cpp \
    modules/videoio/src/cap_pvapi.cpp \
    modules/videoio/src/cap_v4l.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_imgproc libopencv_imgcodecs
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_videostab

LOCAL_LDLIBS := -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/videostab \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/video/include \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/photo/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/calib3d/include \
    $(LOCAL_PATH)/modules/videostab/include

LOCAL_CFLAGS := -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS

LOCAL_SRC_FILES := \
    modules/videostab/src/deblurring.cpp \
    modules/videostab/src/fast_marching.cpp \
    modules/videostab/src/frame_source.cpp \
    modules/videostab/src/global_motion.cpp \
    modules/videostab/src/inpainting.cpp \
    modules/videostab/src/log.cpp \
    modules/videostab/src/motion_stabilizing.cpp \
    modules/videostab/src/optical_flow.cpp \
    modules/videostab/src/outlier_rejection.cpp \
    modules/videostab/src/stabilizer.cpp \
    modules/videostab/src/wobble_suppression.cpp
LOCAL_SHARED_LIBRARIES:= libopencv_core libopencv_flann libopencv_imgproc libopencv_ml libopencv_photo libopencv_video libopencv_imgcodecs libopencv_videoio libopencv_highgui libopencv_features2d libopencv_calib3d

LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 21

LOCAL_MODULE := libopencv_java

LOCAL_LDLIBS := -llog -lz -ljnigraphics -ldl

LOCAL_RTTI_FLAG := -frtti

LOCAL_CFLAGS := -fexceptions -DANDROID -D__OPENCV_BUILD=1 -Dopencv_java_EXPORTS -DCAP_PROP_ANDROID_PREVIEW_SIZES_STRING=1025

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/modules/java/include \
    $(LOCAL_PATH)/modules/java/src \
    $(LOCAL_PATH)/modules \
    $(LOCAL_PATH)/modules/hal/include \
    $(LOCAL_PATH)/modules/core/include \
    $(LOCAL_PATH)/modules/flann/include \
    $(LOCAL_PATH)/modules/imgproc/include \
    $(LOCAL_PATH)/modules/ml/include \
    $(LOCAL_PATH)/modules/photo/include \
    $(LOCAL_PATH)/modules/video/include \
    $(LOCAL_PATH)/modules/androidcamera/include \
    $(LOCAL_PATH)/modules/imgcodecs/include \
    $(LOCAL_PATH)/modules/videoio/include \
    $(LOCAL_PATH)/modules/highgui/include \
    $(LOCAL_PATH)/modules/objdetect/include \
    $(LOCAL_PATH)/modules/features2d/include \
    $(LOCAL_PATH)/modules/calib3d/include \
    $(LOCAL_PATH)/modules/java/generator/src/cpp

LOCAL_SRC_FILES := \
    modules/java/src/ml.cpp \
    modules/java/src/video.cpp \
    modules/java/src/photo.cpp \
    modules/java/src/calib3d.cpp \
    modules/java/src/features2d.cpp \
    modules/java/src/core.cpp \
    modules/java/src/imgproc.cpp \
    modules/java/src/objdetect.cpp \
    modules/java/src/videoio.cpp \
    modules/java/src/imgcodecs.cpp \
    modules/java/generator/src/cpp/jni_part.cpp \
    modules/java/generator/src/cpp/utils.cpp \
    modules/java/generator/src/cpp/converters.cpp \
    modules/java/generator/src/cpp/Mat.cpp \
    modules/core/misc/java/src/cpp/core_manual.cpp

LOCAL_SHARED_LIBRARIES := libopencv_core libopencv_flann libopencv_imgproc libopencv_ml libopencv_photo libopencv_video libopencv_imgcodecs libopencv_videoio libopencv_highgui libopencv_objdetect libopencv_features2d libopencv_calib3d
LOCAL_STATIC_LIBRARIES := libopencv_hal

include $(BUILD_SHARED_LIBRARY)
