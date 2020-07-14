//
// Created by bytedance on 2020/7/14.
//

#ifndef SRC_PERFORMANCE_H
#define SRC_PERFORMANCE_H

#include <atomic>
#include "flutter/fml/logging.h"

namespace flutter {

class Performance {
 public:
  static Performance* GetInstance() {
    static Performance instance;
    return &instance;
  }
  void AddImageMemoryUsage(size_t imageSize);
  void SubImageMemoryUsage(size_t imageSize);

  uint64_t GetImageMemoryUsage();  // KB

 private:
  Performance();

  std::atomic_uint_fast64_t dart_image_memory_usage;  // Byte
};

}
#endif  // SRC_PERFORMANCE_H
