# Android.mk必须以LOCAL_PATH开头，注释#除外
# 设置工作目录，而my-dir则会返回Android.mk文件所在的目录
LOCAL_PATH := $(call my-dir)

# 借助CLEAR_VARS变量清除除LOCAL_PATH外的所有LOCAL_<name>变量

include $(CLEAR_VARS)
# 设置模块的名称，即编译出来.so文件名
# 注，要和上述步骤中build.gradle中NDK节点设置的名字相同
LOCAL_MODULE := native

LOCAL_SRC_FILES := native.cpp inlineHook.c relocate.c dlfcn_compat.cpp dlfcn_nougat.cpp
LOCAL_CFLAGS := -fms-extensions -D_STLP_USE_NEWALLOC -static-libstdc++ -O0 -fvisibility=hidden

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS += -llog

# 必须在文件结尾定义编译类型，指定生成的静态库或者共享库在运行时依赖的共享库模块列表。
include $(BUILD_SHARED_LIBRARY)
