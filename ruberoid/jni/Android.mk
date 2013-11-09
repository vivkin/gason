LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := native-activity
LOCAL_SRC_FILES := main.cpp ../../parser-shootout.cpp\
 ../../gason.cpp\
 ../../vjson/block_allocator.cpp ../../vjson/json.cpp\
 ../../stix-json/jsmn/jsmn.c ../../stix-json/JsonParser.cpp

LOCAL_LDLIBS := -llog -landroid
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
