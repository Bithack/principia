#pragma once

// Put polyfills for stuff we want to use from SDL3. Remove this when we're on SDL3.

#include <SDL.h>
#include <jni.h>

const char *SDL_GetAndroidCachePath(void)
{
    static char *s_AndroidCachePath = NULL;

    if (!s_AndroidCachePath) {
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();

        if (!env || !activity) {
            SDL_Log("Failed to get JNI environment or Activity.");
            return NULL;
        }

        // Get the getCacheDir() method
        jclass activityClass = (*env)->GetObjectClass(env, activity);
        jmethodID getCacheDirMethod = (*env)->GetMethodID(env, activityClass, "getCacheDir", "()Ljava/io/File;");

        if (!getCacheDirMethod) {
            SDL_Log("Failed to get getCacheDir method.");
            return NULL;
        }

        jobject cacheDirFile = (*env)->CallObjectMethod(env, activity, getCacheDirMethod);
        jclass fileClass = (*env)->GetObjectClass(env, cacheDirFile);
        jmethodID getAbsolutePathMethod = (*env)->GetMethodID(env, fileClass, "getAbsolutePath", "()Ljava/lang/String;");

        if (!getAbsolutePathMethod) {
            SDL_Log("Failed to get getAbsolutePath method.");
            return NULL;
        }

        jstring absolutePath = (jstring)(*env)->CallObjectMethod(env, cacheDirFile, getAbsolutePathMethod);
        const char* cachePath = (*env)->GetStringUTFChars(env, absolutePath, NULL);

        char* result = SDL_strdup(cachePath);

        (*env)->ReleaseStringUTFChars(env, absolutePath, cachePath);

        s_AndroidCachePath = result;
    }
    return s_AndroidCachePath;
}