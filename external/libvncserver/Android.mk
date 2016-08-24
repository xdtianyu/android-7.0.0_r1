LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libvncserver
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/lib \
                    $(LOCAL_PATH)/libvncserver \
                    $(LOCAL_PATH) \
                    $(LOCAL_PATH)/common \
                    external/jpeg

LOCAL_SRC_FILES := libvncserver/auth.c \
                   libvncserver/cargs.c \
                   libvncserver/corre.c \
                   libvncserver/cursor.c \
                   libvncserver/cutpaste.c \
                   libvncserver/draw.c \
                   libvncserver/font.c \
                   libvncserver/hextile.c \
                   libvncserver/httpd.c \
                   libvncserver/main.c \
                   libvncserver/rfbcrypto_openssl.c \
                   libvncserver/rfbregion.c \
                   libvncserver/rfbserver.c \
                   libvncserver/rfbssl_openssl.c \
                   libvncserver/rre.c \
                   libvncserver/scale.c \
                   libvncserver/selbox.c \
                   libvncserver/sockets.c \
                   libvncserver/stats.c \
                   libvncserver/tight.c \
                   libvncserver/translate.c \
                   libvncserver/ultra.c \
                   libvncserver/websockets.c \
                   libvncserver/zlib.c \
                   libvncserver/zrle.c \
                   libvncserver/zrleoutstream.c \
                   libvncserver/zrlepalettehelper.c \
                   common/d3des.c \
                   common/minilzo.c \
                   common/sha1.c \
                   common/turbojpeg.c \
                   common/vncauth.c \
                   test/bmp.c

LOCAL_STATIC_LIBRARIES := libz libpng libjpeg_static libssl_static
LOCAL_CFLAGS := -D__ANDROID__ -DHAVE_LIBSSL -DWITH_WEBSOCKETS

#LOCAL_SDK_VERSION := 14

include $(BUILD_STATIC_LIBRARY)
