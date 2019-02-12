// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "flutter/shell/platform/darwin/ios/framework/Source/vsync_waiter_ios.h"

#include <utility>

#include <Foundation/Foundation.h>
#include <QuartzCore/CADisplayLink.h>
#include <mach/mach_time.h>

#include "flutter/common/task_runners.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

@interface VSyncClient : NSObject

- (instancetype)initWithTaskRunner:(fml::RefPtr<fml::TaskRunner>)task_runner
                          callback:(shell::VsyncWaiter::Callback)callback;

- (void)await;

- (void)invalidate;

@end

namespace shell {

VsyncWaiterIOS::VsyncWaiterIOS(blink::TaskRunners task_runners)
    : VsyncWaiter(std::move(task_runners)),
      client_([[VSyncClient alloc] initWithTaskRunner:task_runners_.GetUITaskRunner()
                                             callback:std::bind(&VsyncWaiterIOS::FireCallback,
                                                                this,
                                                                std::placeholders::_1,
                                                                std::placeholders::_2)]) {}

VsyncWaiterIOS::~VsyncWaiterIOS() {
  // This way, we will get no more callbacks from the display link that holds a weak (non-nilling)
  // reference to this C++ object.
  [client_.get() invalidate];
}

void VsyncWaiterIOS::AwaitVSync() {
  [client_.get() await];
}

}  // namespace shell

@implementation VSyncClient {
  shell::VsyncWaiter::Callback callback_;
  fml::scoped_nsobject<CADisplayLink> display_link_;
}

- (instancetype)initWithTaskRunner:(fml::RefPtr<fml::TaskRunner>)task_runner
                          callback:(shell::VsyncWaiter::Callback)callback {
  self = [super init];

  if (self) {
    callback_ = std::move(callback);
    display_link_ = fml::scoped_nsobject<CADisplayLink> {
      [[CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink:)] retain]
    };
    display_link_.get().paused = YES;

    task_runner->PostTask([client = [self retain]]() {
      [client->display_link_.get() addToRunLoop:[NSRunLoop currentRunLoop]
                                        forMode:NSRunLoopCommonModes];
      [client release];
    });
  }

  return self;
}

- (void)await {
  display_link_.get().paused = NO;
}

- (void)onDisplayLink:(CADisplayLink*)link {
  fml::TimePoint frame_start_time = fml::TimePoint::Now();
  fml::TimePoint frame_target_time = frame_start_time + fml::TimeDelta::FromSecondsF(link.duration);

  display_link_.get().paused = YES;

  callback_(frame_start_time, frame_target_time);
}

- (void)invalidate {
  // [CADisplayLink invalidate] is thread-safe.
  [display_link_.get() invalidate];
}

- (void)dealloc {
  [self invalidate];

  [super dealloc];
}

@end
