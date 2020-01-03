// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/common/thread_host.h"

namespace flutter {

ThreadHost::ThreadHost() = default;

ThreadHost::ThreadHost(ThreadHost&&) = default;

// BD ADD: START
#ifdef OS_MACOSX
ThreadHost::ThreadHost(std::string name_prefix, uint64_t mask, bool highQoS) {
#else
// END
ThreadHost::ThreadHost(std::string name_prefix, uint64_t mask) {
// BD ADD:
#endif
  if (mask & ThreadHost::Type::Platform) {
    platform_thread = std::make_unique<fml::Thread>(name_prefix + ".platform");
  }

  if (mask & ThreadHost::Type::UI) {
// BD MOD: START
// ui_thread = std::make_unique<fml::Thread>(name_prefix + ".ui");
#if OS_ANDROID
    ui_thread = std::make_unique<fml::Thread>(name_prefix + ".ui", true);
// BD ADD: START
#elif OS_MACOSX
    ui_thread = std::make_unique<fml::Thread>(name_prefix + ".ui", highQoS);
// END
#else
    ui_thread = std::make_unique<fml::Thread>(name_prefix + ".ui");
#endif
    // END
  }

  if (mask & ThreadHost::Type::GPU) {
// BD ADD: START
#if OS_MACOSX
    gpu_thread = std::make_unique<fml::Thread>(name_prefix + ".gpu", highQoS);
#else
// END
    gpu_thread = std::make_unique<fml::Thread>(name_prefix + ".gpu");
// BD ADD:
#endif
  }

  if (mask & ThreadHost::Type::IO) {
    io_thread = std::make_unique<fml::Thread>(name_prefix + ".io");
  }
}

ThreadHost::~ThreadHost() = default;

void ThreadHost::Reset() {
  platform_thread.reset();
  ui_thread.reset();
  gpu_thread.reset();
  io_thread.reset();
}

}  // namespace flutter
