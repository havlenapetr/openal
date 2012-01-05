LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libopenal
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/OpenAL32/Include \
    $(TOP)/frameworks/base/include/media

LOCAL_SRC_FILES  := \
    OpenAL32/alAuxEffectSlot.c \
    OpenAL32/alBuffer.c        \
    OpenAL32/alDatabuffer.c    \
    OpenAL32/alEffect.c        \
    OpenAL32/alError.c         \
    OpenAL32/alExtension.c     \
    OpenAL32/alFilter.c        \
    OpenAL32/alListener.c      \
    OpenAL32/alSource.c        \
    OpenAL32/alState.c         \
    OpenAL32/alThunk.c         \
    Alc/ALc.c                  \
    Alc/alcConfig.c            \
    Alc/alcEcho.c              \
    Alc/alcModulator.c         \
    Alc/alcReverb.c            \
    Alc/alcRing.c              \
    Alc/alcThread.c            \
    Alc/ALu.c                  \
    Alc/bs2b.c                 \
    Alc/null.c		       \
    Alc/android.cpp

LOCAL_CFLAGS := \
    -DAL_BUILD_LIBRARY \
    -DAL_ALEXT_PROTOTYPES

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libbinder \
    libcutils \
    libmedia

include $(BUILD_SHARED_LIBRARY)

#*************** TEST binary *******************

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := openal-info
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/OpenAL32/Include

LOCAL_SRC_FILES  := \
    examples/openal-info.c

LOCAL_SHARED_LIBRARIES := \
    libopenal

include $(BUILD_EXECUTABLE)