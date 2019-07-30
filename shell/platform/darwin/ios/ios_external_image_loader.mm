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
#include "flutter/lib/ui/ui_dart_state.h"
#include "flutter/fml/make_copyable.h"
//#include "stdlib.h"

namespace flutter {
    
    IOSExternalImageLoader::IOSExternalImageLoader(NSObject<FlutterImageLoader>* imageLoader): imageLoader_(imageLoader) {
        FML_DCHECK(imageLoader_);
    }
    
    IOSExternalImageLoader::~IOSExternalImageLoader() = default;
    
    void releaseTexture(SkImage::ReleaseContext releaseContext) {
        CFRelease(releaseContext);
    }
    
    void DummyReleaseProc(const void*ptr, void*context) {
        std::free((char*)ptr);
//        std::free(context);
    }
    
    void IOSExternalImageLoader::Load(const std::string url, void* dartState, std::function<void(sk_sp<SkImage> image)> callback) {
        NSString* URL = [NSString stringWithCString:url.c_str() encoding:[NSString defaultCStringEncoding]];
//        [imageLoader_ loadImage:URL complete: ^(CVPixelBufferRef pixelBuffer, void *data, int width2, int height2, int size2, int bytesPerRow2) {
//            if (!cache_ref_) {
//                CVOpenGLESTextureCacheRef cache;
//                CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL,
//                                                            [EAGLContext currentContext], NULL, &cache);
//                if (err == noErr) {
//                    cache_ref_.Reset(cache);
//                } else {
//                    FML_LOG(WARNING) << "Failed to create GLES texture cache: " << err;
//                    return;
//                }
//            }
////            fml::CFRef<CVPixelBufferRef> bufferRef;
//            CVOpenGLESTextureRef texture = nullptr;
//            if (true) {
////                bufferRef.Reset(pixelBuffer);
//                if (pixelBuffer != nullptr) {
////                    CVOpenGLESTextureRef texture;
//                    CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(
////                                                                                kCFAllocatorDefault, cache_ref_, bufferRef, nullptr, GL_TEXTURE_2D, GL_RGBA,
//                                                                                kCFAllocatorDefault, cache_ref_, pixelBuffer, nullptr, GL_TEXTURE_2D, GL_RGBA,
//                                                                                static_cast<int>(CVPixelBufferGetWidth(pixelBuffer)),
//                                                                                static_cast<int>(CVPixelBufferGetHeight(pixelBuffer)), GL_BGRA, GL_UNSIGNED_BYTE, 0,
//                                                                                &texture);
////                    texture_ref_.Reset(texture);
//                    if (err != noErr) {
//                        FML_LOG(WARNING) << "Could not create texture from pixel buffer: " << err;
//                        return;
//                    }
//                }
//            }
////            if (!texture_ref_) {
//                        if (!texture) {
//                return;
//            }
////            GrGLTextureInfo textureInfo = {CVOpenGLESTextureGetTarget(texture_ref_),
////                CVOpenGLESTextureGetName(texture_ref_), GL_RGBA8_OES};
//            GrGLTextureInfo textureInfo = {CVOpenGLESTextureGetTarget(texture),
//                CVOpenGLESTextureGetName(texture), GL_RGBA8_OES};
//            int width = CVPixelBufferGetWidth(pixelBuffer);
//            int height = CVPixelBufferGetHeight(pixelBuffer);
//            CFRelease(pixelBuffer);
//            GrBackendTexture backendTexture(width, height, GrMipMapped::kNo, textureInfo);
//            UIDartState* dart_state = static_cast<UIDartState*>(dartState);
//            const auto& task_runners = dart_state->GetTaskRunners();
//            fml::RefPtr<fml::TaskRunner> ui_task_runner = task_runners.GetUITaskRunner();
//            fml::WeakPtr<GrContext> context = dart_state->GetResourceContext();
//            auto io_task_runner = dart_state->GetTaskRunners().GetIOTaskRunner();
//
//           io_task_runner->PostTask(fml::MakeCopyable(
//                                                       [context = std::move(context),
//                                                        texture = std::move(texture),
//                                                       callback = std::move(callback),
//                                                       backendTexture = std::move(backendTexture)]() mutable {
//
//                                                           SkImage::TextureReleaseProc releaseP = &releaseTexture;
//                                                           SkImage::ReleaseContext releaseC = texture;
//                                                          sk_sp<SkImage> image =
//                                                          SkImage::MakeFromTexture(context.get(), backendTexture,
//                                                                                   kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
//                                                                                   kPremul_SkAlphaType, nullptr, releaseP, releaseC);
//                                                           glBindTexture(GL_TEXTURE_2D, CVOpenGLESTextureGetName(texture));
////                                                           CFRelease(texture);
//                                                          FML_DCHECK(image) << "Failed to create SkImage from Texture.";
//                                                           callback(std::move(image));                                                      }));
////           }));
////            sk_sp<SkImage> image =
////            SkImage::MakeFromTexture(context.get(), backendTexture,
////                                     kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
////                                     kPremul_SkAlphaType, nullptr);
////            FML_DCHECK(image) << "Failed to create SkImage from Texture.";
////            callback(image);
//        }];
                [imageLoader_ loadImage:URL complete: ^(CVPixelBufferRef pixelBuffer, void *data, int width, int height, int size, int bytesPerRow) {
//                    free(data);
                    UIDartState* dart_state = static_cast<UIDartState*>(dartState);
                    auto io_task_runner = dart_state->GetTaskRunners().GetIOTaskRunner();
                    fml::WeakPtr<GrContext> context = dart_state->GetResourceContext();
                    io_task_runner->PostTask(fml::MakeCopyable(
                       [data = std::move(data),
                        dart_state = std::move(dart_state),
                        context = std::move(context),
                        width,
                        height,
                        size,
                        bytesPerRow,
                       callback = std::move(callback)]() mutable {
                           sk_sp<SkData> buffer = SkData::MakeWithProc(data, size, &DummyReleaseProc, NULL);
//                         sk_sp<SkData> buffer = SkData::MakeWithoutCopy(data, size);
                         SkImageInfo sk_info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
                         sk_sp<SkImage> skImage;
//                         if (context) {
//                           SkPixmap pixmap(sk_info, buffer->data(), bytesPerRow);
//                           skImage = SkImage::MakeCrossContextFromPixmap(context.get(), pixmap, true, nullptr, true);
//                         } else {
                           skImage = SkImage::MakeRasterData(sk_info, std::move(buffer), bytesPerRow);
//                         }
                           callback(std::move(skImage));
                       }));
                }];
    }
    
}  // namespace flutter
