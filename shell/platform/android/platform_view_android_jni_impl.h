// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_IMPL_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_IMPL_H_

#include "flutter/fml/platform/android/jni_weak_ref.h"
#include "flutter/shell/platform/android/jni/platform_view_android_jni.h"
// BD ADD:
#include "flutter/bdflutter/shell/platform/android/android_external_image_loader.h"

namespace flutter {

//------------------------------------------------------------------------------
/// @brief      Concrete implementation of `PlatformViewAndroidJNI` that is
///             compiled with the Android toolchain.
///
class PlatformViewAndroidJNIImpl final : public PlatformViewAndroidJNI {
 public:
  PlatformViewAndroidJNIImpl(fml::jni::JavaObjectWeakGlobalRef java_object);

  ~PlatformViewAndroidJNIImpl() override;

  void FlutterViewHandlePlatformMessage(
      fml::RefPtr<flutter::PlatformMessage> message,
      int responseId) override;

  void FlutterViewHandlePlatformMessageResponse(
      int responseId,
      std::unique_ptr<fml::Mapping> data) override;

  void FlutterViewUpdateSemantics(std::vector<uint8_t> buffer,
                                  std::vector<std::string> strings) override;

  void FlutterViewUpdateCustomAccessibilityActions(
      std::vector<uint8_t> actions_buffer,
      std::vector<std::string> strings) override;

  void FlutterViewOnFirstFrame() override;

  void FlutterViewOnPreEngineRestart() override;

  void SurfaceTextureAttachToGLContext(JavaWeakGlobalRef surface_texture,
                                       int textureId) override;

  void SurfaceTextureUpdateTexImage(JavaWeakGlobalRef surface_texture) override;

  void SurfaceTextureGetTransformMatrix(JavaWeakGlobalRef surface_texture,
                                        SkMatrix& transform) override;

  void SurfaceTextureDetachFromGLContext(
      JavaWeakGlobalRef surface_texture) override;

  void FlutterViewOnDisplayPlatformView(int view_id,
                                        int x,
                                        int y,
                                        int width,
                                        int height,
                                        int viewWidth,
                                        int viewHeight,
                                        MutatorsStack mutators_stack) override;

  void FlutterViewDisplayOverlaySurface(int surface_id,
                                        int x,
                                        int y,
                                        int width,
                                        int height) override;

  void FlutterViewBeginFrame() override;

  void FlutterViewEndFrame() override;

  std::unique_ptr<PlatformViewAndroidJNI::OverlayMetadata>
  FlutterViewCreateOverlaySurface() override;

  void FlutterViewDestroyOverlaySurfaces() override;

  std::unique_ptr<std::vector<std::string>>
  FlutterViewComputePlatformResolvedLocale(
      std::vector<std::string> supported_locales_data) override;
 private:
  // Reference to FlutterJNI object.
  const fml::jni::JavaObjectWeakGlobalRef java_object_;

  FML_DISALLOW_COPY_AND_ASSIGN(PlatformViewAndroidJNIImpl);
};

/**
 * BD ADD: call android to load image
 */
void CallJavaImageLoader(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(sk_sp<SkImage> image)> callback);

void CallJavaImageLoaderForCodec(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(std::unique_ptr<NativeExportCodec> codec)> callback);

void CallJavaImageLoaderGetNextFrame(jobject android_image_loader, ImageLoaderContext loaderContext, const int currentFrame, std::shared_ptr<NativeExportCodec> codec, std::function<void(sk_sp<SkImage> image)> callback);

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_PLATFORM_VIEW_ANDROID_JNI_IMPL_H_
