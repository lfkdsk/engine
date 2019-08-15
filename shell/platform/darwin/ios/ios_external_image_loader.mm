// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/ios/ios_external_image_loader.h"

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#include "flutter/shell/platform/darwin/ios/framework/Source/vsync_waiter_ios.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "flutter/common/task_runners.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/lib/ui/ui_dart_state.h"

// BD ADD: START
namespace flutter {
    
    IOSExternalImageLoader::IOSExternalImageLoader(NSObject<FlutterImageLoader>* imageLoader): imageLoader_(imageLoader) {
        FML_DCHECK(imageLoader_);
    }
    
    IOSExternalImageLoader::~IOSExternalImageLoader() = default;
    
    void releaseCVOpenGLESTextureRef(SkImage::ReleaseContext releaseContext) {
        if (releaseContext) {
            CFRelease(releaseContext);
        }
    }
    
    void IOSExternalImageLoader::Load(const std::string url, void* contextPtr, std::function<void(sk_sp<SkImage> image)> callback) {
        NSString* URL = [NSString stringWithCString:url.c_str() encoding:[NSString defaultCStringEncoding]];
        [imageLoader_ loadImage:URL complete: ^(IOSImageInfo imageInfo) {
            if (!cache_ref_) {
                CVOpenGLESTextureCacheRef cache;
                CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL,
                                                            [EAGLContext currentContext], NULL, &cache);
                if (err == noErr) {
                    cache_ref_.Reset(cache);
                } else {
                    FML_LOG(WARNING) << "Failed to create GLES texture cache: " << err;
                    return;
                }
            }
            fml::CFRef<CVPixelBufferRef> bufferRef;
            CVOpenGLESTextureRef texture = nullptr;
            if (true) {
                bufferRef.Reset(imageInfo.pixelBufferRef);
                if (bufferRef != nullptr) {
                    CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault, cache_ref_, bufferRef, nullptr, GL_TEXTURE_2D, GL_RGBA,
                        static_cast<int>(CVPixelBufferGetWidth(bufferRef)),
                        static_cast<int>(CVPixelBufferGetHeight(bufferRef)), GL_BGRA, GL_UNSIGNED_BYTE, 0,
                        &texture);
                    if (err != noErr) {
                        FML_LOG(WARNING) << "Could not create texture from pixel buffer: " << err;
                        return;
                    }
                }
            }
            if (!texture) {
                return;
            }
            GrGLTextureInfo textureInfo = {CVOpenGLESTextureGetTarget(texture),
                                           CVOpenGLESTextureGetName(texture), GL_RGBA8_OES};
            GrBackendTexture backendTexture(
                static_cast<int>(CVPixelBufferGetWidth(bufferRef)),
                static_cast<int>(CVPixelBufferGetHeight(bufferRef)),
                GrMipMapped::kNo,
                textureInfo);
            UIDartState* dartState = static_cast<UIDartState*>(contextPtr);
            fml::WeakPtr<GrContext> context = dartState->GetResourceContext();
            auto io_task_runner = dartState->GetTaskRunners().GetIOTaskRunner();
            io_task_runner->PostTask(fml::MakeCopyable(
                [context = std::move(context),
                 texture = std::move(texture),
                 callback = std::move(callback),
                 backendTexture = std::move(backendTexture)]() mutable {
                  SkImage::TextureReleaseProc releaseP = &releaseCVOpenGLESTextureRef;
                  SkImage::ReleaseContext releaseC = texture;
                  sk_sp<SkImage> image =
                      SkImage::MakeFromTexture(context.get(), backendTexture,
                                               kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
                                               kPremul_SkAlphaType, nullptr, releaseP, releaseC);
                  FML_DCHECK(image) << "Failed to create SkImage from Texture.";
                  callback(std::move(image));
                }));
        }];
    }
    
}  // namespace flutter
// END
