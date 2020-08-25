//
//  https://jira.bytedance.com/browse/FLUTTER-61
//  Created by wangying on 2019/8/6.
//

#include "boost.h"
#include <thread>

namespace flutter {

static const int64_t kDefaultMaxTime = 60000;

static const int64_t kMaxSkipGCTime = 5000;

Boost::Boost()
    : boost_flags_(0),
      gc_deadline_(0),
      ui_message_athead_deadline_(0),
      notify_idle_deadline_(0) {}

Boost::~Boost() = default;

void Boost::StartUp(uint16_t flags, int millis) {
  int64_t now = Dart_TimelineGetMicros();
  int64_t deadline = now + millis * 1000;
  int64_t limited_deadline =
      millis < kDefaultMaxTime ? deadline : (now + kDefaultMaxTime * 1000);

  if (flags & Flags::kDisableGC) {
    gc_deadline_ =
        millis < kMaxSkipGCTime ? deadline : (now + kMaxSkipGCTime * 1000);
  }
  if (flags & Flags::kUiMessageAtHead) {
    ui_message_athead_deadline_ = limited_deadline;
  }
  if (flags & Flags::kCanNotifyIdle) {
    notify_idle_deadline_ = deadline;
  }

  boost_flags_ |= flags;
}

void Boost::CheckFinished() {
  if (boost_flags_ == 0) {
    return;
  }
  int64_t current_micros = Dart_TimelineGetMicros();
  uint16_t finish_flags = 0;
  if (gc_deadline_ > 0 && gc_deadline_ < current_micros) {
    finish_flags |= kDisableGC;
  }
  if (ui_message_athead_deadline_ > 0 &&
      ui_message_athead_deadline_ < current_micros) {
    finish_flags |= kUiMessageAtHead;
  }
  if (notify_idle_deadline_ > 0 && notify_idle_deadline_ < current_micros) {
    finish_flags |= kCanNotifyIdle;
  }
  Finish(finish_flags);
}

void Boost::Finish(uint16_t flags) {
  if (flags == 0) {
    return;
  }
  boost_flags_ &= ~flags;

  if (flags & kDisableGC) {
    gc_deadline_ = 0;
  }
  if (flags & Flags::kUiMessageAtHead) {
    ui_message_athead_deadline_ = 0;
  }
}

bool Boost::IsGCDisabled() {
  return boost_flags_ & kDisableGC;
}

bool Boost::IsUiMessageAtHead() {
  return boost_flags_ & kUiMessageAtHead;
}

bool Boost::CanNotifyIdle() {
  return boost_flags_ & Flags::kCanNotifyIdle;
}

void Boost::ForceGC() {
#if defined(DART_PERFORMANCE_EXTENSION)
  Dart_ForceGC();
#endif
}

}  // namespace flutter
