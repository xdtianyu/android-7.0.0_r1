LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

NON_ANDROID_SRC := \
	src/java/org/eclipse/jetty/jmx/ConnectorServer.java \
	src/java/org/eclipse/jetty/jmx/MBeanContainer.java \
	src/java/org/eclipse/jetty/jmx/ObjectMBean.java \
	src/java/org/eclipse/jetty/servlet/jmx/ServletMappingMBean.java \
	src/java/org/eclipse/jetty/servlet/jmx/FilterMappingMBean.java \
	src/java/org/eclipse/jetty/servlet/jmx/HolderMBean.java \
	src/java/org/eclipse/jetty/util/log/jmx/LogMBean.java \
	src/java/org/eclipse/jetty/server/jmx/ServerMBean.java \
	src/java/org/eclipse/jetty/server/handler/jmx/AbstractHandlerMBean.java \
	src/java/org/eclipse/jetty/server/handler/jmx/ContextHandlerMBean.java \
	src/java/org/eclipse/jetty/servlet/StatisticsServlet.java \
	src/java/org/eclipse/jetty/server/session/jmx/AbstractSessionManagerMBean.java \
	src/java/org/eclipse/jetty/security/SpnegoUserIdentity.java \
	src/java/org/eclipse/jetty/security/SpnegoUserPrincipal.java \
	src/java/org/eclipse/jetty/security/SpnegoLoginService.java \
	src/java/org/eclipse/jetty/server/session/JDBCSessionIdManager.java \
	src/java/org/eclipse/jetty/util/preventers/AppContextLeakPreventer.java \
	src/java/org/eclipse/jetty/util/preventers/AWTLeakPreventer.java \
	src/java/org/eclipse/jetty/servlet/listener/IntrospectorCleaner.java \
	src/java/org/eclipse/jetty/util/preventers/AppContextLeakPreventer.java \
	src/java/org/eclipse/jetty/util/preventers/AWTLeakPreventer.java \
	src/java/org/eclipse/jetty/server/session/JDBCSessionManager.java

LOCAL_SRC_FILES := $(filter-out $(NON_ANDROID_SRC), \
	$(call all-java-files-under, src))

LOCAL_MODULE := jetty
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

LOCAL_STATIC_JAVA_LIBRARIES := \
	jetty-util \
	servlet-api \
	slf4j-api \
	slf4j-jdk14

include $(BUILD_STATIC_JAVA_LIBRARY)

#############################################################
# Pre-built dependency jars
#############################################################

include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
	servlet-api:lib/javax.servlet-3.0.0.v201112011016.jar \
	slf4j-api:lib/slf4j-api-1.6.1.jar \
	slf4j-jdk14:lib/slf4j-jdk14-1.6.1.jar \

include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE := jetty-util
LOCAL_SRC_FILES := lib/jetty-util-6.1.26.jar
LOCAL_JACK_FLAGS := -D jack.import.jar.debug-info=false
LOCAL_UNINSTALLABLE_MODULE := true

include $(BUILD_PREBUILT)
