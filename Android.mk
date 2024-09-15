# copy this file to $ANDROID_HOME/SDL2/build/net.eludi.arcajs/app/jni/src
# and create symbolic links of all arcajs sources there

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_CFLAGS := -DNO_CURL # todo
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := window.c graphicsUtils.c console.c audio.c resources.c archive.c \
  value.c httpRequest.c external/miniz.c graphics.c \
  arcajs.c graphicsBindings.c jsBindings.c \
  modules/intersects.c modules/intersectsBindings.c external/duktape.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
