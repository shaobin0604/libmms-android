LOCAL_PATH := $(call my-dir)

#
# Compile libiconv
# 
include $(CLEAR_VARS)
LOCAL_MODULE := libiconv
LOCAL_SRC_FILES := libiconv/libiconv.so
include $(PREBUILT_SHARED_LIBRARY)

#
# Compile libmms
#
include $(CLEAR_VARS)

LOGLEVELS := ERROR WARN INFO DEBUG VERBOSE
cflags_loglevels := $(foreach ll,$(LOGLEVELS),-DAACD_LOGLEVEL_$(ll))

LOCAL_MODULE    := libmms_app
LOCAL_SRC_FILES := libmms-0.6.2/mms.c \
					libmms-0.6.2/mmsh.c \
					libmms-0.6.2/mmsx.c \
					libmms-0.6.2/uri.c \
					libmms-0.6.2/myio.c 
LOCAL_CFLAGS	:= -DHAVE_CONFIG_H $(cflags_loglevels)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/libmms-0.6.2 $(LOCAL_PATH)/libiconv/include
LOCAL_SHARED_LIBRARIES := libiconv
LOCAL_LDLIBS 	:= -llog -lm
include $(BUILD_SHARED_LIBRARY)
