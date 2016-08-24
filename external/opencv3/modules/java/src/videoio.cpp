
//
// This file is auto-generated, please don't edit!
//

#define LOG_TAG "org.opencv.videoio"

#include "common.h"

#include "opencv2/opencv_modules.hpp"
#ifdef HAVE_OPENCV_VIDEOIO

#include <string>

#include "opencv2/videoio.hpp"

#include "../../videoio/include/opencv2/videoio/videoio_c.h"
#include "../../videoio/include/opencv2/videoio.hpp"

using namespace cv;

/// throw java exception
static void throwJavaException(JNIEnv *env, const std::exception *e, const char *method) {
  std::string what = "unknown exception";
  jclass je = 0;

  if(e) {
    std::string exception_type = "std::exception";

    if(dynamic_cast<const cv::Exception*>(e)) {
      exception_type = "cv::Exception";
      je = env->FindClass("org/opencv/core/CvException");
    }

    what = exception_type + ": " + e->what();
  }

  if(!je) je = env->FindClass("java/lang/Exception");
  env->ThrowNew(je, what.c_str());

  LOGE("%s caught %s", method, what.c_str());
  (void)method;        // avoid "unused" warning
}


extern "C" {


//
//   VideoCapture()
//

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "videoio::VideoCapture_10()";
    try {
        LOGD("%s", method_name);
        
        cv::VideoCapture* _retval_ = new cv::VideoCapture(  );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//   VideoCapture(String filename)
//

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_11 (JNIEnv*, jclass, jstring);

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_11
  (JNIEnv* env, jclass , jstring filename)
{
    static const char method_name[] = "videoio::VideoCapture_11()";
    try {
        LOGD("%s", method_name);
        const char* utf_filename = env->GetStringUTFChars(filename, 0); String n_filename( utf_filename ? utf_filename : "" ); env->ReleaseStringUTFChars(filename, utf_filename);
        cv::VideoCapture* _retval_ = new cv::VideoCapture( n_filename );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//   VideoCapture(int device)
//

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_12 (JNIEnv*, jclass, jint);

JNIEXPORT jlong JNICALL Java_org_opencv_videoio_VideoCapture_VideoCapture_12
  (JNIEnv* env, jclass , jint device)
{
    static const char method_name[] = "videoio::VideoCapture_12()";
    try {
        LOGD("%s", method_name);
        
        cv::VideoCapture* _retval_ = new cv::VideoCapture( (int)device );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool open(String filename)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_open_10 (JNIEnv*, jclass, jlong, jstring);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_open_10
  (JNIEnv* env, jclass , jlong self, jstring filename)
{
    static const char method_name[] = "videoio::open_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        const char* utf_filename = env->GetStringUTFChars(filename, 0); String n_filename( utf_filename ? utf_filename : "" ); env->ReleaseStringUTFChars(filename, utf_filename);
        bool _retval_ = me->open( n_filename );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool open(int device)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_open_11 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_open_11
  (JNIEnv* env, jclass , jlong self, jint device)
{
    static const char method_name[] = "videoio::open_11()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        bool _retval_ = me->open( (int)device );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool isOpened()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_isOpened_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_isOpened_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "videoio::isOpened_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        bool _retval_ = me->isOpened(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void release()
//

JNIEXPORT void JNICALL Java_org_opencv_videoio_VideoCapture_release_10 (JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_videoio_VideoCapture_release_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "videoio::release_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        me->release(  );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool grab()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_grab_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_grab_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "videoio::grab_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        bool _retval_ = me->grab(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool retrieve(Mat& image, int flag = 0)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_retrieve_10 (JNIEnv*, jclass, jlong, jlong, jint);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_retrieve_10
  (JNIEnv* env, jclass , jlong self, jlong image_nativeObj, jint flag)
{
    static const char method_name[] = "videoio::retrieve_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        Mat& image = *((Mat*)image_nativeObj);
        bool _retval_ = me->retrieve( image, (int)flag );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_retrieve_11 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_retrieve_11
  (JNIEnv* env, jclass , jlong self, jlong image_nativeObj)
{
    static const char method_name[] = "videoio::retrieve_11()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        Mat& image = *((Mat*)image_nativeObj);
        bool _retval_ = me->retrieve( image );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool read(Mat& image)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_read_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_read_10
  (JNIEnv* env, jclass , jlong self, jlong image_nativeObj)
{
    static const char method_name[] = "videoio::read_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        Mat& image = *((Mat*)image_nativeObj);
        bool _retval_ = me->read( image );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool set(int propId, double value)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_set_10 (JNIEnv*, jclass, jlong, jint, jdouble);

JNIEXPORT jboolean JNICALL Java_org_opencv_videoio_VideoCapture_set_10
  (JNIEnv* env, jclass , jlong self, jint propId, jdouble value)
{
    static const char method_name[] = "videoio::set_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        bool _retval_ = me->set( (int)propId, (double)value );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  double get(int propId)
//

JNIEXPORT jdouble JNICALL Java_org_opencv_videoio_VideoCapture_get_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT jdouble JNICALL Java_org_opencv_videoio_VideoCapture_get_10
  (JNIEnv* env, jclass , jlong self, jint propId)
{
    static const char method_name[] = "videoio::get_10()";
    try {
        LOGD("%s", method_name);
        cv::VideoCapture* me = (cv::VideoCapture*) self; //TODO: check for NULL
        double _retval_ = me->get( (int)propId );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jstring JNICALL Java_org_opencv_videoio_VideoCapture_getSupportedPreviewSizes_10
  (JNIEnv *env, jclass, jlong self);

JNIEXPORT jstring JNICALL Java_org_opencv_videoio_VideoCapture_getSupportedPreviewSizes_10
  (JNIEnv *env, jclass, jlong self)
{
    static const char method_name[] = "videoio::VideoCapture_getSupportedPreviewSizes_10()";
    try {
        LOGD("%s", method_name);
        VideoCapture* me = (VideoCapture*) self; //TODO: check for NULL
        union {double prop; const char* name;} u;
        u.prop = me->get(CAP_PROP_ANDROID_PREVIEW_SIZES_STRING);
        return env->NewStringUTF(u.name);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return env->NewStringUTF("");
}


//
//  native support for java finalize()
//  static void cv::VideoCapture::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_videoio_VideoCapture_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_videoio_VideoCapture_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (cv::VideoCapture*) self;
}



} // extern "C"

#endif // HAVE_OPENCV_VIDEOIO
