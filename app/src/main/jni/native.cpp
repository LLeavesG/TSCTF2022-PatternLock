#include "jni.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/ptrace.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <android/log.h>
#include "include/inlineHook.h"
#include <elf.h>
#include "include/dlfcn_compat.h"
#include "include/dlfcn_nougat.h"


typedef unsigned char byte;

#define LOG_TAG "Native"
#define JNIREG_CLASS "com/crackme/tsctf/TsUtil"
#define LOGD(fmt,args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
unsigned char fake_key[] = "TSCTF2022!!!!!";

void* *(*oriexecve)(const char *file, char *const *argv, char *const *envp);
byte *strAddr = 0;

struct DexFile {
    // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
    // The class we are a part of.
    uint32_t declaring_class_;
    // Access flags; low 16 bits are defined by spec.
    void *begin;
    /* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */
    // Offset to the CodeItem.
    uint32_t size;
};
struct ArtMethod {
    // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
    // The class we are a part of.
    uint32_t declaring_class_;
    // Access flags; low 16 bits are defined by spec.
    uint32_t access_flags_;
    /* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */
    // Offset to the CodeItem.
    uint32_t dex_code_item_offset_;
    // Index into method_ids of the dex file associated with this method.
    uint32_t dex_method_index_;
};

const char *GetClassLinker_LoadMethod_Sym() {

    switch (get_sdk_level()) {
        case 24:
        case 25:
            return "_ZN3art11ClassLinker10LoadMethodEPNS_6ThreadERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE";
        case 27:
        case 28:
            return "_ZN3art11ClassLinker10LoadMethodERKNS_7DexFileERKNS_21ClassDataItemIteratorENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE";
        case 29:
        case 30:
            return "_ZN3art11ClassLinker10LoadMethodERKNS_7DexFileERKNS_13ClassAccessor6MethodENS_6HandleINS_6mirror5ClassEEEPNS_9ArtMethodE";
        default:
            return "";
    }
}

void *(*oriloadmethod)(void *, void *, void *, void *, void *);

void *myloadmethod(void *a, void *b, void *c, void *d, void *e) {
    struct ArtMethod *artmethod = (struct ArtMethod *) e;
    struct DexFile *dexfile = (struct DexFile *) b;

    void *result = oriloadmethod(a, b, c, d, e);

    byte *code_item_addr = static_cast<byte *>(dexfile->begin) + artmethod->dex_code_item_offset_;

    if (dexfile->size == 0x3A8) {
        mprotect(dexfile->begin, dexfile->size, PROT_WRITE);
        strAddr = (byte *)((int)dexfile->begin + 0x274);
    }

    return result;
}

void* *myexecve(const char *__file, char *const *__argv, char *const *__envp) {
    LOGD("process:%d,enter execve:%s", getpid(), __file);
    if (strstr(__file, "dex2oat")) {
        return NULL;
    } else {
        return oriexecve(__file, __argv, __envp);
    }

}



void hooklibc() {
    LOGD("go into hooklibc");

    void *libc_addr = dlopen("libc.so", RTLD_NOW);

    void *execve_addr = dlsym(libc_addr, "execve");
    if (execve_addr != NULL) {
        if (ELE7EN_OK == registerInlineHook((uint32_t) execve_addr, (uint32_t) myexecve,
                                            (uint32_t **) &oriexecve)) {
            if (ELE7EN_OK == inlineHook((uint32_t) execve_addr)) {
                LOGD("inlineHook execve success");
            } else {
                LOGD("inlineHook execve failure");
            }
        }
    }
}

void hookARTMethod() {
    LOGD("go into hookart");
    void *libart_addr = dlopen_compat("libart.so", RTLD_NOW);
    LOGD("%p",libart_addr);
    if(libart_addr){
        void *loadmethod_addr = dlsym_compat(libart_addr,GetClassLinker_LoadMethod_Sym());
        LOGD("%p",loadmethod_addr);
        if (ELE7EN_OK == registerInlineHook((uint32_t) loadmethod_addr, (uint32_t) myloadmethod,
                                            (uint32_t **) &oriloadmethod)) {
            if (ELE7EN_OK == inlineHook((uint32_t) loadmethod_addr)) {
                LOGD("inlineHook loadmethod success");
            } else {
                LOGD("inlineHook loadmethod failure");
            }
        }
    }
}

static int registerNatives(JNIEnv* env, const char *className, const JNINativeMethod *getMethods,jint numMethods)
{
    jclass clazz = env->FindClass(className);

    if (clazz == NULL) {
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, getMethods, numMethods) < 0) {
        return JNI_FALSE;
    }


    return JNI_TRUE;
}

jboolean JNICALL
trueCheck(JNIEnv *env, jclass clazz, jstring str) {
    int i = 0;
    unsigned char box[] = {0xd,0x3c,0x36,0x12,0x29,0x47,0x5e,0x56,0x66,0x49,0x44,0x6a,0x44,0x58};
    for(i = 0; i < 14; i++){
        strAddr[i] = fake_key[i] ^ box[i];
    }
    jclass clz = env->FindClass("com/crackme/tsctf/TsUtil");
    jmethodID mid = env->GetStaticMethodID(clz,"cmp", "(Ljava/lang/String;)Z");
    int ret = env->CallStaticBooleanMethod(clz,mid,str);

    for(i = 0; i < 14; i++) {
        strAddr[i] = fake_key[i];
    }

    return ret;
}


jint __attribute__ ((visibility ("default")))  JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env;
    int i = 0;
    ptrace(PTRACE_TRACEME,0,0,0);

    if (vm->GetEnv((void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    unsigned char method[6] = {0x63,0x69,0x67,0x60,0x6f,0x00};
    unsigned char sign[22] = {0x28,0x4d,0x68,0x62,0x72,0x64,0x29,0x6b,0x69,0x67,0x6d,0x24,0x5f,0x79,0x7c,0x66,0x7e,0x76,0x29,0x3a,0x4e,0x00};
    for(i=0;i<5;i++){
        method[i] ^= i;
    }
    for(i=0;i<21;i++){
        sign[i] ^= i;
    }

    JNINativeMethod getMethods[] = {{reinterpret_cast<const char *>(method), reinterpret_cast<const char *>(sign), (void*)trueCheck }};
    const char *className = JNIREG_CLASS;

    if (!registerNatives(env,className,getMethods,1)) {
        return -1;
    }
    hooklibc();
    hookARTMethod();

    return JNI_VERSION_1_6;

}

extern "C"
JNIEXPORT jboolean JNICALL
_Java_com_crackme_tsctf_TsUtil_check(JNIEnv *env, jclass clazz, jstring str) {

    jclass clz = env->FindClass("com/crackme/tsctf/TsUtil");
    jmethodID mid = env->GetStaticMethodID(clz,"cmp", "(Ljava/lang/String;)Z");
    int ret = env->CallStaticBooleanMethod(clz,mid,str);
    return ret;
}
