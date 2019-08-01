//
// Created by jay on 2019-07-31.
//

#include "android_external_image_loader.h"
#include "platform_view_android_jni.h"

namespace flutter {
    AndroidExternalImageLoader::AndroidExternalImageLoader(
            const fml::jni::JavaObjectWeakGlobalRef &android_image_loader):android_image_loader_(android_image_loader) {}

    AndroidExternalImageLoader::~AndroidExternalImageLoader() {}

    void AndroidExternalImageLoader::Load(const std::string url, void *contextPtr,
                                          std::function<void(sk_sp < SkImage > image)> callback) {
        CallJavaImageLoader()
    }
}