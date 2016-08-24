
//
// This file is auto-generated, please don't edit!
//

#define LOG_TAG "org.opencv.video"

#include "common.h"

#include "opencv2/opencv_modules.hpp"
#ifdef HAVE_OPENCV_VIDEO

#include <string>

#include "opencv2/video.hpp"

#include "../../video/include/opencv2/video/tracking.hpp"
#include "../../video/include/opencv2/video/background_segm.hpp"

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
//  native support for java finalize()
//  static void Ptr<cv::DualTVL1OpticalFlow>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_DualTVL1OpticalFlow_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_DualTVL1OpticalFlow_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::DualTVL1OpticalFlow>*) self;
}


//
//  void calc(Mat I0, Mat I1, Mat& flow)
//

JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_calc_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_calc_10
  (JNIEnv* env, jclass , jlong self, jlong I0_nativeObj, jlong I1_nativeObj, jlong flow_nativeObj)
{
    static const char method_name[] = "video::calc_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::DenseOpticalFlow>* me = (Ptr<cv::DenseOpticalFlow>*) self; //TODO: check for NULL
        Mat& I0 = *((Mat*)I0_nativeObj);
        Mat& I1 = *((Mat*)I1_nativeObj);
        Mat& flow = *((Mat*)flow_nativeObj);
        (*me)->calc( I0, I1, flow );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void collectGarbage()
//

JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_collectGarbage_10 (JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_collectGarbage_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::collectGarbage_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::DenseOpticalFlow>* me = (Ptr<cv::DenseOpticalFlow>*) self; //TODO: check for NULL
        (*me)->collectGarbage(  );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  native support for java finalize()
//  static void Ptr<cv::DenseOpticalFlow>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_DenseOpticalFlow_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::DenseOpticalFlow>*) self;
}


//
//  void getBackgroundImage(Mat& backgroundImage)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_getBackgroundImage_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_getBackgroundImage_10
  (JNIEnv* env, jclass , jlong self, jlong backgroundImage_nativeObj)
{
    static const char method_name[] = "video::getBackgroundImage_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractor>* me = (Ptr<cv::BackgroundSubtractor>*) self; //TODO: check for NULL
        Mat& backgroundImage = *((Mat*)backgroundImage_nativeObj);
        (*me)->getBackgroundImage( backgroundImage );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void apply(Mat image, Mat& fgmask, double learningRate = -1)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_apply_10 (JNIEnv*, jclass, jlong, jlong, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_apply_10
  (JNIEnv* env, jclass , jlong self, jlong image_nativeObj, jlong fgmask_nativeObj, jdouble learningRate)
{
    static const char method_name[] = "video::apply_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractor>* me = (Ptr<cv::BackgroundSubtractor>*) self; //TODO: check for NULL
        Mat& image = *((Mat*)image_nativeObj);
        Mat& fgmask = *((Mat*)fgmask_nativeObj);
        (*me)->apply( image, fgmask, (double)learningRate );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_apply_11 (JNIEnv*, jclass, jlong, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_apply_11
  (JNIEnv* env, jclass , jlong self, jlong image_nativeObj, jlong fgmask_nativeObj)
{
    static const char method_name[] = "video::apply_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractor>* me = (Ptr<cv::BackgroundSubtractor>*) self; //TODO: check for NULL
        Mat& image = *((Mat*)image_nativeObj);
        Mat& fgmask = *((Mat*)fgmask_nativeObj);
        (*me)->apply( image, fgmask );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  native support for java finalize()
//  static void Ptr<cv::BackgroundSubtractor>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractor_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::BackgroundSubtractor>*) self;
}


//
//  RotatedRect CamShift(Mat probImage, Rect& window, TermCriteria criteria)
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_video_Video_CamShift_10 (JNIEnv*, jclass, jlong, jint, jint, jint, jint, jdoubleArray, jint, jint, jdouble);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_video_Video_CamShift_10
  (JNIEnv* env, jclass , jlong probImage_nativeObj, jint window_x, jint window_y, jint window_width, jint window_height, jdoubleArray window_out, jint criteria_type, jint criteria_maxCount, jdouble criteria_epsilon)
{
    static const char method_name[] = "video::CamShift_10()";
    try {
        LOGD("%s", method_name);
        Mat& probImage = *((Mat*)probImage_nativeObj);
        Rect window(window_x, window_y, window_width, window_height);
        TermCriteria criteria(criteria_type, criteria_maxCount, criteria_epsilon);
        RotatedRect _retval_ = cv::CamShift( probImage, window, criteria );
        jdoubleArray _da_retval_ = env->NewDoubleArray(5);  jdouble _tmp_retval_[5] = {_retval_.center.x, _retval_.center.y, _retval_.size.width, _retval_.size.height, _retval_.angle}; env->SetDoubleArrayRegion(_da_retval_, 0, 5, _tmp_retval_);  jdouble tmp_window[4] = {window.x, window.y, window.width, window.height}; env->SetDoubleArrayRegion(window_out, 0, 4, tmp_window);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int meanShift(Mat probImage, Rect& window, TermCriteria criteria)
//

JNIEXPORT jint JNICALL Java_org_opencv_video_Video_meanShift_10 (JNIEnv*, jclass, jlong, jint, jint, jint, jint, jdoubleArray, jint, jint, jdouble);

JNIEXPORT jint JNICALL Java_org_opencv_video_Video_meanShift_10
  (JNIEnv* env, jclass , jlong probImage_nativeObj, jint window_x, jint window_y, jint window_width, jint window_height, jdoubleArray window_out, jint criteria_type, jint criteria_maxCount, jdouble criteria_epsilon)
{
    static const char method_name[] = "video::meanShift_10()";
    try {
        LOGD("%s", method_name);
        Mat& probImage = *((Mat*)probImage_nativeObj);
        Rect window(window_x, window_y, window_width, window_height);
        TermCriteria criteria(criteria_type, criteria_maxCount, criteria_epsilon);
        int _retval_ = cv::meanShift( probImage, window, criteria );
        jdouble tmp_window[4] = {window.x, window.y, window.width, window.height}; env->SetDoubleArrayRegion(window_out, 0, 4, tmp_window);
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int buildOpticalFlowPyramid(Mat img, vector_Mat& pyramid, Size winSize, int maxLevel, bool withDerivatives = true, int pyrBorder = BORDER_REFLECT_101, int derivBorder = BORDER_CONSTANT, bool tryReuseInputImage = true)
//

JNIEXPORT jint JNICALL Java_org_opencv_video_Video_buildOpticalFlowPyramid_10 (JNIEnv*, jclass, jlong, jlong, jdouble, jdouble, jint, jboolean, jint, jint, jboolean);

JNIEXPORT jint JNICALL Java_org_opencv_video_Video_buildOpticalFlowPyramid_10
  (JNIEnv* env, jclass , jlong img_nativeObj, jlong pyramid_mat_nativeObj, jdouble winSize_width, jdouble winSize_height, jint maxLevel, jboolean withDerivatives, jint pyrBorder, jint derivBorder, jboolean tryReuseInputImage)
{
    static const char method_name[] = "video::buildOpticalFlowPyramid_10()";
    try {
        LOGD("%s", method_name);
        std::vector<Mat> pyramid;
        Mat& pyramid_mat = *((Mat*)pyramid_mat_nativeObj);
        Mat& img = *((Mat*)img_nativeObj);
        Size winSize((int)winSize_width, (int)winSize_height);
        int _retval_ = cv::buildOpticalFlowPyramid( img, pyramid, winSize, (int)maxLevel, (bool)withDerivatives, (int)pyrBorder, (int)derivBorder, (bool)tryReuseInputImage );
        vector_Mat_to_Mat( pyramid, pyramid_mat );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jint JNICALL Java_org_opencv_video_Video_buildOpticalFlowPyramid_11 (JNIEnv*, jclass, jlong, jlong, jdouble, jdouble, jint);

JNIEXPORT jint JNICALL Java_org_opencv_video_Video_buildOpticalFlowPyramid_11
  (JNIEnv* env, jclass , jlong img_nativeObj, jlong pyramid_mat_nativeObj, jdouble winSize_width, jdouble winSize_height, jint maxLevel)
{
    static const char method_name[] = "video::buildOpticalFlowPyramid_11()";
    try {
        LOGD("%s", method_name);
        std::vector<Mat> pyramid;
        Mat& pyramid_mat = *((Mat*)pyramid_mat_nativeObj);
        Mat& img = *((Mat*)img_nativeObj);
        Size winSize((int)winSize_width, (int)winSize_height);
        int _retval_ = cv::buildOpticalFlowPyramid( img, pyramid, winSize, (int)maxLevel );
        vector_Mat_to_Mat( pyramid, pyramid_mat );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void calcOpticalFlowPyrLK(Mat prevImg, Mat nextImg, vector_Point2f prevPts, vector_Point2f& nextPts, vector_uchar& status, vector_float& err, Size winSize = Size(21,21), int maxLevel = 3, TermCriteria criteria = TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, 0.01), int flags = 0, double minEigThreshold = 1e-4)
//

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong, jlong, jdouble, jdouble, jint, jint, jint, jdouble, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_10
  (JNIEnv* env, jclass , jlong prevImg_nativeObj, jlong nextImg_nativeObj, jlong prevPts_mat_nativeObj, jlong nextPts_mat_nativeObj, jlong status_mat_nativeObj, jlong err_mat_nativeObj, jdouble winSize_width, jdouble winSize_height, jint maxLevel, jint criteria_type, jint criteria_maxCount, jdouble criteria_epsilon, jint flags, jdouble minEigThreshold)
{
    static const char method_name[] = "video::calcOpticalFlowPyrLK_10()";
    try {
        LOGD("%s", method_name);
        std::vector<Point2f> prevPts;
        Mat& prevPts_mat = *((Mat*)prevPts_mat_nativeObj);
        Mat_to_vector_Point2f( prevPts_mat, prevPts );
        std::vector<Point2f> nextPts;
        Mat& nextPts_mat = *((Mat*)nextPts_mat_nativeObj);
        Mat_to_vector_Point2f( nextPts_mat, nextPts );
        std::vector<uchar> status;
        Mat& status_mat = *((Mat*)status_mat_nativeObj);
        std::vector<float> err;
        Mat& err_mat = *((Mat*)err_mat_nativeObj);
        Mat& prevImg = *((Mat*)prevImg_nativeObj);
        Mat& nextImg = *((Mat*)nextImg_nativeObj);
        Size winSize((int)winSize_width, (int)winSize_height);
        TermCriteria criteria(criteria_type, criteria_maxCount, criteria_epsilon);
        cv::calcOpticalFlowPyrLK( prevImg, nextImg, prevPts, nextPts, status, err, winSize, (int)maxLevel, criteria, (int)flags, (double)minEigThreshold );
        vector_Point2f_to_Mat( nextPts, nextPts_mat );  vector_uchar_to_Mat( status, status_mat );  vector_float_to_Mat( err, err_mat );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_11 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong, jlong, jdouble, jdouble, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_11
  (JNIEnv* env, jclass , jlong prevImg_nativeObj, jlong nextImg_nativeObj, jlong prevPts_mat_nativeObj, jlong nextPts_mat_nativeObj, jlong status_mat_nativeObj, jlong err_mat_nativeObj, jdouble winSize_width, jdouble winSize_height, jint maxLevel)
{
    static const char method_name[] = "video::calcOpticalFlowPyrLK_11()";
    try {
        LOGD("%s", method_name);
        std::vector<Point2f> prevPts;
        Mat& prevPts_mat = *((Mat*)prevPts_mat_nativeObj);
        Mat_to_vector_Point2f( prevPts_mat, prevPts );
        std::vector<Point2f> nextPts;
        Mat& nextPts_mat = *((Mat*)nextPts_mat_nativeObj);
        Mat_to_vector_Point2f( nextPts_mat, nextPts );
        std::vector<uchar> status;
        Mat& status_mat = *((Mat*)status_mat_nativeObj);
        std::vector<float> err;
        Mat& err_mat = *((Mat*)err_mat_nativeObj);
        Mat& prevImg = *((Mat*)prevImg_nativeObj);
        Mat& nextImg = *((Mat*)nextImg_nativeObj);
        Size winSize((int)winSize_width, (int)winSize_height);
        cv::calcOpticalFlowPyrLK( prevImg, nextImg, prevPts, nextPts, status, err, winSize, (int)maxLevel );
        vector_Point2f_to_Mat( nextPts, nextPts_mat );  vector_uchar_to_Mat( status, status_mat );  vector_float_to_Mat( err, err_mat );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_12 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowPyrLK_12
  (JNIEnv* env, jclass , jlong prevImg_nativeObj, jlong nextImg_nativeObj, jlong prevPts_mat_nativeObj, jlong nextPts_mat_nativeObj, jlong status_mat_nativeObj, jlong err_mat_nativeObj)
{
    static const char method_name[] = "video::calcOpticalFlowPyrLK_12()";
    try {
        LOGD("%s", method_name);
        std::vector<Point2f> prevPts;
        Mat& prevPts_mat = *((Mat*)prevPts_mat_nativeObj);
        Mat_to_vector_Point2f( prevPts_mat, prevPts );
        std::vector<Point2f> nextPts;
        Mat& nextPts_mat = *((Mat*)nextPts_mat_nativeObj);
        Mat_to_vector_Point2f( nextPts_mat, nextPts );
        std::vector<uchar> status;
        Mat& status_mat = *((Mat*)status_mat_nativeObj);
        std::vector<float> err;
        Mat& err_mat = *((Mat*)err_mat_nativeObj);
        Mat& prevImg = *((Mat*)prevImg_nativeObj);
        Mat& nextImg = *((Mat*)nextImg_nativeObj);
        cv::calcOpticalFlowPyrLK( prevImg, nextImg, prevPts, nextPts, status, err );
        vector_Point2f_to_Mat( nextPts, nextPts_mat );  vector_uchar_to_Mat( status, status_mat );  vector_float_to_Mat( err, err_mat );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void calcOpticalFlowFarneback(Mat prev, Mat next, Mat& flow, double pyr_scale, int levels, int winsize, int iterations, int poly_n, double poly_sigma, int flags)
//

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowFarneback_10 (JNIEnv*, jclass, jlong, jlong, jlong, jdouble, jint, jint, jint, jint, jdouble, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_Video_calcOpticalFlowFarneback_10
  (JNIEnv* env, jclass , jlong prev_nativeObj, jlong next_nativeObj, jlong flow_nativeObj, jdouble pyr_scale, jint levels, jint winsize, jint iterations, jint poly_n, jdouble poly_sigma, jint flags)
{
    static const char method_name[] = "video::calcOpticalFlowFarneback_10()";
    try {
        LOGD("%s", method_name);
        Mat& prev = *((Mat*)prev_nativeObj);
        Mat& next = *((Mat*)next_nativeObj);
        Mat& flow = *((Mat*)flow_nativeObj);
        cv::calcOpticalFlowFarneback( prev, next, flow, (double)pyr_scale, (int)levels, (int)winsize, (int)iterations, (int)poly_n, (double)poly_sigma, (int)flags );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat estimateRigidTransform(Mat src, Mat dst, bool fullAffine)
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_estimateRigidTransform_10 (JNIEnv*, jclass, jlong, jlong, jboolean);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_estimateRigidTransform_10
  (JNIEnv* env, jclass , jlong src_nativeObj, jlong dst_nativeObj, jboolean fullAffine)
{
    static const char method_name[] = "video::estimateRigidTransform_10()";
    try {
        LOGD("%s", method_name);
        Mat& src = *((Mat*)src_nativeObj);
        Mat& dst = *((Mat*)dst_nativeObj);
        ::Mat _retval_ = cv::estimateRigidTransform( src, dst, (bool)fullAffine );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  double findTransformECC(Mat templateImage, Mat inputImage, Mat& warpMatrix, int motionType = MOTION_AFFINE, TermCriteria criteria = TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 50, 0.001), Mat inputMask = Mat())
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_10 (JNIEnv*, jclass, jlong, jlong, jlong, jint, jint, jint, jdouble, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_10
  (JNIEnv* env, jclass , jlong templateImage_nativeObj, jlong inputImage_nativeObj, jlong warpMatrix_nativeObj, jint motionType, jint criteria_type, jint criteria_maxCount, jdouble criteria_epsilon, jlong inputMask_nativeObj)
{
    static const char method_name[] = "video::findTransformECC_10()";
    try {
        LOGD("%s", method_name);
        Mat& templateImage = *((Mat*)templateImage_nativeObj);
        Mat& inputImage = *((Mat*)inputImage_nativeObj);
        Mat& warpMatrix = *((Mat*)warpMatrix_nativeObj);
        TermCriteria criteria(criteria_type, criteria_maxCount, criteria_epsilon);
        Mat& inputMask = *((Mat*)inputMask_nativeObj);
        double _retval_ = cv::findTransformECC( templateImage, inputImage, warpMatrix, (int)motionType, criteria, inputMask );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_11 (JNIEnv*, jclass, jlong, jlong, jlong, jint);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_11
  (JNIEnv* env, jclass , jlong templateImage_nativeObj, jlong inputImage_nativeObj, jlong warpMatrix_nativeObj, jint motionType)
{
    static const char method_name[] = "video::findTransformECC_11()";
    try {
        LOGD("%s", method_name);
        Mat& templateImage = *((Mat*)templateImage_nativeObj);
        Mat& inputImage = *((Mat*)inputImage_nativeObj);
        Mat& warpMatrix = *((Mat*)warpMatrix_nativeObj);
        double _retval_ = cv::findTransformECC( templateImage, inputImage, warpMatrix, (int)motionType );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_12 (JNIEnv*, jclass, jlong, jlong, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_Video_findTransformECC_12
  (JNIEnv* env, jclass , jlong templateImage_nativeObj, jlong inputImage_nativeObj, jlong warpMatrix_nativeObj)
{
    static const char method_name[] = "video::findTransformECC_12()";
    try {
        LOGD("%s", method_name);
        Mat& templateImage = *((Mat*)templateImage_nativeObj);
        Mat& inputImage = *((Mat*)inputImage_nativeObj);
        Mat& warpMatrix = *((Mat*)warpMatrix_nativeObj);
        double _retval_ = cv::findTransformECC( templateImage, inputImage, warpMatrix );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Ptr_BackgroundSubtractorMOG2 createBackgroundSubtractorMOG2(int history = 500, double varThreshold = 16, bool detectShadows = true)
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorMOG2_10 (JNIEnv*, jclass, jint, jdouble, jboolean);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorMOG2_10
  (JNIEnv* env, jclass , jint history, jdouble varThreshold, jboolean detectShadows)
{
    static const char method_name[] = "video::createBackgroundSubtractorMOG2_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::BackgroundSubtractorMOG2> Ptr_BackgroundSubtractorMOG2;
        Ptr_BackgroundSubtractorMOG2 _retval_ = cv::createBackgroundSubtractorMOG2( (int)history, (double)varThreshold, (bool)detectShadows );
        return (jlong)(new Ptr_BackgroundSubtractorMOG2(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorMOG2_11 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorMOG2_11
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "video::createBackgroundSubtractorMOG2_11()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::BackgroundSubtractorMOG2> Ptr_BackgroundSubtractorMOG2;
        Ptr_BackgroundSubtractorMOG2 _retval_ = cv::createBackgroundSubtractorMOG2(  );
        return (jlong)(new Ptr_BackgroundSubtractorMOG2(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Ptr_DualTVL1OpticalFlow createOptFlow_DualTVL1()
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createOptFlow_1DualTVL1_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createOptFlow_1DualTVL1_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "video::createOptFlow_1DualTVL1_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::DualTVL1OpticalFlow> Ptr_DualTVL1OpticalFlow;
        Ptr_DualTVL1OpticalFlow _retval_ = cv::createOptFlow_DualTVL1(  );
        return (jlong)(new Ptr_DualTVL1OpticalFlow(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Ptr_BackgroundSubtractorKNN createBackgroundSubtractorKNN(int history = 500, double dist2Threshold = 400.0, bool detectShadows = true)
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorKNN_10 (JNIEnv*, jclass, jint, jdouble, jboolean);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorKNN_10
  (JNIEnv* env, jclass , jint history, jdouble dist2Threshold, jboolean detectShadows)
{
    static const char method_name[] = "video::createBackgroundSubtractorKNN_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::BackgroundSubtractorKNN> Ptr_BackgroundSubtractorKNN;
        Ptr_BackgroundSubtractorKNN _retval_ = cv::createBackgroundSubtractorKNN( (int)history, (double)dist2Threshold, (bool)detectShadows );
        return (jlong)(new Ptr_BackgroundSubtractorKNN(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorKNN_11 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_video_Video_createBackgroundSubtractorKNN_11
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "video::createBackgroundSubtractorKNN_11()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::BackgroundSubtractorKNN> Ptr_BackgroundSubtractorKNN;
        Ptr_BackgroundSubtractorKNN _retval_ = cv::createBackgroundSubtractorKNN(  );
        return (jlong)(new Ptr_BackgroundSubtractorKNN(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//   KalmanFilter()
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "video::KalmanFilter_10()";
    try {
        LOGD("%s", method_name);
        
        cv::KalmanFilter* _retval_ = new cv::KalmanFilter(  );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//   KalmanFilter(int dynamParams, int measureParams, int controlParams = 0, int type = CV_32F)
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_11 (JNIEnv*, jclass, jint, jint, jint, jint);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_11
  (JNIEnv* env, jclass , jint dynamParams, jint measureParams, jint controlParams, jint type)
{
    static const char method_name[] = "video::KalmanFilter_11()";
    try {
        LOGD("%s", method_name);
        
        cv::KalmanFilter* _retval_ = new cv::KalmanFilter( (int)dynamParams, (int)measureParams, (int)controlParams, (int)type );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_12 (JNIEnv*, jclass, jint, jint);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_KalmanFilter_12
  (JNIEnv* env, jclass , jint dynamParams, jint measureParams)
{
    static const char method_name[] = "video::KalmanFilter_12()";
    try {
        LOGD("%s", method_name);
        
        cv::KalmanFilter* _retval_ = new cv::KalmanFilter( (int)dynamParams, (int)measureParams );
        return (jlong) _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat predict(Mat control = Mat())
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_predict_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_predict_10
  (JNIEnv* env, jclass , jlong self, jlong control_nativeObj)
{
    static const char method_name[] = "video::predict_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& control = *((Mat*)control_nativeObj);
        ::Mat _retval_ = me->predict( control );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_predict_11 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_predict_11
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::predict_11()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->predict(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat correct(Mat measurement)
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_correct_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_correct_10
  (JNIEnv* env, jclass , jlong self, jlong measurement_nativeObj)
{
    static const char method_name[] = "video::correct_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& measurement = *((Mat*)measurement_nativeObj);
        ::Mat _retval_ = me->correct( measurement );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// Mat KalmanFilter::statePre
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1statePre_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1statePre_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1statePre_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->statePre;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::statePre
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1statePre_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1statePre_10
  (JNIEnv* env, jclass , jlong self, jlong statePre_nativeObj)
{
    static const char method_name[] = "video::set_1statePre_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& statePre = *((Mat*)statePre_nativeObj);
        me->statePre = ( statePre );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::statePost
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1statePost_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1statePost_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1statePost_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->statePost;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::statePost
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1statePost_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1statePost_10
  (JNIEnv* env, jclass , jlong self, jlong statePost_nativeObj)
{
    static const char method_name[] = "video::set_1statePost_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& statePost = *((Mat*)statePost_nativeObj);
        me->statePost = ( statePost );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::transitionMatrix
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1transitionMatrix_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1transitionMatrix_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1transitionMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->transitionMatrix;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::transitionMatrix
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1transitionMatrix_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1transitionMatrix_10
  (JNIEnv* env, jclass , jlong self, jlong transitionMatrix_nativeObj)
{
    static const char method_name[] = "video::set_1transitionMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& transitionMatrix = *((Mat*)transitionMatrix_nativeObj);
        me->transitionMatrix = ( transitionMatrix );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::controlMatrix
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1controlMatrix_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1controlMatrix_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1controlMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->controlMatrix;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::controlMatrix
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1controlMatrix_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1controlMatrix_10
  (JNIEnv* env, jclass , jlong self, jlong controlMatrix_nativeObj)
{
    static const char method_name[] = "video::set_1controlMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& controlMatrix = *((Mat*)controlMatrix_nativeObj);
        me->controlMatrix = ( controlMatrix );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::measurementMatrix
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1measurementMatrix_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1measurementMatrix_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1measurementMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->measurementMatrix;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::measurementMatrix
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1measurementMatrix_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1measurementMatrix_10
  (JNIEnv* env, jclass , jlong self, jlong measurementMatrix_nativeObj)
{
    static const char method_name[] = "video::set_1measurementMatrix_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& measurementMatrix = *((Mat*)measurementMatrix_nativeObj);
        me->measurementMatrix = ( measurementMatrix );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::processNoiseCov
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1processNoiseCov_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1processNoiseCov_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1processNoiseCov_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->processNoiseCov;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::processNoiseCov
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1processNoiseCov_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1processNoiseCov_10
  (JNIEnv* env, jclass , jlong self, jlong processNoiseCov_nativeObj)
{
    static const char method_name[] = "video::set_1processNoiseCov_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& processNoiseCov = *((Mat*)processNoiseCov_nativeObj);
        me->processNoiseCov = ( processNoiseCov );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::measurementNoiseCov
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1measurementNoiseCov_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1measurementNoiseCov_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1measurementNoiseCov_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->measurementNoiseCov;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::measurementNoiseCov
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1measurementNoiseCov_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1measurementNoiseCov_10
  (JNIEnv* env, jclass , jlong self, jlong measurementNoiseCov_nativeObj)
{
    static const char method_name[] = "video::set_1measurementNoiseCov_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& measurementNoiseCov = *((Mat*)measurementNoiseCov_nativeObj);
        me->measurementNoiseCov = ( measurementNoiseCov );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::errorCovPre
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1errorCovPre_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1errorCovPre_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1errorCovPre_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->errorCovPre;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::errorCovPre
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1errorCovPre_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1errorCovPre_10
  (JNIEnv* env, jclass , jlong self, jlong errorCovPre_nativeObj)
{
    static const char method_name[] = "video::set_1errorCovPre_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& errorCovPre = *((Mat*)errorCovPre_nativeObj);
        me->errorCovPre = ( errorCovPre );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::gain
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1gain_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1gain_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1gain_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->gain;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::gain
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1gain_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1gain_10
  (JNIEnv* env, jclass , jlong self, jlong gain_nativeObj)
{
    static const char method_name[] = "video::set_1gain_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& gain = *((Mat*)gain_nativeObj);
        me->gain = ( gain );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// Mat KalmanFilter::errorCovPost
//

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1errorCovPost_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_video_KalmanFilter_get_1errorCovPost_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::get_1errorCovPost_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        ::Mat _retval_ = me->errorCovPost;//(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// void KalmanFilter::errorCovPost
//

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1errorCovPost_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_set_1errorCovPost_10
  (JNIEnv* env, jclass , jlong self, jlong errorCovPost_nativeObj)
{
    static const char method_name[] = "video::set_1errorCovPost_10()";
    try {
        LOGD("%s", method_name);
        cv::KalmanFilter* me = (cv::KalmanFilter*) self; //TODO: check for NULL
        Mat& errorCovPost = *((Mat*)errorCovPost_nativeObj);
        me->errorCovPost = ( errorCovPost );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  native support for java finalize()
//  static void cv::KalmanFilter::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_KalmanFilter_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (cv::KalmanFilter*) self;
}


//
//  double getVarThreshold()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarThreshold_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarThreshold_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getVarThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getVarThreshold(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setVarThreshold(double varThreshold)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarThreshold_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarThreshold_10
  (JNIEnv* env, jclass , jlong self, jdouble varThreshold)
{
    static const char method_name[] = "video::setVarThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setVarThreshold( (double)varThreshold );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getVarThresholdGen()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarThresholdGen_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarThresholdGen_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getVarThresholdGen_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getVarThresholdGen(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setVarThresholdGen(double varThresholdGen)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarThresholdGen_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarThresholdGen_10
  (JNIEnv* env, jclass , jlong self, jdouble varThresholdGen)
{
    static const char method_name[] = "video::setVarThresholdGen_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setVarThresholdGen( (double)varThresholdGen );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getVarInit()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarInit_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarInit_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getVarInit_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getVarInit(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setVarInit(double varInit)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarInit_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarInit_10
  (JNIEnv* env, jclass , jlong self, jdouble varInit)
{
    static const char method_name[] = "video::setVarInit_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setVarInit( (double)varInit );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getVarMin()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarMin_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarMin_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getVarMin_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getVarMin(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setVarMin(double varMin)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarMin_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarMin_10
  (JNIEnv* env, jclass , jlong self, jdouble varMin)
{
    static const char method_name[] = "video::setVarMin_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setVarMin( (double)varMin );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getVarMax()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarMax_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getVarMax_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getVarMax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getVarMax(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setVarMax(double varMax)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarMax_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setVarMax_10
  (JNIEnv* env, jclass , jlong self, jdouble varMax)
{
    static const char method_name[] = "video::setVarMax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setVarMax( (double)varMax );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getComplexityReductionThreshold()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getComplexityReductionThreshold_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getComplexityReductionThreshold_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getComplexityReductionThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getComplexityReductionThreshold(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setComplexityReductionThreshold(double ct)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setComplexityReductionThreshold_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setComplexityReductionThreshold_10
  (JNIEnv* env, jclass , jlong self, jdouble ct)
{
    static const char method_name[] = "video::setComplexityReductionThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setComplexityReductionThreshold( (double)ct );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool getDetectShadows()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getDetectShadows_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getDetectShadows_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getDetectShadows_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getDetectShadows(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setDetectShadows(bool detectShadows)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setDetectShadows_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setDetectShadows_10
  (JNIEnv* env, jclass , jlong self, jboolean detectShadows)
{
    static const char method_name[] = "video::setDetectShadows_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setDetectShadows( (bool)detectShadows );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getShadowValue()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getShadowValue_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getShadowValue_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getShadowValue_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getShadowValue(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setShadowValue(int value)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setShadowValue_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setShadowValue_10
  (JNIEnv* env, jclass , jlong self, jint value)
{
    static const char method_name[] = "video::setShadowValue_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setShadowValue( (int)value );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getShadowThreshold()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getShadowThreshold_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getShadowThreshold_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getShadowThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getShadowThreshold(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setShadowThreshold(double threshold)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setShadowThreshold_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setShadowThreshold_10
  (JNIEnv* env, jclass , jlong self, jdouble threshold)
{
    static const char method_name[] = "video::setShadowThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setShadowThreshold( (double)threshold );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getHistory()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getHistory_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getHistory_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getHistory_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getHistory(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setHistory(int history)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setHistory_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setHistory_10
  (JNIEnv* env, jclass , jlong self, jint history)
{
    static const char method_name[] = "video::setHistory_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setHistory( (int)history );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getNMixtures()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getNMixtures_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getNMixtures_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getNMixtures_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getNMixtures(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setNMixtures(int nmixtures)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setNMixtures_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setNMixtures_10
  (JNIEnv* env, jclass , jlong self, jint nmixtures)
{
    static const char method_name[] = "video::setNMixtures_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setNMixtures( (int)nmixtures );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getBackgroundRatio()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getBackgroundRatio_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_getBackgroundRatio_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getBackgroundRatio_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getBackgroundRatio(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setBackgroundRatio(double ratio)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setBackgroundRatio_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_setBackgroundRatio_10
  (JNIEnv* env, jclass , jlong self, jdouble ratio)
{
    static const char method_name[] = "video::setBackgroundRatio_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorMOG2>* me = (Ptr<cv::BackgroundSubtractorMOG2>*) self; //TODO: check for NULL
        (*me)->setBackgroundRatio( (double)ratio );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  native support for java finalize()
//  static void Ptr<cv::BackgroundSubtractorMOG2>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorMOG2_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::BackgroundSubtractorMOG2>*) self;
}


//
//  int getHistory()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getHistory_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getHistory_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getHistory_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getHistory(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setHistory(int history)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setHistory_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setHistory_10
  (JNIEnv* env, jclass , jlong self, jint history)
{
    static const char method_name[] = "video::setHistory_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setHistory( (int)history );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getNSamples()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getNSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getNSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getNSamples_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getNSamples(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setNSamples(int _nN)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setNSamples_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setNSamples_10
  (JNIEnv* env, jclass , jlong self, jint _nN)
{
    static const char method_name[] = "video::setNSamples_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setNSamples( (int)_nN );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getDist2Threshold()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getDist2Threshold_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getDist2Threshold_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getDist2Threshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getDist2Threshold(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setDist2Threshold(double _dist2Threshold)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setDist2Threshold_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setDist2Threshold_10
  (JNIEnv* env, jclass , jlong self, jdouble _dist2Threshold)
{
    static const char method_name[] = "video::setDist2Threshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setDist2Threshold( (double)_dist2Threshold );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getkNNSamples()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getkNNSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getkNNSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getkNNSamples_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getkNNSamples(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setkNNSamples(int _nkNN)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setkNNSamples_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setkNNSamples_10
  (JNIEnv* env, jclass , jlong self, jint _nkNN)
{
    static const char method_name[] = "video::setkNNSamples_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setkNNSamples( (int)_nkNN );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool getDetectShadows()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getDetectShadows_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getDetectShadows_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getDetectShadows_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getDetectShadows(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setDetectShadows(bool detectShadows)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setDetectShadows_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setDetectShadows_10
  (JNIEnv* env, jclass , jlong self, jboolean detectShadows)
{
    static const char method_name[] = "video::setDetectShadows_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setDetectShadows( (bool)detectShadows );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getShadowValue()
//

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getShadowValue_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getShadowValue_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getShadowValue_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getShadowValue(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setShadowValue(int value)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setShadowValue_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setShadowValue_10
  (JNIEnv* env, jclass , jlong self, jint value)
{
    static const char method_name[] = "video::setShadowValue_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setShadowValue( (int)value );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getShadowThreshold()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getShadowThreshold_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_getShadowThreshold_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "video::getShadowThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getShadowThreshold(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setShadowThreshold(double threshold)
//

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setShadowThreshold_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_setShadowThreshold_10
  (JNIEnv* env, jclass , jlong self, jdouble threshold)
{
    static const char method_name[] = "video::setShadowThreshold_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::BackgroundSubtractorKNN>* me = (Ptr<cv::BackgroundSubtractorKNN>*) self; //TODO: check for NULL
        (*me)->setShadowThreshold( (double)threshold );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  native support for java finalize()
//  static void Ptr<cv::BackgroundSubtractorKNN>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_video_BackgroundSubtractorKNN_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::BackgroundSubtractorKNN>*) self;
}



} // extern "C"

#endif // HAVE_OPENCV_VIDEO
