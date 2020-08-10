// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_H_

#include <jni.h>
#include "flutter/fml/macros.h"
#include "flutter/shell/platform/android/platform_view_android.h"

namespace flutter {

void FlutterViewHandlePlatformMessage(JNIEnv* env,
                                      jobject obj,
                                      jstring channel,
                                      jobject message,
                                      jint responseId);

void FlutterViewHandlePlatformMessageResponse(JNIEnv* env,
                                              jobject obj,
                                              jint responseId,
                                              jobject response);

void FlutterViewUpdateSemantics(JNIEnv* env,
                                jobject obj,
                                jobject buffer,
                                jobjectArray strings);

void FlutterViewUpdateCustomAccessibilityActions(JNIEnv* env,
                                                 jobject obj,
                                                 jobject buffer,
                                                 jobjectArray strings);

void FlutterViewOnFirstFrame(JNIEnv* env, jobject obj);

void FlutterViewOnPreEngineRestart(JNIEnv* env, jobject obj);

void SurfaceTextureAttachToGLContext(JNIEnv* env, jobject obj, jint textureId);

void SurfaceTextureUpdateTexImage(JNIEnv* env, jobject obj);

void SurfaceTextureGetTransformMatrix(JNIEnv* env,
                                      jobject obj,
                                      jfloatArray result);

void SurfaceTextureDetachFromGLContext(JNIEnv* env, jobject obj);
/**
 * BD ADD: call android to load image
 */
void CallJavaImageLoader(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(sk_sp<SkImage> image)> callback);

void CallJavaImageLoaderForCodec(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(std::unique_ptr<NativeExportCodec> codec)> callback);

void CallJavaImageLoaderGetNextFrame(jobject android_image_loader, ImageLoaderContext loaderContext, const int currentFrame, std::shared_ptr<NativeExportCodec> codec, std::function<void(sk_sp<SkImage> image)> callback);

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_H_
