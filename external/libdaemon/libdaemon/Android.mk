LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dexec.c \
	dfork.c \
	dlog.c \
	dnonblock.c \
	dpid.c \
	dsignal.c

LOCAL_C_INCLUDES+= $(LOCAL_PATH)/..

LOCAL_MODULE:=libdaemon

LOCAL_EXPORT_C_INCLUDE_DIRS:= $(LOCAL_PATH)/..

# -std=gnu99 -DHAVE_CONFIG_H -I. -I..  -I..   -g -O2 -pipe
# -Wall -W -Wextra -pedantic -Wformat -Wold-style-definition
# -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations
# -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
# -Wmissing-noreturn -Wshadow -Wendif-labels -Wpointer-arith
# -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Winline
#  -Wstrict-aliasing
# -MT testd.o -MD -MP -MF .deps/testd.Tpo -c -o testd.o testd.c

LOCAL_CFLAGS+= \
	-Wno-unused-parameter \
	-DHAVE_CONFIG_H \
	-DLOCALSTATEDIR=\"/var\"

include $(BUILD_SHARED_LIBRARY)
