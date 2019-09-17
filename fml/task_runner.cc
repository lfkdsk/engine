// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include "flutter/fml/task_runner.h"

#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/message_loop_impl.h"

namespace fml {

TaskRunner::TaskRunner(fml::RefPtr<MessageLoopImpl> loop)
    : loop_(std::move(loop)) {}

TaskRunner::~TaskRunner() = default;

void TaskRunner::PostTask(fml::closure task) {
  loop_->PostTask(std::move(task), fml::TimePoint::Now());
}

void TaskRunner::PostTaskForTime(fml::closure task,
                                 fml::TimePoint target_time) {
  loop_->PostTask(std::move(task), target_time);
}

void TaskRunner::PostDelayedTask(fml::closure task, fml::TimeDelta delay) {
  loop_->PostTask(std::move(task), fml::TimePoint::Now() + delay);
}

// BD START:
void TaskRunner::PostTaskAtHead(fml::closure task) {
  loop_->PostTask(std::move(task), fml::TimePoint::FromEpochDelta(TimeDelta::FromNanoseconds(1)));
}
// END
bool TaskRunner::RunsTasksOnCurrentThread() {
  if (!fml::MessageLoop::IsInitializedForCurrentThread()) {
    return false;
  }
  return MessageLoop::GetCurrent().GetLoopImpl() == loop_;
}

void TaskRunner::RunNowOrPostTask(fml::RefPtr<fml::TaskRunner> runner,
                                  fml::closure task) {
  FML_DCHECK(runner);
  if (runner->RunsTasksOnCurrentThread()) {
    task();
  } else {
    runner->PostTask(std::move(task));
  }
}
// BD ADD: START
void TaskRunner::RunNowOrPostTaskAtHead(fml::RefPtr<fml::TaskRunner> runner,
                                        fml::closure task){
  FML_DCHECK(runner);
  if (runner->RunsTasksOnCurrentThread()) {
    task();
  } else {
    runner->PostTaskAtHead(std::move(task));
  }
}
// END

// BD ADD: START
void TaskRunner::PostBarrier(bool barrier_enabled) {
    loop_->PostBarrier(barrier_enabled);
}

void TaskRunner::PostTask(fml::closure task, bool is_low_priority) {
    loop_->PostTask(std::move(task), fml::TimePoint::Now(), is_low_priority);
}

void TaskRunner::PostTaskForTime(fml::closure task,
                                 fml::TimePoint target_time, bool is_low_priority) {
    loop_->PostTask(std::move(task), target_time, is_low_priority);
}

void TaskRunner::PostDelayedTask(fml::closure task, fml::TimeDelta delay, bool is_low_priority) {
    loop_->PostTask(std::move(task), fml::TimePoint::Now() + delay, is_low_priority);
}
// END
}  // namespace fml
