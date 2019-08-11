//
//  https://jira.bytedance.com/browse/FLUTTER-61
//  Created by wangying on 2019/8/6.
//

#ifndef BOOST_H
#define BOOST_H

#include <stdio.h>
#include <atomic>
#include "flutter/fml/synchronization/semaphore.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/time/time_point.h"
#include "third_party/dart/runtime/include/dart_tools_api.h"

namespace flutter {
using namespace fml;
using namespace std;

class Boost {
 public:
  enum Flags {
    kDisableAA = 1 << 0,
    kDisableGC = 1 << 1,
    kEnableWaitSwapBuffer = 1 << 2,
    kEnableBarrier = 1 << 3,
  };

  static constexpr unsigned kAllFlags = 0x0F;

 public:
  static Boost* Current() {
    static Boost instance;
    return &instance;
  }

  int GetQueueLength();

  void Init(bool disable_anti_alias, int queue_length = 2);
  void StartUp(int flags, int millis);
  void CheckFinished();

  bool IsAADisabled();
  bool IsGCDisabled();
  bool IsBarrierEnabled();

  void WaitSwapBufferIfNeed();
  void UpdateVsync(bool received,
                   TimePoint frame_target_time = TimePoint::Now());
  void UpdateFrameDeadline(int64_t time);

  void Finish(int flags);

 private:
  Boost();
  ~Boost();

  int64_t boost_deadline_;

  int ui_gpu_queue_length_;

  bool is_aa_disabled_;
  bool is_gc_disagled_;
  bool is_wait_swap_buffer_enabled_;
  bool is_barrier_enabled_;

  atomic_bool vsync_received_;
  TimePoint dart_frame_deadline_;

  FML_DISALLOW_COPY_AND_ASSIGN(Boost);
};

}  // namespace flutter
#endif /* boost_h */
