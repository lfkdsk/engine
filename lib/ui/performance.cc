//
// Created by bytedance on 2020/7/14.
//

#include "performance.h"

namespace flutter {

Performance::Performance():
      dart_image_memory_usage(0u) {}

uint64_t Performance::GetImageMemoryUsage() {
  return dart_image_memory_usage.load(std::memory_order_relaxed) >> 10;
}

void Performance::SubImageMemoryUsage(size_t imageSize) {
  dart_image_memory_usage.fetch_sub(imageSize, std::memory_order_relaxed);
  // to delete
  FML_LOG(ERROR) << "image memory " << GetImageMemoryUsage() << "KB ( -"
                 << (imageSize >> 10) << ")";
}

void Performance::AddImageMemoryUsage(size_t imageSize) {
  dart_image_memory_usage.fetch_add(imageSize, std::memory_order_relaxed);
  // to delete
  FML_LOG(ERROR) << "image memory " << GetImageMemoryUsage() << "KB ( +"
                 << (imageSize >> 10) << ")";
}

}
