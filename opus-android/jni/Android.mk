LOCAL_PATH := $(call my-dir)
OPUS_DIR            := opus

include $(OPUS_DIR)/Android.mk

include $(CLEAR_VARS)

LOCAL_MODULE        := codec
LOCAL_CFLAGS        := -DNULL=0
#LOCAL_LDLIBS        := -lm -llog
LOCAL_C_INCLUDES    := $(LOCAL_PATH)/$(OPUS_DIR)/include
LOCAL_SHARED_LIBRARIES := opus
include $(BUILD_STATIC_LIBRARY)
