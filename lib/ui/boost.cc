//
//  https://jira.bytedance.com/browse/FLUTTER-61
//  Created by wangying on 2019/8/6.
//

#include "boost.h"
#include <thread>
#include "SkCanvas.h"
#include "flutter/lib/ui/text/font_collection.h"
#include "flutter/lib/ui/window/window.h"
#include "minikin/FontLanguageListCache.h"

namespace flutter {

static const int64_t kMaxSkipGCTime = 5000 * 1000;

Boost::Boost()
    : boost_flags_(0),
      gc_deadline_(0),
      aa_deadline_(0),
      delay_future_deadline_(0),
      delay_platform_message_deadline_(0),
      ui_message_athead_deadline_(0),
      wait_swap_buffer_deadline_(0),
      vsync_received_(false),
      dart_frame_deadline_(TimePoint::Now()),
      extend_buffer_deadline_(0),
      extend_count_(0),
      extend_semaphore_(4),
      weak_factory_(this) {}

Boost::~Boost() = default;

void Boost::Init(bool disable_anti_alias) {
#if defined(SKIA_PERFORMANCE_EXTENSION)
  gSkDisableAntiAlias = disable_anti_alias;
#endif
}

void Boost::StartUp(uint16_t flags, int millis) {
#if defined(DEBUG)
  FML_LOG(INFO) << "StartUp Flags:" << flags << " millis:" millis;
#endif

  int64_t current_deadline_ = millis * 1000 + Dart_TimelineGetMicros();

  if (flags & Flags::kDisableGC) {
    gc_deadline_ =
        current_deadline_ < kMaxSkipGCTime ? current_deadline_ : kMaxSkipGCTime;
  }
  if (flags & Flags::kDisableAA) {
    aa_deadline_ = current_deadline_;
  }
  if (flags & Flags::kEnableWaitSwapBuffer) {
    wait_swap_buffer_deadline_ = current_deadline_;
  }
  if (flags & Flags::kEnableExtendBuffer) {
    extend_buffer_deadline_ = current_deadline_;
  }
  if (flags & Flags::kDelayFuture) {
    delay_future_deadline_ = current_deadline_;
    if (!(boost_flags_ & Flags::kDelayFuture)) {
      const auto& task_runners = UIDartState::Current()->GetTaskRunners();
      task_runners.GetUITaskRunner()->PostBarrier(true);
      task_runners.GetUITaskRunner()->PostDelayedTask(
          [&task_runners]() {
            Boost::Current()->Finish(Flags::kDelayFuture, &task_runners);
          },
          TimeDelta::FromMilliseconds(millis));
    }
  }
  if (flags & Flags::kDelayPlatformMessage) {
    delay_platform_message_deadline_ = current_deadline_;
    if (!(boost_flags_ & Flags::kDelayFuture)) {
      const auto& task_runners = UIDartState::Current()->GetTaskRunners();
      task_runners.GetPlatformTaskRunner()->PostBarrier(true);
      task_runners.GetUITaskRunner()->PostDelayedTask(
          [&task_runners]() {
            Boost::Current()->Finish(Flags::kDelayPlatformMessage,
                                     &task_runners);
          },
          TimeDelta::FromMilliseconds(millis));
    }
  }
  if (flags & Flags::kUiMessageAtHead) {
    ui_message_athead_deadline_ = current_deadline_;
  }
  boost_flags_ |= flags;
#if defined(DEBUG)
  FML_LOG(INFO) << "StartUp Boost Flags:" << boost_flags_;
#endif
}

void Boost::CheckFinished() {
  int64_t current_micros = Dart_TimelineGetMicros();
  uint16_t finish_flags = 0;
  if (gc_deadline_ > 0 && gc_deadline_ < current_micros) {
    finish_flags |= kDisableGC;
  }
  if (aa_deadline_ > 0 && aa_deadline_ < current_micros) {
    finish_flags |= kDisableAA;
  }
  if (wait_swap_buffer_deadline_ > 0 &&
      wait_swap_buffer_deadline_ < current_micros) {
    finish_flags |= kEnableWaitSwapBuffer;
  }
  if (extend_buffer_deadline_ > 0 && extend_buffer_deadline_ < current_micros) {
    finish_flags |= kEnableExtendBuffer;
  }
  if (delay_future_deadline_ > 0 && delay_future_deadline_ < current_micros) {
    finish_flags |= kDelayFuture;
  }
  if (delay_platform_message_deadline_ > 0 &&
      delay_platform_message_deadline_ < current_micros) {
    finish_flags |= kDelayPlatformMessage;
  }
  if (ui_message_athead_deadline_ > 0 &&
      ui_message_athead_deadline_ < current_micros) {
    finish_flags |= kUiMessageAtHead;
  }
  Finish(finish_flags);
}

void Boost::Finish(uint16_t flags, const TaskRunners* task_runners) {
  if (flags <= 0) {
    return;
  }
#if defined(DEBUG)
  FML_LOG(INFO) << "Finish Flags:" << flags << "Origin:" << boost_flags_;
#endif
  boost_flags_ &= ~flags;

  if (flags & kDisableGC) {
    gc_deadline_ = 0;
  }
  if (flags & kDisableAA) {
    aa_deadline_ = 0;
  }
  if (flags & kEnableWaitSwapBuffer) {
    wait_swap_buffer_deadline_ = 0;
  }
  if (flags & Flags::kEnableExtendBuffer) {
    extend_buffer_deadline_ = 0;
  }
  if (flags & Flags::kDelayFuture) {
    delay_future_deadline_ = 0;
    if (task_runners == nullptr) {
      task_runners = &UIDartState::Current()->GetTaskRunners();
    }
    task_runners->GetUITaskRunner()->PostBarrier(false);
  }
  if (flags & Flags::kDelayPlatformMessage) {
    delay_platform_message_deadline_ = 0;
    if (task_runners == nullptr) {
      task_runners = &UIDartState::Current()->GetTaskRunners();
    }
    task_runners->GetPlatformTaskRunner()->PostBarrier(false);
  }
  if (flags & Flags::kUiMessageAtHead) {
    ui_message_athead_deadline_ = 0;
  }
}

bool Boost::IsAADisabled() {
#if defined(SKIA_PERFORMANCE_EXTENSION)
  return boost_flags_ & kDisableAA || gSkDisableAntiAlias;
#else
  return boost_flags_ & kDisableAA;
#endif
}

bool Boost::IsGCDisabled() {
  return boost_flags_ & kDisableGC;
}

bool Boost::IsDelayFuture() {
  return boost_flags_ & kDelayFuture;
}

bool Boost::IsDelayPlatformMessage() {
  return boost_flags_ & kDelayPlatformMessage;
}

bool Boost::IsUiMessageAtHead() {
  return boost_flags_ & kUiMessageAtHead;
}

bool Boost::IsEnableExtendBuffer() {
  return boost_flags_ & kEnableExtendBuffer;
}

bool Boost::IsValidExtension() {
  return IsEnableExtendBuffer() && extend_semaphore_.IsValid();
}

bool Boost::TryWaitExtension() {
  if (IsEnableExtendBuffer() && extend_semaphore_.TryWait()) {
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
  if (vsync_received_ || !(boost_flags_ & kEnableWaitSwapBuffer)) {
    return;
  }
  double next_frame_time =
      (dart_frame_deadline_ - TimePoint::Now()).ToMillisecondsF() + 0.5;
  if (next_frame_time < 0) {
    return;
  }
  if (next_frame_time > 16) {
    next_frame_time = 16;
  }
  AutoResetWaitableEvent swap_buffer_wait;
  swap_buffer_wait.WaitWithTimeout(
      TimeDelta::FromMilliseconds(next_frame_time));
}

void Boost::PreloadFontFamilies(const std::vector<std::string>& font_families,
                                const std::string& locale) {
  FontCollection& collection =
      UIDartState::Current()->window()->client()->GetFontCollection();
  std::string minikin_locale;
  if (!locale.empty()) {
    uint32_t language_list_id =
        minikin::FontStyle::registerLanguageList(locale);
    const minikin::FontLanguages& langs =
        minikin::FontLanguageListCache::getById(language_list_id);
    if (langs.size()) {
      minikin_locale = langs[0].getString();
    }
  }
  collection.GetFontCollection()->GetMinikinFontCollectionForFamilies(
      font_families, minikin_locale);
}

}  // namespace flutter
