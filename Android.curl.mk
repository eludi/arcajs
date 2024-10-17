# copy this file to $ANDROID_HOME/SDL2/build/net.eludi.arcajs/app/jni/src
# and create symbolic links of all arcajs sources there.
# also put libcurl headers and lbraries into ../curl as obtainable at https://github.com/gustavogenovese/curl-android-ios/tree/master/prebuilt-with-ssl/android

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
CURL_PATH := ../curl

LOCAL_CFLAGS :=
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CURL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := window.c graphicsUtils.c console.c audio.c resources.c archive.c \
  value.c httpRequest.c external/miniz.c graphics.c log.c \
  arcajs.c graphicsBindings.c jsBindings.c \
  modules/intersects.c modules/intersectsBindings.c external/duktape.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid
LOCAL_LDLIBS += -L$(LOCAL_PATH)/$(CURL_PATH)/$(TARGET_ARCH_ABI) -lcurl -lz

include $(BUILD_SHARED_LIBRARY)
