
//
// This file is auto-generated, please don't edit!
//

#define LOG_TAG "org.opencv.ml"

#include "common.h"

#include "opencv2/opencv_modules.hpp"
#ifdef HAVE_OPENCV_ML

#include <string>

#include "opencv2/ml.hpp"

#include "../../ml/include/opencv2/ml.hpp"

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
//  int getClustersNumber()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_EM_getClustersNumber_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_EM_getClustersNumber_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getClustersNumber_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getClustersNumber(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setClustersNumber(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setClustersNumber_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setClustersNumber_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setClustersNumber_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        (*me)->setClustersNumber( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getCovarianceMatrixType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_EM_getCovarianceMatrixType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_EM_getCovarianceMatrixType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCovarianceMatrixType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getCovarianceMatrixType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setCovarianceMatrixType(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setCovarianceMatrixType_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setCovarianceMatrixType_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setCovarianceMatrixType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        (*me)->setCovarianceMatrixType( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  TermCriteria getTermCriteria()
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_EM_getTermCriteria_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_EM_getTermCriteria_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        TermCriteria _retval_ = (*me)->getTermCriteria(  );
        jdoubleArray _da_retval_ = env->NewDoubleArray(3);  jdouble _tmp_retval_[3] = {_retval_.type, _retval_.maxCount, _retval_.epsilon}; env->SetDoubleArrayRegion(_da_retval_, 0, 3, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTermCriteria(TermCriteria val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setTermCriteria_10 (JNIEnv*, jclass, jlong, jint, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_setTermCriteria_10
  (JNIEnv* env, jclass , jlong self, jint val_type, jint val_maxCount, jdouble val_epsilon)
{
    static const char method_name[] = "ml::setTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        TermCriteria val(val_type, val_maxCount, val_epsilon);
        (*me)->setTermCriteria( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getWeights()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_getWeights_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_getWeights_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getWeights_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getWeights(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getMeans()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_getMeans_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_getMeans_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMeans_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getMeans(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Vec2d predict2(Mat sample, Mat& probs)
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_EM_predict2_10 (JNIEnv*, jclass, jlong, jlong, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_EM_predict2_10
  (JNIEnv* env, jclass , jlong self, jlong sample_nativeObj, jlong probs_nativeObj)
{
    static const char method_name[] = "ml::predict2_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& sample = *((Mat*)sample_nativeObj);
        Mat& probs = *((Mat*)probs_nativeObj);
        Vec2d _retval_ = (*me)->predict2( sample, probs );
        jdoubleArray _da_retval_ = env->NewDoubleArray(2);  jdouble _tmp_retval_[2] = {_retval_.val[0], _retval_.val[1]}; env->SetDoubleArrayRegion(_da_retval_, 0, 2, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool trainEM(Mat samples, Mat& logLikelihoods = Mat(), Mat& labels = Mat(), Mat& probs = Mat())
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainEM_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainEM_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong logLikelihoods_nativeObj, jlong labels_nativeObj, jlong probs_nativeObj)
{
    static const char method_name[] = "ml::trainEM_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& logLikelihoods = *((Mat*)logLikelihoods_nativeObj);
        Mat& labels = *((Mat*)labels_nativeObj);
        Mat& probs = *((Mat*)probs_nativeObj);
        bool _retval_ = (*me)->trainEM( samples, logLikelihoods, labels, probs );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainEM_11 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainEM_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj)
{
    static const char method_name[] = "ml::trainEM_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        bool _retval_ = (*me)->trainEM( samples );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool trainE(Mat samples, Mat means0, Mat covs0 = Mat(), Mat weights0 = Mat(), Mat& logLikelihoods = Mat(), Mat& labels = Mat(), Mat& probs = Mat())
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainE_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong, jlong, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainE_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong means0_nativeObj, jlong covs0_nativeObj, jlong weights0_nativeObj, jlong logLikelihoods_nativeObj, jlong labels_nativeObj, jlong probs_nativeObj)
{
    static const char method_name[] = "ml::trainE_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& means0 = *((Mat*)means0_nativeObj);
        Mat& covs0 = *((Mat*)covs0_nativeObj);
        Mat& weights0 = *((Mat*)weights0_nativeObj);
        Mat& logLikelihoods = *((Mat*)logLikelihoods_nativeObj);
        Mat& labels = *((Mat*)labels_nativeObj);
        Mat& probs = *((Mat*)probs_nativeObj);
        bool _retval_ = (*me)->trainE( samples, means0, covs0, weights0, logLikelihoods, labels, probs );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainE_11 (JNIEnv*, jclass, jlong, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainE_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong means0_nativeObj)
{
    static const char method_name[] = "ml::trainE_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& means0 = *((Mat*)means0_nativeObj);
        bool _retval_ = (*me)->trainE( samples, means0 );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool trainM(Mat samples, Mat probs0, Mat& logLikelihoods = Mat(), Mat& labels = Mat(), Mat& probs = Mat())
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainM_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainM_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong probs0_nativeObj, jlong logLikelihoods_nativeObj, jlong labels_nativeObj, jlong probs_nativeObj)
{
    static const char method_name[] = "ml::trainM_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& probs0 = *((Mat*)probs0_nativeObj);
        Mat& logLikelihoods = *((Mat*)logLikelihoods_nativeObj);
        Mat& labels = *((Mat*)labels_nativeObj);
        Mat& probs = *((Mat*)probs_nativeObj);
        bool _retval_ = (*me)->trainM( samples, probs0, logLikelihoods, labels, probs );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainM_11 (JNIEnv*, jclass, jlong, jlong, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_EM_trainM_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong probs0_nativeObj)
{
    static const char method_name[] = "ml::trainM_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::EM>* me = (Ptr<cv::ml::EM>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& probs0 = *((Mat*)probs0_nativeObj);
        bool _retval_ = (*me)->trainM( samples, probs0 );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_EM create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_EM_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::EM> Ptr_EM;
        Ptr_EM _retval_ = cv::ml::EM::create(  );
        return (jlong)(new Ptr_EM(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::EM>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_EM_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_EM_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::EM>*) self;
}


//
//  int getType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_SVM_getType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_SVM_getType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setType(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setType_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setType_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setType( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getGamma()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getGamma_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getGamma_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getGamma_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getGamma(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setGamma(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setGamma_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setGamma_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setGamma_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setGamma( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getCoef0()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getCoef0_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getCoef0_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCoef0_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getCoef0(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setCoef0(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setCoef0_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setCoef0_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setCoef0_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setCoef0( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getDegree()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getDegree_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getDegree_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getDegree_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getDegree(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setDegree(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setDegree_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setDegree_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setDegree_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setDegree( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getC()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getC_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getC_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getC_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getC(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setC(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setC_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setC_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setC_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setC( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getNu()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getNu_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getNu_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNu_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getNu(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setNu(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setNu_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setNu_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setNu_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setNu( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getP()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getP_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getP_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getP_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getP(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setP(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setP_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setP_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setP_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setP( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getClassWeights()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_getClassWeights_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_getClassWeights_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getClassWeights_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getClassWeights(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setClassWeights(Mat val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setClassWeights_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setClassWeights_10
  (JNIEnv* env, jclass , jlong self, jlong val_nativeObj)
{
    static const char method_name[] = "ml::setClassWeights_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        Mat& val = *((Mat*)val_nativeObj);
        (*me)->setClassWeights( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  TermCriteria getTermCriteria()
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_SVM_getTermCriteria_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_SVM_getTermCriteria_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        TermCriteria _retval_ = (*me)->getTermCriteria(  );
        jdoubleArray _da_retval_ = env->NewDoubleArray(3);  jdouble _tmp_retval_[3] = {_retval_.type, _retval_.maxCount, _retval_.epsilon}; env->SetDoubleArrayRegion(_da_retval_, 0, 3, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTermCriteria(TermCriteria val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setTermCriteria_10 (JNIEnv*, jclass, jlong, jint, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setTermCriteria_10
  (JNIEnv* env, jclass , jlong self, jint val_type, jint val_maxCount, jdouble val_epsilon)
{
    static const char method_name[] = "ml::setTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        TermCriteria val(val_type, val_maxCount, val_epsilon);
        (*me)->setTermCriteria( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getKernelType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_SVM_getKernelType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_SVM_getKernelType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getKernelType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getKernelType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setKernel(int kernelType)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setKernel_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_setKernel_10
  (JNIEnv* env, jclass , jlong self, jint kernelType)
{
    static const char method_name[] = "ml::setKernel_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        (*me)->setKernel( (int)kernelType );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getSupportVectors()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_getSupportVectors_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_getSupportVectors_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getSupportVectors_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getSupportVectors(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  double getDecisionFunction(int i, Mat& alpha, Mat& svidx)
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getDecisionFunction_10 (JNIEnv*, jclass, jlong, jint, jlong, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_SVM_getDecisionFunction_10
  (JNIEnv* env, jclass , jlong self, jint i, jlong alpha_nativeObj, jlong svidx_nativeObj)
{
    static const char method_name[] = "ml::getDecisionFunction_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::SVM>* me = (Ptr<cv::ml::SVM>*) self; //TODO: check for NULL
        Mat& alpha = *((Mat*)alpha_nativeObj);
        Mat& svidx = *((Mat*)svidx_nativeObj);
        double _retval_ = (*me)->getDecisionFunction( (int)i, alpha, svidx );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_SVM create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_SVM_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::SVM> Ptr_SVM;
        Ptr_SVM _retval_ = cv::ml::SVM::create(  );
        return (jlong)(new Ptr_SVM(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::SVM>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_SVM_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::SVM>*) self;
}


//
//  float predictProb(Mat inputs, Mat& outputs, Mat& outputProbs, int flags = 0)
//

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_NormalBayesClassifier_predictProb_10 (JNIEnv*, jclass, jlong, jlong, jlong, jlong, jint);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_NormalBayesClassifier_predictProb_10
  (JNIEnv* env, jclass , jlong self, jlong inputs_nativeObj, jlong outputs_nativeObj, jlong outputProbs_nativeObj, jint flags)
{
    static const char method_name[] = "ml::predictProb_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::NormalBayesClassifier>* me = (Ptr<cv::ml::NormalBayesClassifier>*) self; //TODO: check for NULL
        Mat& inputs = *((Mat*)inputs_nativeObj);
        Mat& outputs = *((Mat*)outputs_nativeObj);
        Mat& outputProbs = *((Mat*)outputProbs_nativeObj);
        float _retval_ = (*me)->predictProb( inputs, outputs, outputProbs, (int)flags );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jfloat JNICALL Java_org_opencv_ml_NormalBayesClassifier_predictProb_11 (JNIEnv*, jclass, jlong, jlong, jlong, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_NormalBayesClassifier_predictProb_11
  (JNIEnv* env, jclass , jlong self, jlong inputs_nativeObj, jlong outputs_nativeObj, jlong outputProbs_nativeObj)
{
    static const char method_name[] = "ml::predictProb_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::NormalBayesClassifier>* me = (Ptr<cv::ml::NormalBayesClassifier>*) self; //TODO: check for NULL
        Mat& inputs = *((Mat*)inputs_nativeObj);
        Mat& outputs = *((Mat*)outputs_nativeObj);
        Mat& outputProbs = *((Mat*)outputProbs_nativeObj);
        float _retval_ = (*me)->predictProb( inputs, outputs, outputProbs );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_NormalBayesClassifier create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_NormalBayesClassifier_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_NormalBayesClassifier_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::NormalBayesClassifier> Ptr_NormalBayesClassifier;
        Ptr_NormalBayesClassifier _retval_ = cv::ml::NormalBayesClassifier::create(  );
        return (jlong)(new Ptr_NormalBayesClassifier(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::NormalBayesClassifier>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_NormalBayesClassifier_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_NormalBayesClassifier_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::NormalBayesClassifier>*) self;
}


//
//  int getLayout()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getLayout_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getLayout_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getLayout_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getLayout(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getNTestSamples()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNTestSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNTestSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNTestSamples_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getNTestSamples(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getNTrainSamples()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNTrainSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNTrainSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNTrainSamples_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getNTrainSamples(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getNSamples()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNSamples_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getNSamples(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getNVars()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNVars_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNVars_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNVars_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getNVars(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void getSample(Mat varIdx, int sidx, float* buf)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_getSample_10 (JNIEnv*, jclass, jlong, jlong, jint, jfloat);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_getSample_10
  (JNIEnv* env, jclass , jlong self, jlong varIdx_nativeObj, jint sidx, jfloat buf)
{
    static const char method_name[] = "ml::getSample_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        Mat& varIdx = *((Mat*)varIdx_nativeObj);
        me->getSample( varIdx, (int)sidx, &buf );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getNAllVars()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNAllVars_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getNAllVars_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNAllVars_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getNAllVars(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getMissing()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getMissing_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getMissing_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMissing_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getMissing(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTrainSamples(int layout = ROW_SAMPLE, bool compressSamples = true, bool compressVars = true)
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSamples_10 (JNIEnv*, jclass, jlong, jint, jboolean, jboolean);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSamples_10
  (JNIEnv* env, jclass , jlong self, jint layout, jboolean compressSamples, jboolean compressVars)
{
    static const char method_name[] = "ml::getTrainSamples_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainSamples( (int)layout, (bool)compressSamples, (bool)compressVars );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSamples_11 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSamples_11
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainSamples_11()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainSamples(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTrainResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTrainNormCatResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainNormCatResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainNormCatResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainNormCatResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainNormCatResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTestResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTestResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTestResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTestNormCatResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestNormCatResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestNormCatResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTestNormCatResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTestNormCatResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getSamples()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSamples_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSamples_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getSamples_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getSamples(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getNormCatResponses()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getNormCatResponses_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getNormCatResponses_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getNormCatResponses_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getNormCatResponses(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getSampleWeights()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSampleWeights_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSampleWeights_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getSampleWeights_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getSampleWeights(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTrainSampleWeights()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSampleWeights_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSampleWeights_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainSampleWeights_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainSampleWeights(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTestSampleWeights()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestSampleWeights_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestSampleWeights_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTestSampleWeights_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTestSampleWeights(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getVarIdx()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getVarIdx_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getVarIdx_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getVarIdx_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getVarIdx(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getVarType()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getVarType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getVarType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getVarType_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getVarType(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getResponseType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getResponseType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getResponseType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getResponseType_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getResponseType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTrainSampleIdx()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSampleIdx_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTrainSampleIdx_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainSampleIdx_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTrainSampleIdx(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getTestSampleIdx()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestSampleIdx_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getTestSampleIdx_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTestSampleIdx_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getTestSampleIdx(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void getValues(int vi, Mat sidx, float* values)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_getValues_10 (JNIEnv*, jclass, jlong, jint, jlong, jfloat);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_getValues_10
  (JNIEnv* env, jclass , jlong self, jint vi, jlong sidx_nativeObj, jfloat values)
{
    static const char method_name[] = "ml::getValues_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        Mat& sidx = *((Mat*)sidx_nativeObj);
        me->getValues( (int)vi, sidx, &values );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getDefaultSubstValues()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getDefaultSubstValues_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getDefaultSubstValues_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getDefaultSubstValues_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getDefaultSubstValues(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  int getCatCount(int vi)
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getCatCount_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT jint JNICALL Java_org_opencv_ml_TrainData_getCatCount_10
  (JNIEnv* env, jclass , jlong self, jint vi)
{
    static const char method_name[] = "ml::getCatCount_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        int _retval_ = me->getCatCount( (int)vi );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getClassLabels()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getClassLabels_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getClassLabels_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getClassLabels_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getClassLabels(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getCatOfs()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getCatOfs_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getCatOfs_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCatOfs_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getCatOfs(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat getCatMap()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getCatMap_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getCatMap_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCatMap_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        ::Mat _retval_ = me->getCatMap(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTrainTestSplit(int count, bool shuffle = true)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplit_10 (JNIEnv*, jclass, jlong, jint, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplit_10
  (JNIEnv* env, jclass , jlong self, jint count, jboolean shuffle)
{
    static const char method_name[] = "ml::setTrainTestSplit_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        me->setTrainTestSplit( (int)count, (bool)shuffle );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplit_11 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplit_11
  (JNIEnv* env, jclass , jlong self, jint count)
{
    static const char method_name[] = "ml::setTrainTestSplit_11()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        me->setTrainTestSplit( (int)count );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void setTrainTestSplitRatio(double ratio, bool shuffle = true)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplitRatio_10 (JNIEnv*, jclass, jlong, jdouble, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplitRatio_10
  (JNIEnv* env, jclass , jlong self, jdouble ratio, jboolean shuffle)
{
    static const char method_name[] = "ml::setTrainTestSplitRatio_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        me->setTrainTestSplitRatio( (double)ratio, (bool)shuffle );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplitRatio_11 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_setTrainTestSplitRatio_11
  (JNIEnv* env, jclass , jlong self, jdouble ratio)
{
    static const char method_name[] = "ml::setTrainTestSplitRatio_11()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        me->setTrainTestSplitRatio( (double)ratio );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void shuffleTrainTest()
//

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_shuffleTrainTest_10 (JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_shuffleTrainTest_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::shuffleTrainTest_10()";
    try {
        LOGD("%s", method_name);
        cv::ml::TrainData* me = (cv::ml::TrainData*) self; //TODO: check for NULL
        me->shuffleTrainTest(  );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// static Mat getSubVector(Mat vec, Mat idx)
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSubVector_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_TrainData_getSubVector_10
  (JNIEnv* env, jclass , jlong vec_nativeObj, jlong idx_nativeObj)
{
    static const char method_name[] = "ml::getSubVector_10()";
    try {
        LOGD("%s", method_name);
        Mat& vec = *((Mat*)vec_nativeObj);
        Mat& idx = *((Mat*)idx_nativeObj);
        ::Mat _retval_ = cv::ml::TrainData::getSubVector( vec, idx );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void cv::ml::TrainData::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_TrainData_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (cv::ml::TrainData*) self;
}


//
//  int getBoostType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_Boost_getBoostType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_Boost_getBoostType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getBoostType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getBoostType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setBoostType(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setBoostType_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setBoostType_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setBoostType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        (*me)->setBoostType( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getWeakCount()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_Boost_getWeakCount_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_Boost_getWeakCount_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getWeakCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getWeakCount(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setWeakCount(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setWeakCount_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setWeakCount_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setWeakCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        (*me)->setWeakCount( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getWeightTrimRate()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_Boost_getWeightTrimRate_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_Boost_getWeightTrimRate_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getWeightTrimRate_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getWeightTrimRate(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setWeightTrimRate(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setWeightTrimRate_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_setWeightTrimRate_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setWeightTrimRate_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::Boost>* me = (Ptr<cv::ml::Boost>*) self; //TODO: check for NULL
        (*me)->setWeightTrimRate( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// static Ptr_Boost create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_Boost_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_Boost_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::Boost> Ptr_Boost;
        Ptr_Boost _retval_ = cv::ml::Boost::create(  );
        return (jlong)(new Ptr_Boost(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::Boost>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_Boost_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::Boost>*) self;
}


//
//  double getLearningRate()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_LogisticRegression_getLearningRate_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_LogisticRegression_getLearningRate_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getLearningRate_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getLearningRate(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setLearningRate(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setLearningRate_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setLearningRate_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setLearningRate_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        (*me)->setLearningRate( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getIterations()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getIterations_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getIterations_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getIterations_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getIterations(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setIterations(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setIterations_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setIterations_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setIterations_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        (*me)->setIterations( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getRegularization()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getRegularization_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getRegularization_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRegularization_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getRegularization(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRegularization(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setRegularization_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setRegularization_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setRegularization_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        (*me)->setRegularization( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getTrainMethod()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getTrainMethod_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getTrainMethod_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainMethod_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getTrainMethod(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTrainMethod(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setTrainMethod_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setTrainMethod_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setTrainMethod_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        (*me)->setTrainMethod( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getMiniBatchSize()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getMiniBatchSize_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_LogisticRegression_getMiniBatchSize_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMiniBatchSize_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getMiniBatchSize(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setMiniBatchSize(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setMiniBatchSize_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setMiniBatchSize_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setMiniBatchSize_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        (*me)->setMiniBatchSize( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  TermCriteria getTermCriteria()
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_LogisticRegression_getTermCriteria_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_LogisticRegression_getTermCriteria_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        TermCriteria _retval_ = (*me)->getTermCriteria(  );
        jdoubleArray _da_retval_ = env->NewDoubleArray(3);  jdouble _tmp_retval_[3] = {_retval_.type, _retval_.maxCount, _retval_.epsilon}; env->SetDoubleArrayRegion(_da_retval_, 0, 3, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTermCriteria(TermCriteria val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setTermCriteria_10 (JNIEnv*, jclass, jlong, jint, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_setTermCriteria_10
  (JNIEnv* env, jclass , jlong self, jint val_type, jint val_maxCount, jdouble val_epsilon)
{
    static const char method_name[] = "ml::setTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        TermCriteria val(val_type, val_maxCount, val_epsilon);
        (*me)->setTermCriteria( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  float predict(Mat samples, Mat& results = Mat(), int flags = 0)
//

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_LogisticRegression_predict_10 (JNIEnv*, jclass, jlong, jlong, jlong, jint);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_LogisticRegression_predict_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong results_nativeObj, jint flags)
{
    static const char method_name[] = "ml::predict_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& results = *((Mat*)results_nativeObj);
        float _retval_ = (*me)->predict( samples, results, (int)flags );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jfloat JNICALL Java_org_opencv_ml_LogisticRegression_predict_11 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_LogisticRegression_predict_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj)
{
    static const char method_name[] = "ml::predict_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        float _retval_ = (*me)->predict( samples );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  Mat get_learnt_thetas()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_LogisticRegression_get_1learnt_1thetas_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_LogisticRegression_get_1learnt_1thetas_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::get_1learnt_1thetas_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::LogisticRegression>* me = (Ptr<cv::ml::LogisticRegression>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->get_learnt_thetas(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_LogisticRegression create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_LogisticRegression_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_LogisticRegression_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::LogisticRegression> Ptr_LogisticRegression;
        Ptr_LogisticRegression _retval_ = cv::ml::LogisticRegression::create(  );
        return (jlong)(new Ptr_LogisticRegression(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::LogisticRegression>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_LogisticRegression_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::LogisticRegression>*) self;
}


//
//  void setDefaultK(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setDefaultK_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setDefaultK_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setDefaultK_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        (*me)->setDefaultK( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getDefaultK()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getDefaultK_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getDefaultK_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getDefaultK_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getDefaultK(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool getIsClassifier()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_KNearest_getIsClassifier_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_KNearest_getIsClassifier_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getIsClassifier_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getIsClassifier(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setIsClassifier(bool val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setIsClassifier_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setIsClassifier_10
  (JNIEnv* env, jclass , jlong self, jboolean val)
{
    static const char method_name[] = "ml::setIsClassifier_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        (*me)->setIsClassifier( (bool)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getEmax()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getEmax_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getEmax_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getEmax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getEmax(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setEmax(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setEmax_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setEmax_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setEmax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        (*me)->setEmax( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getAlgorithmType()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getAlgorithmType_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_KNearest_getAlgorithmType_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getAlgorithmType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getAlgorithmType(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setAlgorithmType(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setAlgorithmType_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_setAlgorithmType_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setAlgorithmType_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        (*me)->setAlgorithmType( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  float findNearest(Mat samples, int k, Mat& results, Mat& neighborResponses = Mat(), Mat& dist = Mat())
//

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_KNearest_findNearest_10 (JNIEnv*, jclass, jlong, jlong, jint, jlong, jlong, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_KNearest_findNearest_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jint k, jlong results_nativeObj, jlong neighborResponses_nativeObj, jlong dist_nativeObj)
{
    static const char method_name[] = "ml::findNearest_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& results = *((Mat*)results_nativeObj);
        Mat& neighborResponses = *((Mat*)neighborResponses_nativeObj);
        Mat& dist = *((Mat*)dist_nativeObj);
        float _retval_ = (*me)->findNearest( samples, (int)k, results, neighborResponses, dist );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jfloat JNICALL Java_org_opencv_ml_KNearest_findNearest_11 (JNIEnv*, jclass, jlong, jlong, jint, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_KNearest_findNearest_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jint k, jlong results_nativeObj)
{
    static const char method_name[] = "ml::findNearest_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::KNearest>* me = (Ptr<cv::ml::KNearest>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& results = *((Mat*)results_nativeObj);
        float _retval_ = (*me)->findNearest( samples, (int)k, results );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_KNearest create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_KNearest_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_KNearest_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::KNearest> Ptr_KNearest;
        Ptr_KNearest _retval_ = cv::ml::KNearest::create(  );
        return (jlong)(new Ptr_KNearest(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::KNearest>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_KNearest_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::KNearest>*) self;
}


//
//  int getMaxCategories()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMaxCategories_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMaxCategories_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMaxCategories_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getMaxCategories(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setMaxCategories(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMaxCategories_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMaxCategories_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setMaxCategories_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setMaxCategories( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getMaxDepth()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMaxDepth_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMaxDepth_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMaxDepth_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getMaxDepth(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setMaxDepth(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMaxDepth_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMaxDepth_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setMaxDepth_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setMaxDepth( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getMinSampleCount()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMinSampleCount_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getMinSampleCount_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getMinSampleCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getMinSampleCount(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setMinSampleCount(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMinSampleCount_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setMinSampleCount_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setMinSampleCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setMinSampleCount( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getCVFolds()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getCVFolds_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_DTrees_getCVFolds_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCVFolds_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getCVFolds(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setCVFolds(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setCVFolds_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setCVFolds_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setCVFolds_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setCVFolds( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool getUseSurrogates()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getUseSurrogates_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getUseSurrogates_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getUseSurrogates_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getUseSurrogates(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setUseSurrogates(bool val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setUseSurrogates_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setUseSurrogates_10
  (JNIEnv* env, jclass , jlong self, jboolean val)
{
    static const char method_name[] = "ml::setUseSurrogates_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setUseSurrogates( (bool)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool getUse1SERule()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getUse1SERule_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getUse1SERule_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getUse1SERule_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getUse1SERule(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setUse1SERule(bool val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setUse1SERule_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setUse1SERule_10
  (JNIEnv* env, jclass , jlong self, jboolean val)
{
    static const char method_name[] = "ml::setUse1SERule_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setUse1SERule( (bool)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  bool getTruncatePrunedTree()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getTruncatePrunedTree_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_DTrees_getTruncatePrunedTree_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTruncatePrunedTree_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getTruncatePrunedTree(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTruncatePrunedTree(bool val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setTruncatePrunedTree_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setTruncatePrunedTree_10
  (JNIEnv* env, jclass , jlong self, jboolean val)
{
    static const char method_name[] = "ml::setTruncatePrunedTree_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setTruncatePrunedTree( (bool)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  float getRegressionAccuracy()
//

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_DTrees_getRegressionAccuracy_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_DTrees_getRegressionAccuracy_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRegressionAccuracy_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        float _retval_ = (*me)->getRegressionAccuracy(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRegressionAccuracy(float val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setRegressionAccuracy_10 (JNIEnv*, jclass, jlong, jfloat);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setRegressionAccuracy_10
  (JNIEnv* env, jclass , jlong self, jfloat val)
{
    static const char method_name[] = "ml::setRegressionAccuracy_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        (*me)->setRegressionAccuracy( (float)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getPriors()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_DTrees_getPriors_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_DTrees_getPriors_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getPriors_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getPriors(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setPriors(Mat val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setPriors_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_setPriors_10
  (JNIEnv* env, jclass , jlong self, jlong val_nativeObj)
{
    static const char method_name[] = "ml::setPriors_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::DTrees>* me = (Ptr<cv::ml::DTrees>*) self; //TODO: check for NULL
        Mat& val = *((Mat*)val_nativeObj);
        (*me)->setPriors( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
// static Ptr_DTrees create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_DTrees_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_DTrees_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::DTrees> Ptr_DTrees;
        Ptr_DTrees _retval_ = cv::ml::DTrees::create(  );
        return (jlong)(new Ptr_DTrees(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::DTrees>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_DTrees_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::DTrees>*) self;
}


//
//  void setTrainMethod(int method, double param1 = 0, double param2 = 0)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTrainMethod_10 (JNIEnv*, jclass, jlong, jint, jdouble, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTrainMethod_10
  (JNIEnv* env, jclass , jlong self, jint method, jdouble param1, jdouble param2)
{
    static const char method_name[] = "ml::setTrainMethod_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setTrainMethod( (int)method, (double)param1, (double)param2 );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTrainMethod_11 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTrainMethod_11
  (JNIEnv* env, jclass , jlong self, jint method)
{
    static const char method_name[] = "ml::setTrainMethod_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setTrainMethod( (int)method );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  void setActivationFunction(int type, double param1 = 0, double param2 = 0)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setActivationFunction_10 (JNIEnv*, jclass, jlong, jint, jdouble, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setActivationFunction_10
  (JNIEnv* env, jclass , jlong self, jint type, jdouble param1, jdouble param2)
{
    static const char method_name[] = "ml::setActivationFunction_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setActivationFunction( (int)type, (double)param1, (double)param2 );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setActivationFunction_11 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setActivationFunction_11
  (JNIEnv* env, jclass , jlong self, jint type)
{
    static const char method_name[] = "ml::setActivationFunction_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setActivationFunction( (int)type );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getTrainMethod()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_ANN_1MLP_getTrainMethod_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_ANN_1MLP_getTrainMethod_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTrainMethod_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getTrainMethod(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setLayerSizes(Mat _layer_sizes)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setLayerSizes_10 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setLayerSizes_10
  (JNIEnv* env, jclass , jlong self, jlong _layer_sizes_nativeObj)
{
    static const char method_name[] = "ml::setLayerSizes_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        Mat& _layer_sizes = *((Mat*)_layer_sizes_nativeObj);
        (*me)->setLayerSizes( _layer_sizes );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getLayerSizes()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_getLayerSizes_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_getLayerSizes_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getLayerSizes_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getLayerSizes(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  TermCriteria getTermCriteria()
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_ANN_1MLP_getTermCriteria_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_ANN_1MLP_getTermCriteria_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        TermCriteria _retval_ = (*me)->getTermCriteria(  );
        jdoubleArray _da_retval_ = env->NewDoubleArray(3);  jdouble _tmp_retval_[3] = {_retval_.type, _retval_.maxCount, _retval_.epsilon}; env->SetDoubleArrayRegion(_da_retval_, 0, 3, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTermCriteria(TermCriteria val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTermCriteria_10 (JNIEnv*, jclass, jlong, jint, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setTermCriteria_10
  (JNIEnv* env, jclass , jlong self, jint val_type, jint val_maxCount, jdouble val_epsilon)
{
    static const char method_name[] = "ml::setTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        TermCriteria val(val_type, val_maxCount, val_epsilon);
        (*me)->setTermCriteria( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getBackpropWeightScale()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getBackpropWeightScale_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getBackpropWeightScale_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getBackpropWeightScale_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getBackpropWeightScale(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setBackpropWeightScale(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setBackpropWeightScale_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setBackpropWeightScale_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setBackpropWeightScale_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setBackpropWeightScale( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getBackpropMomentumScale()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getBackpropMomentumScale_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getBackpropMomentumScale_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getBackpropMomentumScale_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getBackpropMomentumScale(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setBackpropMomentumScale(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setBackpropMomentumScale_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setBackpropMomentumScale_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setBackpropMomentumScale_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setBackpropMomentumScale( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getRpropDW0()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDW0_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDW0_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRpropDW0_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getRpropDW0(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRpropDW0(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDW0_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDW0_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setRpropDW0_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setRpropDW0( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getRpropDWPlus()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWPlus_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWPlus_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRpropDWPlus_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getRpropDWPlus(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRpropDWPlus(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWPlus_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWPlus_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setRpropDWPlus_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setRpropDWPlus( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getRpropDWMinus()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMinus_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMinus_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRpropDWMinus_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getRpropDWMinus(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRpropDWMinus(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMinus_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMinus_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setRpropDWMinus_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setRpropDWMinus( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getRpropDWMin()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMin_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMin_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRpropDWMin_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getRpropDWMin(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRpropDWMin(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMin_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMin_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setRpropDWMin_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setRpropDWMin( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  double getRpropDWMax()
//

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMax_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdouble JNICALL Java_org_opencv_ml_ANN_1MLP_getRpropDWMax_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getRpropDWMax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        double _retval_ = (*me)->getRpropDWMax(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setRpropDWMax(double val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMax_10 (JNIEnv*, jclass, jlong, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_setRpropDWMax_10
  (JNIEnv* env, jclass , jlong self, jdouble val)
{
    static const char method_name[] = "ml::setRpropDWMax_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        (*me)->setRpropDWMax( (double)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getWeights(int layerIdx)
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_getWeights_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_getWeights_10
  (JNIEnv* env, jclass , jlong self, jint layerIdx)
{
    static const char method_name[] = "ml::getWeights_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::ANN_MLP>* me = (Ptr<cv::ml::ANN_MLP>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getWeights( (int)layerIdx );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_ANN_MLP create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_ANN_1MLP_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::ANN_MLP> Ptr_ANN_MLP;
        Ptr_ANN_MLP _retval_ = cv::ml::ANN_MLP::create(  );
        return (jlong)(new Ptr_ANN_MLP(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::ANN_MLP>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_ANN_1MLP_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::ANN_MLP>*) self;
}


//
//  int getVarCount()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_StatModel_getVarCount_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_StatModel_getVarCount_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getVarCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getVarCount(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool empty()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_empty_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_empty_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::empty_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->empty(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool isTrained()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_isTrained_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_isTrained_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::isTrained_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->isTrained(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool isClassifier()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_isClassifier_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_isClassifier_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::isClassifier_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->isClassifier(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  bool train(Mat samples, int layout, Mat responses)
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_train_10 (JNIEnv*, jclass, jlong, jlong, jint, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_StatModel_train_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jint layout, jlong responses_nativeObj)
{
    static const char method_name[] = "ml::train_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& responses = *((Mat*)responses_nativeObj);
        bool _retval_ = (*me)->train( samples, (int)layout, responses );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  float predict(Mat samples, Mat& results = Mat(), int flags = 0)
//

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_StatModel_predict_10 (JNIEnv*, jclass, jlong, jlong, jlong, jint);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_StatModel_predict_10
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj, jlong results_nativeObj, jint flags)
{
    static const char method_name[] = "ml::predict_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        Mat& results = *((Mat*)results_nativeObj);
        float _retval_ = (*me)->predict( samples, results, (int)flags );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



JNIEXPORT jfloat JNICALL Java_org_opencv_ml_StatModel_predict_11 (JNIEnv*, jclass, jlong, jlong);

JNIEXPORT jfloat JNICALL Java_org_opencv_ml_StatModel_predict_11
  (JNIEnv* env, jclass , jlong self, jlong samples_nativeObj)
{
    static const char method_name[] = "ml::predict_11()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::StatModel>* me = (Ptr<cv::ml::StatModel>*) self; //TODO: check for NULL
        Mat& samples = *((Mat*)samples_nativeObj);
        float _retval_ = (*me)->predict( samples );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::StatModel>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_StatModel_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_StatModel_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::StatModel>*) self;
}


//
//  bool getCalculateVarImportance()
//

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_RTrees_getCalculateVarImportance_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jboolean JNICALL Java_org_opencv_ml_RTrees_getCalculateVarImportance_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getCalculateVarImportance_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        bool _retval_ = (*me)->getCalculateVarImportance(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setCalculateVarImportance(bool val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setCalculateVarImportance_10 (JNIEnv*, jclass, jlong, jboolean);

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setCalculateVarImportance_10
  (JNIEnv* env, jclass , jlong self, jboolean val)
{
    static const char method_name[] = "ml::setCalculateVarImportance_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        (*me)->setCalculateVarImportance( (bool)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  int getActiveVarCount()
//

JNIEXPORT jint JNICALL Java_org_opencv_ml_RTrees_getActiveVarCount_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jint JNICALL Java_org_opencv_ml_RTrees_getActiveVarCount_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getActiveVarCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        int _retval_ = (*me)->getActiveVarCount(  );
        return _retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setActiveVarCount(int val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setActiveVarCount_10 (JNIEnv*, jclass, jlong, jint);

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setActiveVarCount_10
  (JNIEnv* env, jclass , jlong self, jint val)
{
    static const char method_name[] = "ml::setActiveVarCount_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        (*me)->setActiveVarCount( (int)val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  TermCriteria getTermCriteria()
//

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_RTrees_getTermCriteria_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jdoubleArray JNICALL Java_org_opencv_ml_RTrees_getTermCriteria_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        TermCriteria _retval_ = (*me)->getTermCriteria(  );
        jdoubleArray _da_retval_ = env->NewDoubleArray(3);  jdouble _tmp_retval_[3] = {_retval_.type, _retval_.maxCount, _retval_.epsilon}; env->SetDoubleArrayRegion(_da_retval_, 0, 3, _tmp_retval_);
        return _da_retval_;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  void setTermCriteria(TermCriteria val)
//

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setTermCriteria_10 (JNIEnv*, jclass, jlong, jint, jint, jdouble);

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_setTermCriteria_10
  (JNIEnv* env, jclass , jlong self, jint val_type, jint val_maxCount, jdouble val_epsilon)
{
    static const char method_name[] = "ml::setTermCriteria_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        TermCriteria val(val_type, val_maxCount, val_epsilon);
        (*me)->setTermCriteria( val );
        return;
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return;
}



//
//  Mat getVarImportance()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_RTrees_getVarImportance_10 (JNIEnv*, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_RTrees_getVarImportance_10
  (JNIEnv* env, jclass , jlong self)
{
    static const char method_name[] = "ml::getVarImportance_10()";
    try {
        LOGD("%s", method_name);
        Ptr<cv::ml::RTrees>* me = (Ptr<cv::ml::RTrees>*) self; //TODO: check for NULL
        ::Mat _retval_ = (*me)->getVarImportance(  );
        return (jlong) new ::Mat(_retval_);
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
// static Ptr_RTrees create()
//

JNIEXPORT jlong JNICALL Java_org_opencv_ml_RTrees_create_10 (JNIEnv*, jclass);

JNIEXPORT jlong JNICALL Java_org_opencv_ml_RTrees_create_10
  (JNIEnv* env, jclass )
{
    static const char method_name[] = "ml::create_10()";
    try {
        LOGD("%s", method_name);
        typedef Ptr<cv::ml::RTrees> Ptr_RTrees;
        Ptr_RTrees _retval_ = cv::ml::RTrees::create(  );
        return (jlong)(new Ptr_RTrees(_retval_));
    } catch(const std::exception &e) {
        throwJavaException(env, &e, method_name);
    } catch (...) {
        throwJavaException(env, 0, method_name);
    }
    return 0;
}



//
//  native support for java finalize()
//  static void Ptr<cv::ml::RTrees>::delete( __int64 self )
//
JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_delete(JNIEnv*, jclass, jlong);

JNIEXPORT void JNICALL Java_org_opencv_ml_RTrees_delete
  (JNIEnv*, jclass, jlong self)
{
    delete (Ptr<cv::ml::RTrees>*) self;
}



} // extern "C"

#endif // HAVE_OPENCV_ML
