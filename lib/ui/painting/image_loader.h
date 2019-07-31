// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_IMAGE_LOADER_H_
#define FLUTTER_FLOW_IMAGE_LOADER_H_

#include "third_party/skia/include/core/SkCanvas.h"
#include "flutter/fml/macros.h"

namespace flutter {
    
    class ImageLoader {
    public:
        ImageLoader();
        virtual ~ImageLoader();
        virtual void Load(const std::string url, void* contextPtr, std::function<void(sk_sp<SkImage> image)> callback) = 0;
        
        FML_DISALLOW_COPY_AND_ASSIGN(ImageLoader);
    };
    
}  // namespace flutter

#endif  // FLUTTER_FLOW_IMAGE_LOADER_H_
