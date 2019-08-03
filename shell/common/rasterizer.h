// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_RASTERIZER_H_
#define SHELL_COMMON_RASTERIZER_H_

#include <memory>
#include <optional>

#include "flutter/common/task_runners.h"
#include "flutter/flow/compositor_context.h"
#include "flutter/flow/layers/layer_tree.h"
#include "flutter/fml/closure.h"
#include "flutter/fml/memory/weak_ptr.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/lib/ui/snapshot_delegate.h"
#include "flutter/shell/common/pipeline.h"
#include "flutter/shell/common/surface.h"

namespace flutter {

class Rasterizer final : public SnapshotDelegate {
 public:
  Rasterizer(TaskRunners task_runners);

  Rasterizer(TaskRunners task_runners,
             std::unique_ptr<flutter::CompositorContext> compositor_context);

  ~Rasterizer();

  void Setup(std::unique_ptr<Surface> surface);

  void Teardown();

  // Frees up Skia GPU resources.
  //
  // This method must be called from the GPU task runner.
  void NotifyLowMemoryWarning() const;

  fml::WeakPtr<Rasterizer> GetWeakPtr() const;

  fml::WeakPtr<SnapshotDelegate> GetSnapshotDelegate() const;

  flutter::LayerTree* GetLastLayerTree();

  void DrawLastLayerTree();

  flutter::TextureRegistry* GetTextureRegistry();

  void Draw(fml::RefPtr<Pipeline<flutter::LayerTree>> pipeline);

  enum class ScreenshotType {
    SkiaPicture,
    UncompressedImage,  // In kN32_SkColorType format
    CompressedImage,
  };

  struct Screenshot {
    sk_sp<SkData> data;
    SkISize frame_size = SkISize::MakeEmpty();

    Screenshot();

    Screenshot(sk_sp<SkData> p_data, SkISize p_size);

    Screenshot(const Screenshot& other);

    ~Screenshot();
  };

  Screenshot ScreenshotLastLayerTree(ScreenshotType type, bool base64_encode);

  // Sets a callback that will be executed after the next frame is submitted to
  // the surface on the GPU task runner.
  void SetNextFrameCallback(fml::closure callback);

  void AddNextFrameCallback(fml::closure callback);

  flutter::CompositorContext* compositor_context() {
    return compositor_context_.get();
  }

  //----------------------------------------------------------------------------
  /// @brief      Skia has no notion of time. To work around the performance
  ///             implications of this, it may cache GPU resources to reference
  ///             them from one frame to the next. Using this call, embedders
  ///             may set the maximum bytes cached by Skia in its caches
  ///             dedicated to on-screen rendering.
  ///
  /// @attention  This cache setting will be invalidated when the surface is
  ///             torn down via `Rasterizer::Teardown`. This call must be made
  ///             again with new limits after surface re-acquisition.
  ///
  /// @attention  This cache does not describe the entirety of GPU resources
  ///             that may be cached. The `RasterCache` also holds very large
  ///             GPU resources.
  ///
  /// @see        `RasterCache`
  ///
  /// @param[in]  max_bytes  The maximum byte size of resource that may be
  ///                        cached for GPU rendering.
  /// @param[in]  from_user  Whether this request was from user code, e.g. via
  ///                        the flutter/skia message channel, in which case
  ///                        it should not be overridden by the platform.
  ///
  void SetResourceCacheMaxBytes(size_t max_bytes, bool from_user);

  //----------------------------------------------------------------------------
  /// @brief      The current value of Skia's resource cache size, if a surface
  ///             is present.
  ///
  /// @attention  This cache does not describe the entirety of GPU resources
  ///             that may be cached. The `RasterCache` also holds very large
  ///             GPU resources.
  ///
  /// @see        `RasterCache`
  ///
  /// @return     The size of Skia's resource cache, if available.
  ///
  std::optional<size_t> GetResourceCacheMaxBytes() const;

 private:
  TaskRunners task_runners_;
  std::unique_ptr<Surface> surface_;
  std::unique_ptr<flutter::CompositorContext> compositor_context_;
  std::unique_ptr<flutter::LayerTree> last_layer_tree_;
  fml::closure next_frame_callback_;
  std::vector<fml::closure> next_frame_callbacks_;
  bool user_override_resource_cache_bytes_;
  std::optional<size_t> max_cache_bytes_;
  fml::WeakPtrFactory<Rasterizer> weak_factory_;

  // |SnapshotDelegate|
  sk_sp<SkImage> MakeRasterSnapshot(sk_sp<SkPicture> picture,
                                    SkISize picture_size) override;

  void DoDraw(std::unique_ptr<flutter::LayerTree> layer_tree);

  bool DrawToSurface(flutter::LayerTree& layer_tree);

  void FireNextFrameCallbackIfPresent();

  FML_DISALLOW_COPY_AND_ASSIGN(Rasterizer);
};

}  // namespace flutter

#endif  // SHELL_COMMON_RASTERIZER_H_
