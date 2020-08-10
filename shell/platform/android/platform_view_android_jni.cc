// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/platform_view_android_jni.h"

#include <android/native_window_jni.h>
// BD ADD: START
#include <android/bitmap.h>
#include "flutter/lib/ui/painting/image.h"
// END

#include <utility>

#include "flutter/assets/directory_asset_bundle.h"
#include "flutter/common/settings.h"
#include "flutter/fml/file.h"
#include "flutter/fml/platform/android/jni_util.h"
#include "flutter/fml/platform/android/jni_weak_ref.h"
#include "flutter/fml/platform/android/scoped_java_ref.h"
#include "flutter/fml/size.h"
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/runtime/dart_service_isolate.h"
#include "flutter/shell/common/run_configuration.h"
#include "flutter/shell/platform/android/android_external_texture_gl.h"
#include "flutter/shell/platform/android/android_shell_holder.h"
#include "flutter/shell/platform/android/apk_asset_provider.h"
#include "flutter/shell/platform/android/flutter_main.h"
// BD ADD:
#include "flutter/fml/make_copyable.h"

#define ANDROID_SHELL_HOLDER \
  (reinterpret_cast<AndroidShellHolder*>(shell_holder))

namespace flutter {

namespace {

bool CheckException(JNIEnv* env) {
  if (env->ExceptionCheck() == JNI_FALSE)
    return true;

  jthrowable exception = env->ExceptionOccurred();
  env->ExceptionClear();
  FML_LOG(ERROR) << fml::jni::GetJavaExceptionInfo(env, exception);
  env->DeleteLocalRef(exception);
  return false;
}

}  // anonymous namespace

static fml::jni::ScopedJavaGlobalRef<jclass>* g_flutter_callback_info_class =
    nullptr;

static fml::jni::ScopedJavaGlobalRef<jclass>* g_flutter_jni_class = nullptr;

static fml::jni::ScopedJavaGlobalRef<jclass>* g_surface_texture_class = nullptr;

/**
 * BD ADD: android image loader callback class
 */
static fml::jni::ScopedJavaGlobalRef<jclass>* g_image_loader_callback_class = nullptr;
/**
 * BD ADD: android image loader class
 */
static fml::jni::ScopedJavaGlobalRef<jclass>* g_image_loader_class = nullptr;

// Called By Native

static jmethodID g_flutter_callback_info_constructor = nullptr;
jobject CreateFlutterCallbackInformation(
    JNIEnv* env,
    const std::string& callbackName,
    const std::string& callbackClassName,
    const std::string& callbackLibraryPath) {
  return env->NewObject(g_flutter_callback_info_class->obj(),
                        g_flutter_callback_info_constructor,
                        env->NewStringUTF(callbackName.c_str()),
                        env->NewStringUTF(callbackClassName.c_str()),
                        env->NewStringUTF(callbackLibraryPath.c_str()));
}

static jmethodID g_handle_platform_message_method = nullptr;
void FlutterViewHandlePlatformMessage(JNIEnv* env,
                                      jobject obj,
                                      jstring channel,
                                      jobject message,
                                      jint responseId) {
  env->CallVoidMethod(obj, g_handle_platform_message_method, channel, message,
                      responseId);
  FML_CHECK(CheckException(env));
}

static jmethodID g_handle_platform_message_response_method = nullptr;
void FlutterViewHandlePlatformMessageResponse(JNIEnv* env,
                                              jobject obj,
                                              jint responseId,
                                              jobject response) {
  env->CallVoidMethod(obj, g_handle_platform_message_response_method,
                      responseId, response);
  FML_CHECK(CheckException(env));
}

static jmethodID g_update_semantics_method = nullptr;
void FlutterViewUpdateSemantics(JNIEnv* env,
                                jobject obj,
                                jobject buffer,
                                jobjectArray strings) {
  env->CallVoidMethod(obj, g_update_semantics_method, buffer, strings);
  FML_CHECK(CheckException(env));
}

static jmethodID g_update_custom_accessibility_actions_method = nullptr;
void FlutterViewUpdateCustomAccessibilityActions(JNIEnv* env,
                                                 jobject obj,
                                                 jobject buffer,
                                                 jobjectArray strings) {
  env->CallVoidMethod(obj, g_update_custom_accessibility_actions_method, buffer,
                      strings);
  FML_CHECK(CheckException(env));
}

static jmethodID g_on_first_frame_method = nullptr;
void FlutterViewOnFirstFrame(JNIEnv* env, jobject obj) {
  env->CallVoidMethod(obj, g_on_first_frame_method);
  FML_CHECK(CheckException(env));
}

static jmethodID g_on_engine_restart_method = nullptr;
void FlutterViewOnPreEngineRestart(JNIEnv* env, jobject obj) {
  env->CallVoidMethod(obj, g_on_engine_restart_method);
  FML_CHECK(CheckException(env));
}

static jmethodID g_is_released_method = nullptr;
bool SurfaceTextureIsReleased(JNIEnv* env, jobject obj) {
  if (g_is_released_method == nullptr) {
    return false;
  }
  jboolean is_released = env->CallBooleanMethod(obj, g_is_released_method);
  FML_CHECK(CheckException(env));
  return is_released;
}

static jmethodID g_attach_to_gl_context_method = nullptr;
void SurfaceTextureAttachToGLContext(JNIEnv* env, jobject obj, jint textureId) {
  if (SurfaceTextureIsReleased(env, obj)) {
    return;
  }
  env->CallVoidMethod(obj, g_attach_to_gl_context_method, textureId);
  // 暂时清理异常防止crash
  fml::jni::ClearException(env);
  FML_CHECK(CheckException(env));
}

static jmethodID g_update_tex_image_method = nullptr;
void SurfaceTextureUpdateTexImage(JNIEnv* env, jobject obj) {
  if (SurfaceTextureIsReleased(env, obj)) {
    return;
  }
  env->CallVoidMethod(obj, g_update_tex_image_method);
  // 暂时清理异常防止crash
  fml::jni::ClearException(env);
  FML_CHECK(CheckException(env));
}

static jmethodID g_get_transform_matrix_method = nullptr;
void SurfaceTextureGetTransformMatrix(JNIEnv* env,
                                      jobject obj,
                                      jfloatArray result) {
  env->CallVoidMethod(obj, g_get_transform_matrix_method, result);
  // 暂时清理异常防止crash
  fml::jni::ClearException(env);
  FML_CHECK(CheckException(env));
}

static jmethodID g_detach_from_gl_context_method = nullptr;
void SurfaceTextureDetachFromGLContext(JNIEnv* env, jobject obj) {
  env->CallVoidMethod(obj, g_detach_from_gl_context_method);
  // 暂时清理异常防止crash
  fml::jni::ClearException(env);
  FML_CHECK(CheckException(env));
}

/**
 * BD ADD: add for c++ image load result callback
 */
static jmethodID g_native_callback_constructor = nullptr;
/**
 * BD ADD: android image loader load method
 */
static jmethodID g_image_loader_class_load = nullptr;

static jmethodID g_image_loader_class_load_gif = nullptr;
/**
 * BD ADD: android image loader release method
 */
static jmethodID g_image_loader_class_release = nullptr;
/**
 * BD ADD: lock pixel buffer from android Bitmap
 */
bool ObtainPixelsFromJavaBitmap(JNIEnv* env, jobject jbitmap, uint32_t* width, uint32_t* height, int32_t* format, uint32_t* stride, void** pixels) {
    FML_CHECK(CheckException(env));
    AndroidBitmapInfo info;
    if (ANDROID_BITMAP_RESULT_SUCCESS != AndroidBitmap_getInfo(env, jbitmap, &info)) {
        FML_LOG(ERROR)<<"ObtainPixelsFromJavaBitmap: get bitmap info failed"<<std::endl;
        return false;
    }

    if(info.format == ANDROID_BITMAP_FORMAT_NONE){
        FML_LOG(ERROR)<<"ObtainPixelsFromJavaBitmap: format not support"<<std::endl;
        return false;
    }

    *width = info.width;
    *height = info.height;
    *stride = info.stride;
    *format = info.format;
    if (ANDROID_BITMAP_RESULT_SUCCESS != AndroidBitmap_lockPixels(env, jbitmap, pixels) || *pixels == nullptr) {
        FML_LOG(ERROR)<<"ObtainPixelsFromJavaBitmap: lock dst bitmap failed"<<std::endl;
        return false;
    }
    return true;
}
/**
 * BD ADD: release image load context
 */
void ReleaseLoadContext(const void* pixels, SkImage::ReleaseContext releaseContext);

// BD ADD: START
class AndroidImageLoadContext {
public:

    ImageLoaderContext loaderContext;

    AndroidImageLoadContext(std::function<void(sk_sp<SkImage> image)> _callback, ImageLoaderContext _loaderContext, jobject _imageLoader):
    loaderContext(_loaderContext),
    androidImageLoader(_imageLoader),
    callback(std::move(_callback)){}

    AndroidImageLoadContext(std::function<void(std::unique_ptr<NativeExportCodec> codec)> _callback, ImageLoaderContext _loaderContext, jobject _imageLoader):
    loaderContext(_loaderContext),
    androidImageLoader(_imageLoader),
    codecCallback(std::move(_callback)){}

    ~AndroidImageLoadContext(){
    }

    void onLoadForCodecSuccess(JNIEnv *env, std::string cKey, jobject jCodec) {
      if (!loaderContext.task_runners.IsValid()) {
        return;
      }
      auto loaderContentRef = loaderContext;
      loaderContext.task_runners.GetUITaskRunner()->PostTask([loaderContentRef, jCodec, cKey,
                                                               androidImageLoader = androidImageLoader,
                                                               codecCallback = std::move(codecCallback)] {
        JNIEnv *env = fml::jni::AttachCurrentThread();
        jclass clazz = env->GetObjectClass(jCodec);
        jint width = env->GetIntField(jCodec, env->GetFieldID(clazz, "width", "I"));
        jint height = env->GetIntField(jCodec, env->GetFieldID(clazz, "height", "I"));
        jint frameCount = env->GetIntField(jCodec, env->GetFieldID(clazz, "frameCount", "I"));
        jint repeatCount = env->GetIntField(jCodec, env->GetFieldID(clazz, "repeatCount", "I"));
        jobject jObject = env->GetObjectField(jCodec, env->GetFieldID(clazz, "codec", "Ljava/lang/Object;"));
        int* frameDurations = nullptr;
        jobject frameDurationsObj = env->GetObjectField(jCodec, env->GetFieldID(clazz, "frameDurations", "[I"));
        if (frameDurationsObj) {
          jintArray *frameDurationsJni = reinterpret_cast<jintArray*>(&frameDurationsObj);
          frameDurations = env->GetIntArrayElements(*frameDurationsJni, NULL);
        }

        NativeExportCodec *codec = new NativeExportCodec();
        codec->frameDurations = frameDurations;
        codec->repetitionCount_ = repeatCount;
        codec->frameCount_ = frameCount;
        codec->width = width;
        codec->height = height;
        codec->key = new std::string(cKey);
        auto context = loaderContentRef.resourceContext;
        auto task_runners = loaderContentRef.task_runners;
        if (codec->frameCount_ == -1) {
          jObject = env->NewGlobalRef(jObject);
          loaderContentRef.task_runners.GetIOTaskRunner()->PostTask(
            [cKey, task_runners, context,
              codecCallback = std::move(codecCallback),
               codec, jObject, jCodec, androidImageLoader]() {
              JNIEnv *env = fml::jni::AttachCurrentThread();
              codec->skImage = uploadTexture(env, jObject, context);
              unique_ptr<NativeExportCodec> codec2 (codec);
              codecCallback(std::move(codec2));
              task_runners.GetPlatformTaskRunner()->PostTask([jCodec] {
                JNIEnv *env = fml::jni::AttachCurrentThread();
                  env->DeleteGlobalRef(jCodec);
              });
              task_runners.GetUITaskRunner()->PostTask([jObject, androidImageLoader, cKey, task_runners] {
                JNIEnv *env = fml::jni::AttachCurrentThread();
                env->DeleteGlobalRef(jObject);
                env->CallVoidMethod(androidImageLoader, g_image_loader_class_release,
                                    fml::jni::StringToJavaString(env, cKey).obj());
                task_runners.GetIOTaskRunner()->PostTask([androidImageLoader = androidImageLoader] {
                  JNIEnv *env = fml::jni::AttachCurrentThread();
                  env->DeleteGlobalRef(androidImageLoader);
                });
              });
            });
        } else {
          codec->codec = env->NewGlobalRef(jObject);
          unique_ptr<NativeExportCodec> codec2 (codec);
          codecCallback(std::move(codec2));
          task_runners.GetPlatformTaskRunner()->PostTask([cKey, task_runners, jCodec, androidImageLoader] {
            JNIEnv *env = fml::jni::AttachCurrentThread();
            env->DeleteGlobalRef(jCodec);
            task_runners.GetUITaskRunner()->PostTask([task_runners, cKey, androidImageLoader] {
              JNIEnv *env = fml::jni::AttachCurrentThread();
              env->CallVoidMethod(androidImageLoader, g_image_loader_class_release,
                                  fml::jni::StringToJavaString(env, cKey).obj());
              task_runners.GetIOTaskRunner()->PostTask([androidImageLoader = androidImageLoader] {
                JNIEnv *env = fml::jni::AttachCurrentThread();
                env->DeleteGlobalRef(androidImageLoader);
              });
            });
          });
        }
      });
    }

    void onGetNextFrameSuccess(JNIEnv *env, std::string cKey, jobject jbitmap) {
      if (!loaderContext.task_runners.IsValid()) {
        return;
      }
      loaderContext.task_runners.GetIOTaskRunner()->PostTask(
        [this, cKey, jbitmap, androidImageLoader = androidImageLoader,
          callback = std::move(callback)]() {
          JNIEnv *env = fml::jni::AttachCurrentThread();
          env->CallVoidMethod(androidImageLoader, g_image_loader_class_release,
                              fml::jni::StringToJavaString(env, cKey).obj());
          env->DeleteGlobalRef(androidImageLoader);
          auto context = loaderContext.resourceContext;
          callback(uploadTexture(env, jbitmap, context));
          loaderContext.task_runners.GetPlatformTaskRunner()->PostTask([jbitmap] {
            JNIEnv *env = fml::jni::AttachCurrentThread();
            if (env->GetObjectRefType(jbitmap) == 2) {
              env->DeleteGlobalRef(jbitmap);
            }
          });
        });
    }

    void onLoadSuccess(JNIEnv *env, std::string cKey, jobject jbitmap) {
      if (!loaderContext.task_runners.IsValid()) {
        return;
      }
      loaderContext.task_runners.GetIOTaskRunner()->PostTask(
          [this, cKey, jbitmap, androidImageLoader = androidImageLoader,
           callback = std::move(callback)]() {
            JNIEnv* env = fml::jni::AttachCurrentThread();
            env->CallVoidMethod(androidImageLoader,
                                g_image_loader_class_release,
                                fml::jni::StringToJavaString(env, cKey).obj());
            env->DeleteGlobalRef(androidImageLoader);
            auto context = loaderContext.resourceContext;
            callback(uploadTexture(env, jbitmap, context));
            loaderContext.task_runners.GetPlatformTaskRunner()->PostTask(
                [jbitmap] {
                  JNIEnv* env = fml::jni::AttachCurrentThread();
                  if (env->GetObjectRefType(jbitmap) == 2) {
                    env->DeleteGlobalRef(jbitmap);
                  }
                });
          });
    }

    static sk_sp<SkImage> uploadTexture(JNIEnv *env, jobject jbitmap, fml::WeakPtr<GrContext> context) {
      if (jbitmap == nullptr) {
        return nullptr;
      }
      void *pixels = nullptr;
      uint32_t width = 0;
      uint32_t height = 0;
      int32_t format = 0;
      uint32_t stride;
      if (!ObtainPixelsFromJavaBitmap(env, jbitmap, &width, &height, &format, &stride, &pixels)) {
        return nullptr;
      }
      sk_sp<SkImage> skImage;
      SkColorType ct;
      // if android
      switch (format) {
        case AndroidBitmapFormat::ANDROID_BITMAP_FORMAT_NONE:
          ct = kUnknown_SkColorType;
          break;
        case AndroidBitmapFormat::ANDROID_BITMAP_FORMAT_RGBA_8888:
          ct = kRGBA_8888_SkColorType;
          break;
        case AndroidBitmapFormat::ANDROID_BITMAP_FORMAT_RGB_565:
          ct = kRGB_565_SkColorType;
          break;
        case AndroidBitmapFormat::ANDROID_BITMAP_FORMAT_RGBA_4444:
          ct = kARGB_4444_SkColorType;
          break;
        case AndroidBitmapFormat::ANDROID_BITMAP_FORMAT_A_8:
          ct = kAlpha_8_SkColorType;
          break;
      }
      SkImageInfo sk_info =
              SkImageInfo::Make(width, height, ct, kPremul_SkAlphaType);
      size_t row_bytes = stride;
      if (row_bytes < sk_info.minRowBytes()) {
        return nullptr;
      }

      sk_sp<SkData> buffer = SkData::MakeWithProc(pixels, row_bytes * height, ReleaseLoadContext,
                                                  nullptr);
      SkPixmap pixelMap(sk_info, buffer->data(), row_bytes);
      skImage = SkImage::MakeCrossContextFromPixmap(context.get(), pixelMap, false, true);
      auto res = AndroidBitmap_unlockPixels(env, jbitmap);
//      jclass clazz = env->GetObjectClass(jbitmap);
//      env->CallVoidMethod(jbitmap, env->GetMethodID(clazz, "recycle", "()V"));
      if (ANDROID_BITMAP_RESULT_SUCCESS != res) {
        FML_LOG(ERROR)
        << "FlutterViewHandleBitmapPixels: unlock dst bitmap failed code is " + std::to_string(res)
        << std::endl;
      }
      return skImage;
    }

    void onLoadFail(JNIEnv* env, std::string cKey) {
        callback(nullptr);
    }

    void onCodecLoadFail(JNIEnv* env, std::string cKey) {
      if (!loaderContext.task_runners.IsValid()) {
        return;
      }
      loaderContext.task_runners.GetIOTaskRunner()->PostTask([callback = std::move(codecCallback),
                                                               androidImageLoader = androidImageLoader](){
        fml::jni::AttachCurrentThread()->DeleteGlobalRef(androidImageLoader);
        callback(nullptr);
      });
    }

    void onGetNextFrameFail(JNIEnv* env, std::string cKey) {
      if (!loaderContext.task_runners.IsValid()) {
        return;
      }
      loaderContext.task_runners.GetIOTaskRunner()->PostTask([callback = std::move(callback),
                                                                androidImageLoader = androidImageLoader](){
        fml::jni::AttachCurrentThread()->DeleteGlobalRef(androidImageLoader);
        callback(nullptr);
      });
    }
private:
    jobject androidImageLoader;
    std::function<void(sk_sp<SkImage> image)> callback;
    std::function<void(std::unique_ptr<NativeExportCodec> codec)> codecCallback;
};
// END

/**
 * BD ADD: global map for image load context
 */
static std::map<std::string, std::shared_ptr<AndroidImageLoadContext>> g_image_load_contexts;
static std::recursive_mutex g_mutex;
/**
 * BD ADD: call android to load image
 */
void CallJavaImageLoader(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(sk_sp<SkImage> image)> callback) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  auto androidLoadContext = std::make_shared<AndroidImageLoadContext>(callback, loaderContext, env->NewGlobalRef(android_image_loader));
  auto key = url + std::to_string(reinterpret_cast<jlong>(androidLoadContext.get()));
  g_mutex.lock();
  g_image_load_contexts[key] = androidLoadContext;
  g_mutex.unlock();
  auto callObject = env->NewObject(g_image_loader_callback_class->obj(), g_native_callback_constructor);
  auto nativeCallback = new fml::jni::ScopedJavaLocalRef<jobject>(env, callObject);
  env->CallVoidMethod(android_image_loader, g_image_loader_class_load, fml::jni::StringToJavaString(env, url).obj(),
                      reinterpret_cast<jint>(width), reinterpret_cast<jint>(height), scale, nativeCallback->obj(), fml::jni::StringToJavaString(env, key).obj());
  env->DeleteLocalRef(callObject);
}

void CallJavaImageLoaderForCodec(jobject android_image_loader, const std::string url, const int width, const int height, const float scale, ImageLoaderContext loaderContext, std::function<void(std::unique_ptr<NativeExportCodec> codec)> callback) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  auto loadContext = std::make_shared<AndroidImageLoadContext>(callback, loaderContext, env->NewGlobalRef(android_image_loader));
  auto key = url + std::to_string(reinterpret_cast<jlong>(loadContext.get()));
  g_image_load_contexts[key] = loadContext;
  auto callObject = env->NewObject(g_image_loader_callback_class->obj(), g_native_callback_constructor);
  auto nativeCallback = new fml::jni::ScopedJavaLocalRef<jobject>(env, callObject);
  env->CallVoidMethod(android_image_loader, g_image_loader_class_load, fml::jni::StringToJavaString(env, url).obj(),
                      reinterpret_cast<jint>(width), reinterpret_cast<jint>(height), scale, nativeCallback->obj(), fml::jni::StringToJavaString(env, key).obj());
  env->DeleteLocalRef(callObject);
}


void CallJavaImageLoaderGetNextFrame(jobject android_image_loader, ImageLoaderContext loaderContext, int currentFrame, std::shared_ptr<NativeExportCodec> codec, std::function<void(sk_sp<SkImage> image)> callback) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  auto loadContext = std::make_shared<AndroidImageLoadContext>(callback, loaderContext, env->NewGlobalRef(android_image_loader));
  auto key = *codec->key + std::to_string(reinterpret_cast<jlong>(loadContext.get())) + std::to_string(currentFrame);
  g_image_load_contexts[key] = loadContext;
  auto callObject = env->NewObject(g_image_loader_callback_class->obj(), g_native_callback_constructor);
  auto nativeCallback = new fml::jni::ScopedJavaLocalRef<jobject>(env, callObject);
  env->CallVoidMethod(android_image_loader, g_image_loader_class_load_gif, reinterpret_cast<jint>(currentFrame), codec->codec, nativeCallback->obj(), fml::jni::StringToJavaString(env, key).obj());
  env->DeleteLocalRef(callObject);
}

/**
 * BD ADD: called by skia to release pixel resource
 */
void ReleaseLoadContext(const void* pixels, SkImage::ReleaseContext releaseContext){
}

// Called By Java

static jlong AttachJNI(JNIEnv* env,
                       jclass clazz,
                       jobject flutterJNI,
                       jboolean is_background_view) {
  fml::jni::JavaObjectWeakGlobalRef java_object(env, flutterJNI);
  auto shell_holder = std::make_unique<AndroidShellHolder>(
      FlutterMain::Get().GetSettings(), java_object, is_background_view);
  if (shell_holder->IsValid()) {
    return reinterpret_cast<jlong>(shell_holder.release());
  } else {
    return 0;
  }
}

// BD ADD:
static void ScheduleBackgroundFrame(JNIEnv *env, jobject jcaller, jlong shell_holder) {
  ANDROID_SHELL_HOLDER->ScheduleBackgroundFrame();
}
// END

static void DestroyJNI(JNIEnv *env, jobject jcaller, jlong shell_holder) {
    // BD MOD
    // delete ANDROID_SHELL_HOLDER;
    ANDROID_SHELL_HOLDER->ExitApp([holder = ANDROID_SHELL_HOLDER]() { delete holder; });
}

/**
 * BD ADD
 */
static void NotifyLowMemory(JNIEnv *env, jobject jcaller, jlong shell_holder) {
    ANDROID_SHELL_HOLDER->NotifyLowMemory();
}

static void SurfaceCreated(JNIEnv* env,
                           jobject jcaller,
                           jlong shell_holder,
                           jobject jsurface) {
  // Note: This frame ensures that any local references used by
  // ANativeWindow_fromSurface are released immediately. This is needed as a
  // workaround for https://code.google.com/p/android/issues/detail?id=68174
  fml::jni::ScopedJavaLocalFrame scoped_local_reference_frame(env);
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(
      ANativeWindow_fromSurface(env, jsurface));
  ANDROID_SHELL_HOLDER->GetPlatformView()->NotifyCreated(std::move(window));
}

static void SurfaceChanged(JNIEnv* env,
                           jobject jcaller,
                           jlong shell_holder,
                           jint width,
                           jint height) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->NotifyChanged(
      SkISize::Make(width, height));
}

static void SurfaceDestroyed(JNIEnv* env, jobject jcaller, jlong shell_holder) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->NotifyDestroyed();
}

static void RunBundleAndSnapshotFromLibrary(JNIEnv* env,
                                            jobject jcaller,
                                            jlong shell_holder,
                                            jstring jBundlePath,
                                            jstring jEntrypoint,
                                            jstring jLibraryUrl,
                                            jobject jAssetManager) {
  auto asset_manager = std::make_shared<flutter::AssetManager>();

  asset_manager->PushBack(std::make_unique<flutter::APKAssetProvider>(
      env,                                             // jni environment
      jAssetManager,                                   // asset manager
      fml::jni::JavaStringToString(env, jBundlePath))  // apk asset dir
  );

  std::unique_ptr<IsolateConfiguration> isolate_configuration;
  if (flutter::DartVM::IsRunningPrecompiledCode()) {
    isolate_configuration = IsolateConfiguration::CreateForAppSnapshot();
  } else {
    std::unique_ptr<fml::Mapping> kernel_blob =
        fml::FileMapping::CreateReadOnly(
            ANDROID_SHELL_HOLDER->GetSettings().application_kernel_asset);
    if (!kernel_blob) {
      FML_DLOG(ERROR) << "Unable to load the kernel blob asset.";
      return;
    }
    isolate_configuration =
        IsolateConfiguration::CreateForKernel(std::move(kernel_blob));
  }

  RunConfiguration config(std::move(isolate_configuration),
                          std::move(asset_manager));

  {
    auto entrypoint = fml::jni::JavaStringToString(env, jEntrypoint);
    auto libraryUrl = fml::jni::JavaStringToString(env, jLibraryUrl);

    if ((entrypoint.size() > 0) && (libraryUrl.size() > 0)) {
      config.SetEntrypointAndLibrary(std::move(entrypoint),
                                     std::move(libraryUrl));
    } else if (entrypoint.size() > 0) {
      config.SetEntrypoint(std::move(entrypoint));
    }
  }

  ANDROID_SHELL_HOLDER->Launch(std::move(config));
}

static jobject LookupCallbackInformation(JNIEnv* env,
                                         /* unused */ jobject,
                                         jlong handle) {
  auto cbInfo = flutter::DartCallbackCache::GetCallbackInformation(handle);
  if (cbInfo == nullptr) {
    return nullptr;
  }
  return CreateFlutterCallbackInformation(env, cbInfo->name, cbInfo->class_name,
                                          cbInfo->library_path);
}

static void SetViewportMetrics(JNIEnv* env,
                               jobject jcaller,
                               jlong shell_holder,
                               jfloat devicePixelRatio,
                               jint physicalWidth,
                               jint physicalHeight,
                               jint physicalPaddingTop,
                               jint physicalPaddingRight,
                               jint physicalPaddingBottom,
                               jint physicalPaddingLeft,
                               jint physicalViewInsetTop,
                               jint physicalViewInsetRight,
                               jint physicalViewInsetBottom,
                               jint physicalViewInsetLeft,
                               jint systemGestureInsetTop,
                               jint systemGestureInsetRight,
                               jint systemGestureInsetBottom,
                               jint systemGestureInsetLeft) {
  const flutter::ViewportMetrics metrics{
      static_cast<double>(devicePixelRatio),
      static_cast<double>(physicalWidth),
      static_cast<double>(physicalHeight),
      static_cast<double>(physicalPaddingTop),
      static_cast<double>(physicalPaddingRight),
      static_cast<double>(physicalPaddingBottom),
      static_cast<double>(physicalPaddingLeft),
      static_cast<double>(physicalViewInsetTop),
      static_cast<double>(physicalViewInsetRight),
      static_cast<double>(physicalViewInsetBottom),
      static_cast<double>(physicalViewInsetLeft),
      static_cast<double>(systemGestureInsetTop),
      static_cast<double>(systemGestureInsetRight),
      static_cast<double>(systemGestureInsetBottom),
      static_cast<double>(systemGestureInsetLeft),
  };

  ANDROID_SHELL_HOLDER->GetPlatformView()->SetViewportMetrics(metrics);
}

static jobject GetBitmap(JNIEnv* env, jobject jcaller, jlong shell_holder) {
  auto screenshot = ANDROID_SHELL_HOLDER->Screenshot(
      Rasterizer::ScreenshotType::UncompressedImage, false);
  if (screenshot.data == nullptr) {
    return nullptr;
  }

  const SkISize& frame_size = screenshot.frame_size;
  jsize pixels_size = frame_size.width() * frame_size.height();
  jintArray pixels_array = env->NewIntArray(pixels_size);
  if (pixels_array == nullptr) {
    return nullptr;
  }

  jint* pixels = env->GetIntArrayElements(pixels_array, nullptr);
  if (pixels == nullptr) {
    return nullptr;
  }

  auto* pixels_src = static_cast<const int32_t*>(screenshot.data->data());

  // Our configuration of Skia does not support rendering to the
  // BitmapConfig.ARGB_8888 format expected by android.graphics.Bitmap.
  // Convert from kRGBA_8888 to kBGRA_8888 (equivalent to ARGB_8888).
  for (int i = 0; i < pixels_size; i++) {
    int32_t src_pixel = pixels_src[i];
    uint8_t* src_bytes = reinterpret_cast<uint8_t*>(&src_pixel);
    std::swap(src_bytes[0], src_bytes[2]);
    pixels[i] = src_pixel;
  }

  env->ReleaseIntArrayElements(pixels_array, pixels, 0);

  jclass bitmap_class = env->FindClass("android/graphics/Bitmap");
  if (bitmap_class == nullptr) {
    return nullptr;
  }

  jmethodID create_bitmap = env->GetStaticMethodID(
      bitmap_class, "createBitmap",
      "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
  if (create_bitmap == nullptr) {
    return nullptr;
  }

  jclass bitmap_config_class = env->FindClass("android/graphics/Bitmap$Config");
  if (bitmap_config_class == nullptr) {
    return nullptr;
  }

  jmethodID bitmap_config_value_of = env->GetStaticMethodID(
      bitmap_config_class, "valueOf",
      "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
  if (bitmap_config_value_of == nullptr) {
    return nullptr;
  }

  jstring argb = env->NewStringUTF("RGB_565");
  if (argb == nullptr) {
    return nullptr;
  }

  jobject bitmap_config = env->CallStaticObjectMethod(
      bitmap_config_class, bitmap_config_value_of, argb);
  if (bitmap_config == nullptr) {
    return nullptr;
  }

  return env->CallStaticObjectMethod(bitmap_class, create_bitmap, pixels_array,
                                     frame_size.width(), frame_size.height(),
                                     bitmap_config);
}

static void DispatchPlatformMessage(JNIEnv* env,
                                    jobject jcaller,
                                    jlong shell_holder,
                                    jstring channel,
                                    jobject message,
                                    jint position,
                                    jint responseId) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->DispatchPlatformMessage(
      env,                                         //
      fml::jni::JavaStringToString(env, channel),  //
      message,                                     //
      position,                                    //
      responseId                                   //
  );
}

static void DispatchEmptyPlatformMessage(JNIEnv* env,
                                         jobject jcaller,
                                         jlong shell_holder,
                                         jstring channel,
                                         jint responseId) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->DispatchEmptyPlatformMessage(
      env,                                         //
      fml::jni::JavaStringToString(env, channel),  //
      responseId                                   //
  );
}

static void DispatchPointerDataPacket(JNIEnv* env,
                                      jobject jcaller,
                                      jlong shell_holder,
                                      jobject buffer,
                                      jint position) {
  uint8_t* data = static_cast<uint8_t*>(env->GetDirectBufferAddress(buffer));
  auto packet = std::make_unique<flutter::PointerDataPacket>(data, position);
  ANDROID_SHELL_HOLDER->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));
}

static void DispatchSemanticsAction(JNIEnv* env,
                                    jobject jcaller,
                                    jlong shell_holder,
                                    jint id,
                                    jint action,
                                    jobject args,
                                    jint args_position) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->DispatchSemanticsAction(
      env,           //
      id,            //
      action,        //
      args,          //
      args_position  //
  );
}

static void SetSemanticsEnabled(JNIEnv* env,
                                jobject jcaller,
                                jlong shell_holder,
                                jboolean enabled) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->SetSemanticsEnabled(enabled);
}

static void SetAccessibilityFeatures(JNIEnv* env,
                                     jobject jcaller,
                                     jlong shell_holder,
                                     jint flags) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->SetAccessibilityFeatures(flags);
}

static jboolean GetIsSoftwareRendering(JNIEnv* env, jobject jcaller) {
  return FlutterMain::Get().GetSettings().enable_software_rendering;
}

static void RegisterTexture(JNIEnv* env,
                            jobject jcaller,
                            jlong shell_holder,
                            jlong texture_id,
                            jobject surface_texture) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->RegisterExternalTexture(
      static_cast<int64_t>(texture_id),                        //
      fml::jni::JavaObjectWeakGlobalRef(env, surface_texture)  //
  );
}

static void MarkTextureFrameAvailable(JNIEnv* env,
                                      jobject jcaller,
                                      jlong shell_holder,
                                      jlong texture_id) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->MarkTextureFrameAvailable(
      static_cast<int64_t>(texture_id));
}

static void UnregisterTexture(JNIEnv* env,
                              jobject jcaller,
                              jlong shell_holder,
                              jlong texture_id) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->UnregisterTexture(
      static_cast<int64_t>(texture_id));
}
/**
 * BD ADD: jni call to notify android  image load success
 */
static void ExternalImageLoadSuccess(JNIEnv *env,
        jobject jcaller,
        jstring key,
        jobject jBitmap) {

  auto cKey = fml::jni::JavaStringToString(env, key);
  g_mutex.lock();
  auto loadContext = g_image_load_contexts[cKey];
  g_image_load_contexts.erase(cKey);
  g_mutex.unlock();
  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  auto globalJBitmap = env->NewGlobalRef(jBitmap);
  loaderContext.task_runners.GetIOTaskRunner()->PostTask([cKey,
                                                              globalJBitmap,
                                                              loadContext]() {
    if (loadContext == nullptr) {
      return;
    }
    JNIEnv *jniEnv = fml::jni::AttachCurrentThread();
    loadContext->onLoadSuccess(jniEnv, cKey, globalJBitmap);
  });
}

/**
 * BD ADD: jni call to notify android  image load success
 */
static void ExternalImageLoadForCodecSuccess(JNIEnv *env,
                                     jobject jcaller,
                                     jstring key,
                                     jobject jCodec) {
  auto cKey = fml::jni::JavaStringToString(env, key);
  auto loadContext = g_image_load_contexts[cKey];
  if (loadContext == nullptr) {
    return;
  }
  loadContext->onLoadForCodecSuccess(env, cKey, env->NewGlobalRef(jCodec));

  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  loaderContext.task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable([cKey = std::move(cKey)](){
      g_image_load_contexts.erase(cKey);
  }));
}

static void ExternalImageLoadForGetNextFrameSuccess(JNIEnv *env,
                                     jobject jcaller,
                                     jstring key,
                                     jobject jBitmap) {
  auto cKey = fml::jni::JavaStringToString(env, key);
  auto loadContext = g_image_load_contexts[cKey];
  if (loadContext == nullptr) {
    return;
  }
  loadContext->onGetNextFrameSuccess(env, cKey, env->NewGlobalRef(jBitmap));

  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  loaderContext.task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable([cKey = std::move(cKey)](){
    g_image_load_contexts.erase(cKey);
  }));
}

/**
 * BD ADD: jni call to notify android image load fail
 */
static void ExternalImageLoadFail(JNIEnv *env,
        jobject jcaller,
        jstring key) {
  auto cKey = fml::jni::JavaStringToString(env, key);
  g_mutex.lock();
  auto loadContext = g_image_load_contexts[cKey];
  g_image_load_contexts.erase(cKey);
  g_mutex.unlock();
  if (!loadContext) {
    return;
  }
  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  loaderContext.task_runners.GetIOTaskRunner()->PostTask([env,
                                                              cKey,
                                                              loadContext]() {
    if (loadContext == nullptr) {
      return;
    }
    loadContext->onLoadFail(env, cKey);
  });
}


/**
 * BD ADD: jni call to notify android image load fail
 */
static void ExternalImageCodecLoadFail(JNIEnv *env,
                                  jobject jcaller,
                                  jstring key) {
  auto cKey = fml::jni::JavaStringToString(env, key);
  auto loadContext = g_image_load_contexts[cKey];
  if (loadContext == nullptr) {
    return;
  }
  loadContext->onCodecLoadFail(env, cKey);

  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  loaderContext.task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable([cKey = std::move(cKey)](){
      g_image_load_contexts.erase(cKey);
  }));
}

static void ExternalImageGetNextFrameFail(JNIEnv *env,
                                  jobject jcaller,
                                  jstring key) {
  auto cKey = fml::jni::JavaStringToString(env, key);
  auto loadContext = g_image_load_contexts[cKey];
  if (loadContext == nullptr) {
    return;
  }
  loadContext->onGetNextFrameFail(env, cKey);

  auto loaderContext = static_cast<ImageLoaderContext>(loadContext->loaderContext);
  if (!loaderContext.task_runners.IsValid()) {
    return;
  }
  loaderContext.task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable([cKey = std::move(cKey)](){
    g_image_load_contexts.erase(cKey);
  }));
}

/**
 * BD ADD: register android image loader
 */
static void RegisterAndroidImageLoader(JNIEnv *env,
                            jobject jcaller,
                            jlong shell_holder,
                            jobject android_image_loader) {
  ANDROID_SHELL_HOLDER->GetPlatformView()->RegisterExternalImageLoader(
      fml::jni::JavaObjectWeakGlobalRef(env, android_image_loader)
      );
}
/**
 * BD ADD: unregister android image loader
 */
static void UnRegisterAndroidImageLoader(JNIEnv *env,
                                       jobject jcaller,
                                       jlong shell_holder) {

}

static void InvokePlatformMessageResponseCallback(JNIEnv* env,
                                                  jobject jcaller,
                                                  jlong shell_holder,
                                                  jint responseId,
                                                  jobject message,
                                                  jint position) {
  ANDROID_SHELL_HOLDER->GetPlatformView()
      ->InvokePlatformMessageResponseCallback(env,         //
                                              responseId,  //
                                              message,     //
                                              position     //
      );
}

static void InvokePlatformMessageEmptyResponseCallback(JNIEnv* env,
                                                       jobject jcaller,
                                                       jlong shell_holder,
                                                       jint responseId) {
  ANDROID_SHELL_HOLDER->GetPlatformView()
      ->InvokePlatformMessageEmptyResponseCallback(env,        //
                                                   responseId  //
      );
}

bool RegisterApi(JNIEnv* env) {
  static const JNINativeMethod flutter_jni_methods[] = {
      // Start of methods from FlutterJNI
      {
          .name = "nativeAttach",
          .signature = "(Lio/flutter/embedding/engine/FlutterJNI;Z)J",
          .fnPtr = reinterpret_cast<void*>(&AttachJNI),
      },
      // BD ADD: START
      {
          .name = "nativeScheduleBackgroundFrame",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(&ScheduleBackgroundFrame),
      },
      // END
      {
          .name = "nativeDestroy",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(&DestroyJNI),
      },
      // BD ADD:START
      {
          .name = "nativeNotifyLowMemory",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(&NotifyLowMemory),
      },
      // END
      {
          .name = "nativeRunBundleAndSnapshotFromLibrary",
          .signature = "(JLjava/lang/String;Ljava/lang/String;"
                       "Ljava/lang/String;Landroid/content/res/AssetManager;)V",
          .fnPtr = reinterpret_cast<void*>(&RunBundleAndSnapshotFromLibrary),
      },
      {
          .name = "nativeDispatchEmptyPlatformMessage",
          .signature = "(JLjava/lang/String;I)V",
          .fnPtr = reinterpret_cast<void*>(&DispatchEmptyPlatformMessage),
      },
      {
          .name = "nativeDispatchPlatformMessage",
          .signature = "(JLjava/lang/String;Ljava/nio/ByteBuffer;II)V",
          .fnPtr = reinterpret_cast<void*>(&DispatchPlatformMessage),
      },
      {
          .name = "nativeInvokePlatformMessageResponseCallback",
          .signature = "(JILjava/nio/ByteBuffer;I)V",
          .fnPtr =
              reinterpret_cast<void*>(&InvokePlatformMessageResponseCallback),
      },
      {
          .name = "nativeInvokePlatformMessageEmptyResponseCallback",
          .signature = "(JI)V",
          .fnPtr = reinterpret_cast<void*>(
              &InvokePlatformMessageEmptyResponseCallback),
      },

      // Start of methods from FlutterView
      {
          .name = "nativeGetBitmap",
          .signature = "(J)Landroid/graphics/Bitmap;",
          .fnPtr = reinterpret_cast<void*>(&GetBitmap),
      },
      {
          .name = "nativeSurfaceCreated",
          .signature = "(JLandroid/view/Surface;)V",
          .fnPtr = reinterpret_cast<void*>(&SurfaceCreated),
      },
      {
          .name = "nativeSurfaceChanged",
          .signature = "(JII)V",
          .fnPtr = reinterpret_cast<void*>(&SurfaceChanged),
      },
      {
          .name = "nativeSurfaceDestroyed",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(&SurfaceDestroyed),
      },
      {
          .name = "nativeSetViewportMetrics",
          .signature = "(JFIIIIIIIIIIIIII)V",
          .fnPtr = reinterpret_cast<void*>(&SetViewportMetrics),
      },
      {
          .name = "nativeDispatchPointerDataPacket",
          .signature = "(JLjava/nio/ByteBuffer;I)V",
          .fnPtr = reinterpret_cast<void*>(&DispatchPointerDataPacket),
      },
      {
          .name = "nativeDispatchSemanticsAction",
          .signature = "(JIILjava/nio/ByteBuffer;I)V",
          .fnPtr = reinterpret_cast<void*>(&DispatchSemanticsAction),
      },
      {
          .name = "nativeSetSemanticsEnabled",
          .signature = "(JZ)V",
          .fnPtr = reinterpret_cast<void*>(&SetSemanticsEnabled),
      },
      {
          .name = "nativeSetAccessibilityFeatures",
          .signature = "(JI)V",
          .fnPtr = reinterpret_cast<void*>(&SetAccessibilityFeatures),
      },
      {
          .name = "nativeGetIsSoftwareRenderingEnabled",
          .signature = "()Z",
          .fnPtr = reinterpret_cast<void*>(&GetIsSoftwareRendering),
      },
      {
          .name = "nativeRegisterTexture",
          .signature = "(JJLandroid/graphics/SurfaceTexture;)V",
          .fnPtr = reinterpret_cast<void*>(&RegisterTexture),
      },
      {
          .name = "nativeMarkTextureFrameAvailable",
          .signature = "(JJ)V",
          .fnPtr = reinterpret_cast<void*>(&MarkTextureFrameAvailable),
      },
      {
          .name = "nativeUnregisterTexture",
          .signature = "(JJ)V",
          .fnPtr = reinterpret_cast<void*>(&UnregisterTexture),
      },

      // Methods for Dart callback functionality.
      {
          .name = "nativeLookupCallbackInformation",
          .signature = "(J)Lio/flutter/view/FlutterCallbackInformation;",
          .fnPtr = reinterpret_cast<void*>(&LookupCallbackInformation),
      },

      // BD ADD: add for register android image loader HuWeijie
      {
          .name = "nativeRegisterAndroidImageLoader",
          .signature = "(JLio/flutter/view/AndroidImageLoader;)V",
          .fnPtr = reinterpret_cast<void*>(&RegisterAndroidImageLoader),
      },
      // BD ADD: add for unregister android image loader HuWeijie
      {
          .name = "nativeUnregisterAndroidImageLoader",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(&UnRegisterAndroidImageLoader),
      },
  };

  if (env->RegisterNatives(g_flutter_jni_class->obj(), flutter_jni_methods,
                           fml::size(flutter_jni_methods)) != 0) {
    FML_LOG(ERROR) << "Failed to RegisterNatives with FlutterJNI";
    return false;
  }

  g_handle_platform_message_method =
      env->GetMethodID(g_flutter_jni_class->obj(), "handlePlatformMessage",
                       "(Ljava/lang/String;[BI)V");

  if (g_handle_platform_message_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate handlePlatformMessage method";
    return false;
  }

  g_handle_platform_message_response_method = env->GetMethodID(
      g_flutter_jni_class->obj(), "handlePlatformMessageResponse", "(I[B)V");

  if (g_handle_platform_message_response_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate handlePlatformMessageResponse method";
    return false;
  }

  g_update_semantics_method =
      env->GetMethodID(g_flutter_jni_class->obj(), "updateSemantics",
                       "(Ljava/nio/ByteBuffer;[Ljava/lang/String;)V");

  if (g_update_semantics_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate updateSemantics method";
    return false;
  }

  g_update_custom_accessibility_actions_method = env->GetMethodID(
      g_flutter_jni_class->obj(), "updateCustomAccessibilityActions",
      "(Ljava/nio/ByteBuffer;[Ljava/lang/String;)V");

  if (g_update_custom_accessibility_actions_method == nullptr) {
    FML_LOG(ERROR)
        << "Could not locate updateCustomAccessibilityActions method";
    return false;
  }

  g_on_first_frame_method =
      env->GetMethodID(g_flutter_jni_class->obj(), "onFirstFrame", "()V");

  if (g_on_first_frame_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate onFirstFrame method";
    return false;
  }

  g_on_engine_restart_method =
      env->GetMethodID(g_flutter_jni_class->obj(), "onPreEngineRestart", "()V");

  if (g_on_engine_restart_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate onEngineRestart method";
    return false;
  }

  return true;
}
// BD ADD: START
static void ExternalImageLoadSuccess(JNIEnv *env,
        jobject jcaller,
        jstring key,
        jobject jBitmap);

static void ExternalImageLoadForCodecSuccess(JNIEnv *env,
        jobject jcaller,
        jstring key,
        jobject jCodec);

static void ExternalImageLoadForGetNextFrameSuccess(JNIEnv *env,
                                             jobject jcaller,
                                             jstring key,
                                             jobject jCodec);

static void ExternalImageLoadFail(JNIEnv *env,
        jobject jcaller,
        jstring key);

static void ExternalImageCodecLoadFail(JNIEnv *env,
                                  jobject jcaller,
                                  jstring key);

static void ExternalImageGetNextFrameFail(JNIEnv *env,
                                       jobject jcaller,
                                       jstring key);

// END
bool PlatformViewAndroid::Register(JNIEnv* env) {
  if (env == nullptr) {
    FML_LOG(ERROR) << "No JNIEnv provided";
    return false;
  }

  g_flutter_callback_info_class = new fml::jni::ScopedJavaGlobalRef<jclass>(
      env, env->FindClass("io/flutter/view/FlutterCallbackInformation"));
  if (g_flutter_callback_info_class->is_null()) {
    FML_LOG(ERROR) << "Could not locate FlutterCallbackInformation class";
    return false;
  }

  g_flutter_callback_info_constructor = env->GetMethodID(
      g_flutter_callback_info_class->obj(), "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (g_flutter_callback_info_constructor == nullptr) {
    FML_LOG(ERROR) << "Could not locate FlutterCallbackInformation constructor";
    return false;
  }

  g_flutter_jni_class = new fml::jni::ScopedJavaGlobalRef<jclass>(
      env, env->FindClass("io/flutter/embedding/engine/FlutterJNI"));
  if (g_flutter_jni_class->is_null()) {
    FML_LOG(ERROR) << "Failed to find FlutterJNI Class.";
    return false;
  }

  g_surface_texture_class = new fml::jni::ScopedJavaGlobalRef<jclass>(
      env, env->FindClass("android/graphics/SurfaceTexture"));
  if (g_surface_texture_class->is_null()) {
    FML_LOG(ERROR) << "Could not locate SurfaceTexture class";
    return false;
  }
  // BD ADD: HuWeijie
  g_image_loader_class = new fml::jni::ScopedJavaGlobalRef<jclass>(env, env->FindClass("io/flutter/view/AndroidImageLoader"));
  if (g_image_loader_class->is_null()) {
      FML_LOG(ERROR) << "Could not locate AndroidImageLoader class";
      return false;
  }

  g_image_loader_class_load = env->GetMethodID(g_image_loader_class->obj(), "load", "(Ljava/lang/String;IIFLio/flutter/view/NativeLoadCallback;Ljava/lang/String;)V");
  if (g_image_loader_class_load == nullptr) {
    FML_LOG(ERROR) << "Could not locate AndroidImageLoader load method";
    return false;
  }

  g_image_loader_class_load_gif = env->GetMethodID(g_image_loader_class->obj(), "getNextFrame", "(ILjava/lang/Object;Lio/flutter/view/NativeLoadCallback;Ljava/lang/String;)V");
  if (g_image_loader_class_load == nullptr) {
    FML_LOG(ERROR) << "Could not locate AndroidImageLoader load method";
    return false;
  }

  g_image_loader_class_release = env->GetMethodID(g_image_loader_class->obj(), "release", "(Ljava/lang/String;)V");
  if (g_image_loader_class_release == nullptr) {
    FML_LOG(ERROR) << "Could not locate AndroidImageLoader release method";
    return false;
  }

  g_image_loader_callback_class = new fml::jni::ScopedJavaGlobalRef<jclass>(env, env->FindClass("io/flutter/view/NativeLoadCallback"));
  if (g_image_loader_callback_class->is_null()) {
      FML_LOG(ERROR) << "Could not locate NativeLoadCallback class";
      return false;
  }

  g_native_callback_constructor = env->GetMethodID(g_image_loader_callback_class->obj(), "<init>", "()V");
  if (g_native_callback_constructor == nullptr) {
    FML_LOG(ERROR) << "Could not locate NativeLoadCallback constructor";
    return false;
  }

  static const JNINativeMethod native_load_callback_methods[] = {
          {
              .name = "nativeSuccessCallback",
              .signature = "(Ljava/lang/String;Landroid/graphics/Bitmap;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageLoadSuccess),
          },
          {
              .name = "nativeCodecSuccessCallback",
              .signature = "(Ljava/lang/String;Lio/flutter/view/NativeCodec;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageLoadForCodecSuccess),
          },
          {
              .name = "nativeGetNextFrameSuccessCallback",
              .signature = "(Ljava/lang/String;Landroid/graphics/Bitmap;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageLoadForGetNextFrameSuccess),
          },
          {
              .name = "nativeFailCallback",
              .signature = "(Ljava/lang/String;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageLoadFail),
          },
          {
              .name = "nativeCodecFailCallback",
              .signature = "(Ljava/lang/String;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageCodecLoadFail),
          },
          {
              .name = "nativeGetNextFrameFailCallback",
              .signature = "(Ljava/lang/String;)V",
              .fnPtr = reinterpret_cast<void*>(&ExternalImageGetNextFrameFail),
          },
  };

  if (env->RegisterNatives(g_image_loader_callback_class->obj(),
                           native_load_callback_methods,
                           fml::size(native_load_callback_methods)) != 0) {
      FML_LOG(ERROR) << "Failed to RegisterNatives with NativeLoadCallback";
      return false;
  }
  // END

  g_is_released_method =
      env->GetMethodID(g_surface_texture_class->obj(), "isReleased", "()Z");

  // 解决5.x手机上的crash
  fml::jni::ClearException(env);

  g_attach_to_gl_context_method = env->GetMethodID(
      g_surface_texture_class->obj(), "attachToGLContext", "(I)V");

  if (g_attach_to_gl_context_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate attachToGlContext method";
    return false;
  }

  g_update_tex_image_method =
      env->GetMethodID(g_surface_texture_class->obj(), "updateTexImage", "()V");

  if (g_update_tex_image_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate updateTexImage method";
    return false;
  }

  g_get_transform_matrix_method = env->GetMethodID(
      g_surface_texture_class->obj(), "getTransformMatrix", "([F)V");

  if (g_get_transform_matrix_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate getTransformMatrix method";
    return false;
  }

  g_detach_from_gl_context_method = env->GetMethodID(
      g_surface_texture_class->obj(), "detachFromGLContext", "()V");

  if (g_detach_from_gl_context_method == nullptr) {
    FML_LOG(ERROR) << "Could not locate detachFromGlContext method";
    return false;
  }

  return RegisterApi(env);
}

}  // namespace flutter
