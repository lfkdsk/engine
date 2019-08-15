//
//  https://jira.bytedance.com/browse/FLUTTER-61
//  Created by wangying on 2019/8/6.
//

#include "boost.h"
#include <thread>
#include "SkCanvas.h"

namespace flutter {

Boost::Boost()
    : boost_deadline_(0),
      ui_gpu_queue_length_(2),
      is_aa_disabled_(false),
      is_gc_disagled_(false),
      is_wait_swap_buffer_enabled_(true),
      vsync_received_(false),
      dart_frame_deadline_(TimePoint::Now()),
      can_extend_(false),
      extend_count_(0),
      extend_semaphore_(6) {}

Boost::~Boost() = default;

int Boost::GetQueueLength() {
  return ui_gpu_queue_length_;
}

void Boost::Init(bool disable_anti_alias, int queue_length) {
#if defined(SKIA_PERFORMANCE_EXTENSION)
  gSkDisableAntiAlias = disable_anti_alias;
#endif
  ui_gpu_queue_length_ = queue_length;
}

void Boost::StartUp(int flags, int millis) {
  boost_deadline_ = millis * 1000 + Dart_TimelineGetMicros();
  if (flags & Flags::kDisableAA) {
    is_aa_disabled_ = true;
  }
  if (flags & Flags::kDisableGC) {
    is_gc_disagled_ = true;
#if defined(DART_PERFORMANCE_EXTENSION)
    Dart_SkipGCFromNow(millis);
#endif
  }
  if (flags & Flags::kEnableWaitSwapBuffer) {
    is_wait_swap_buffer_enabled_ = true;
  }
  if (flags & Flags::kDelayFuture) {
    is_delay_future_ = true;
    UIDartState::Current()->GetTaskRunners().GetUITaskRunner()->PostBarrier(
        true);
  }
  if (flags & Flags::kDelayPlatformMessage) {
    is_delay_platform_message_ = true;
    UIDartState::Current()
        ->GetTaskRunners()
        .GetPlatformTaskRunner()
        ->PostBarrier(true);
  }
  if (flags & Flags::kEnableExtendBuffer) {
    can_extend_ = true;
  }
}

void Boost::CheckFinished() {
  if (boost_deadline_ <= 0 || boost_deadline_ > Dart_TimelineGetMicros()) {
    return;
  }
  Finish(kAllFlags);
}

bool Boost::IsAADisabled() {
#if defined(SKIA_PERFORMANCE_EXTENSION)
  return is_aa_disabled_ || gSkDisableAntiAlias;
#else
  return is_aa_disabled_;
#endif
}

bool Boost::IsGCDisabled() {
  return is_gc_disagled_;
}

bool Boost::IsDelayFuture() {
  return is_delay_future_;
}

bool Boost::IsDelayPlatformMessage() {
  return is_delay_platform_message_;
}

bool Boost::IsValidExtension() {
  return can_extend_ && extend_semaphore_.IsValid();
}

bool Boost::TryWaitExtension() {
  if (can_extend_ && extend_semaphore_.TryWait()) {
    extend_count_++;
    return true;
  }
  return false;
}

bool Boost::SignalExtension() {
  if (extend_count_ <= 0) {
    return false;
  }
  extend_count_--;
  extend_semaphore_.Signal();
  return true;
}

void Boost::UpdateVsync(bool received, TimePoint frame_target_time) {
  vsync_received_ = received;
  if (received) {
    dart_frame_deadline_ = frame_target_time;
  }
}

// If haven't received the vsync signal, wait util the frame dead line.
void Boost::WaitSwapBufferIfNeed() {
  if (vsync_received_ || !is_wait_swap_buffer_enabled_) {
    return;
  }
  int next_frame_time =
      (dart_frame_deadline_ - TimePoint::Now()).ToMilliseconds() + 1;
  if (next_frame_time > 16) {
    next_frame_time = 16;
  }
  if (next_frame_time > 0) {
    AutoResetWaitableEvent swap_buffer_wait;
    swap_buffer_wait.WaitWithTimeout(
        TimeDelta::FromMilliseconds(next_frame_time));
  }
}

void Boost::Finish(int flags) {
  if (boost_deadline_ <= 0) {
    return;
  }
  boost_deadline_ = 0;
  if (flags & kDisableAA) {
    is_aa_disabled_ = false;
  }
  if (flags & kDisableGC) {
    is_gc_disagled_ = false;
#if defined(DART_PERFORMANCE_EXTENSION)
    Dart_SkipGCFromNow(0);
#endif
  }
  if (flags & kEnableWaitSwapBuffer) {
    is_wait_swap_buffer_enabled_ = false;
  }
  if (flags & Flags::kDelayFuture) {
    is_delay_future_ = false;
    UIDartState::Current()->GetTaskRunners().GetUITaskRunner()->PostBarrier(
        false);
  }
  if (flags & Flags::kDelayPlatformMessage) {
    is_delay_platform_message_ = false;
    UIDartState::Current()
        ->GetTaskRunners()
        .GetPlatformTaskRunner()
        ->PostBarrier(false);
  }
  if (flags & Flags::kEnableExtendBuffer) {
    can_extend_ = false;
  }
}

}  // namespace flutter
