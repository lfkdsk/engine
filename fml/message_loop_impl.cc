// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include "flutter/fml/message_loop_impl.h"

#include <algorithm>
#include <vector>

#include "flutter/fml/build_config.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

#if OS_MACOSX
#include "flutter/fml/platform/darwin/message_loop_darwin.h"
#elif OS_ANDROID
#include "flutter/fml/platform/android/message_loop_android.h"
#elif OS_LINUX
#include "flutter/fml/platform/linux/message_loop_linux.h"
#elif OS_WIN
#include "flutter/fml/platform/win/message_loop_win.h"
#endif

namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create() {
#if OS_MACOSX
  return fml::MakeRefCounted<MessageLoopDarwin>();
#elif OS_ANDROID
  return fml::MakeRefCounted<MessageLoopAndroid>();
#elif OS_LINUX
  return fml::MakeRefCounted<MessageLoopLinux>();
#elif OS_WIN
  return fml::MakeRefCounted<MessageLoopWin>();
#else
  return nullptr;
#endif
}

// BD MOD: WangYing
MessageLoopImpl::MessageLoopImpl()
    : task_queue_(MessageLoopTaskQueues::GetInstance()),
      queue_id_(task_queue_->CreateTaskQueue()),
      terminated_(false), barrier_enabled_(false) {
  task_queue_->SetWakeable(queue_id_, this);
}
// END

MessageLoopImpl::~MessageLoopImpl() = default;

void MessageLoopImpl::PostTask(fml::closure task, fml::TimePoint target_time) {
  FML_DCHECK(task != nullptr);
  FML_DCHECK(task != nullptr);
  if (terminated_) {
    // If the message loop has already been terminated, PostTask should destruct
    // |task| synchronously within this function.
    return;
  }
  task_queue_->RegisterTask(queue_id_, task, target_time);
}

void MessageLoopImpl::AddTaskObserver(intptr_t key, fml::closure callback) {
  FML_DCHECK(callback != nullptr);
  FML_DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
      << "Message loop task observer must be added on the same thread as the "
         "loop.";
  task_queue_->AddTaskObserver(queue_id_, key, callback);
}

void MessageLoopImpl::RemoveTaskObserver(intptr_t key) {
  FML_DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
      << "Message loop task observer must be removed from the same thread as "
         "the loop.";
  task_queue_->RemoveTaskObserver(queue_id_, key);
}

void MessageLoopImpl::DoRun() {
  if (terminated_) {
    // Message loops may be run only once.
    return;
  }

  // Allow the implementation to do its thing.
  Run();

  // The loop may have been implicitly terminated. This can happen if the
  // implementation supports termination via platform specific APIs or just
  // error conditions. Set the terminated flag manually.
  terminated_ = true;

  // The message loop is shutting down. Check if there are expired tasks. This
  // is the last chance for expired tasks to be serviced. Make sure the
  // terminated flag is already set so we don't accrue additional tasks now.
  RunExpiredTasksNow();

  // When the message loop is in the process of shutting down, pending tasks
  // should be destructed on the message loop's thread. We have just returned
  // from the implementations |Run| method which we know is on the correct
  // thread. Drop all pending tasks on the floor.

  task_queue_->Dispose(queue_id_);

  // BD ADD: WangYing
  low_priority_tasks_ = {};
  // END
}

void MessageLoopImpl::DoTerminate() {
  terminated_ = true;
  Terminate();
}

// Thread safety analysis disabled as it does not account for defered locks.
void MessageLoopImpl::SwapTaskQueues(const fml::RefPtr<MessageLoopImpl>& other)
    FML_NO_THREAD_SAFETY_ANALYSIS {
  if (terminated_ || other->terminated_) {
    return;
  }

  // task_flushing locks
  std::unique_lock<std::mutex> t1(tasks_flushing_mutex_, std::defer_lock);
  std::unique_lock<std::mutex> t2(other->tasks_flushing_mutex_,
                                  std::defer_lock);

  std::lock(t1, t2);
  task_queue_->Swap(queue_id_, other->queue_id_);
}

void MessageLoopImpl::FlushTasks(FlushType type) {
  TRACE_EVENT0("fml", "MessageLoop::FlushTasks");
  std::vector<fml::closure> invocations;

  // We are grabbing this lock here as a proxy to indicate
  // that we are running tasks and will invoke the
  // "right" observers, we are trying to avoid the scenario
  // where:
  // gather invocations -> Swap -> execute invocations
  // will lead us to run invocations on the wrong thread.
  std::scoped_lock task_flush_lock(tasks_flushing_mutex_);
  task_queue_->GetTasksToRunNow(queue_id_, type, invocations);

  for (const auto& invocation : invocations) {
    invocation();
    task_queue_->NotifyObservers(queue_id_);
  }
  // BD ADD: START
  if (!barrier_enabled_) {
    FlushLowPriorityTasks(type);
  }
  // END
}

// BD ADD: START
void MessageLoopImpl::PostBarrier(bool barrier_enabled) {
  barrier_enabled_ = barrier_enabled;
  if (!barrier_enabled_) {
    std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);
    if (low_priority_tasks_.empty()) {
      return;
    }
    WakeUp(low_priority_tasks_.top().target_time);
  }
}

void MessageLoopImpl::PostTask(fml::closure task,
                               fml::TimePoint target_time,
                               bool is_low_priority) {
  FML_DCHECK(task != nullptr);
  RegisterTask(task, target_time, is_low_priority);
}

void MessageLoopImpl::RegisterTask(fml::closure task,
                                   fml::TimePoint target_time,
                                   bool is_low_priority) {
  FML_DCHECK(task != nullptr);
  if (terminated_) {
    // If the message loop has already been terminated, PostTask should destruct
    // |task| synchronously within this function.
    return;
  }
  std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);
  if (is_low_priority) {
    low_priority_tasks_.push({++order_, std::move(task), target_time});
    WakeUp(low_priority_tasks_.top().target_time);
  } else {
    delayed_tasks_.push({++order_, std::move(task), target_time});
    WakeUp(delayed_tasks_.top().target_time);
  }
}

void MessageLoopImpl::FlushLowPriorityTasks(FlushType type) {
  TRACE_EVENT0("fml", "MessageLoop::FlushLowPriorityTasks");
  std::vector<fml::closure> invocations;

  {
    std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);

    if (low_priority_tasks_.empty()) {
      return;
    }

    auto now = fml::TimePoint::Now();
    while (!low_priority_tasks_.empty()) {
      const auto& top = low_priority_tasks_.top();
      if (top.target_time > now) {
        break;
      }
      invocations.emplace_back(std::move(top.task));
      low_priority_tasks_.pop();
      if (type == FlushType::kSingle) {
        break;
      }
    }
    WakeUp(low_priority_tasks_.empty() ? fml::TimePoint::Max()
                                       : low_priority_tasks_.top().target_time);
  }
  for (const auto& invocation : invocations) {
    invocation();
    for (const auto& observer : task_observers_) {
      observer.second();
    }
  }
}
// END

void MessageLoopImpl::RunExpiredTasksNow() {
  FlushTasks(FlushType::kAll);
}

void MessageLoopImpl::RunSingleExpiredTaskNow() {
  FlushTasks(FlushType::kSingle);
}

TaskQueueId MessageLoopImpl::GetTaskQueueId() const {
  return queue_id_;
}

}  // namespace fml
