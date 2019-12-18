// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/text/paragraph.h"

#include "flutter/common/settings.h"
#include "flutter/common/task_runners.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/task_runner.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
// BD ADD: START
#include "flutter/fml/make_copyable.h"
#include "third_party/tonic/logging/dart_invoke.h"
// END

using tonic::ToDart;

namespace flutter {

// BD ADD: START
double clamp(double num, double lowerLimit, double upperLimit) {
  return num < lowerLimit ? lowerLimit : (num > upperLimit ? upperLimit : num);
}
// END

IMPLEMENT_WRAPPERTYPEINFO(ui, Paragraph);

#define FOR_EACH_BINDING(V)         \
  V(Paragraph, width)               \
  V(Paragraph, height)              \
  V(Paragraph, tightWidth)          \
  V(Paragraph, minIntrinsicWidth)   \
  V(Paragraph, maxIntrinsicWidth)   \
  V(Paragraph, alphabeticBaseline)  \
  V(Paragraph, ideographicBaseline) \
  V(Paragraph, didExceedMaxLines)   \
  V(Paragraph, layout)              \
  V(Paragraph, paint)               \
  V(Paragraph, getWordBoundary)     \
  V(Paragraph, getRectsForRange)    \
  V(Paragraph, getPositionForOffset)

DART_BIND_ALL(Paragraph, FOR_EACH_BINDING)

Paragraph::Paragraph(std::unique_ptr<txt::Paragraph> paragraph)
    : m_paragraphImpl(
          // BD MOD:
          // std::make_unique<ParagraphImplTxt>(std::move(paragraph))) {}
          std::make_shared<ParagraphImplTxt>(std::move(paragraph))) {}

Paragraph::~Paragraph() = default;

size_t Paragraph::GetAllocationSize() {
  // We don't have an accurate accounting of the paragraph's memory consumption,
  // so return a fixed size to indicate that its impact is more than the size
  // of the Paragraph class.
  return 2000;
}

double Paragraph::width() {
  return m_paragraphImpl->width();
}

double Paragraph::height() {
  return m_paragraphImpl->height();
}

double Paragraph::tightWidth() {
  return m_paragraphImpl->tightWidth();
}

double Paragraph::minIntrinsicWidth() {
  return m_paragraphImpl->minIntrinsicWidth();
}

double Paragraph::maxIntrinsicWidth() {
  return m_paragraphImpl->maxIntrinsicWidth();
}

double Paragraph::alphabeticBaseline() {
  return m_paragraphImpl->alphabeticBaseline();
}

double Paragraph::ideographicBaseline() {
  return m_paragraphImpl->ideographicBaseline();
}

bool Paragraph::didExceedMaxLines() {
  return m_paragraphImpl->didExceedMaxLines();
}

// BD MOD: START
// void Paragraph::layout(double width) {
//   m_paragraphImpl->layout(width);
// }
void Paragraph::layout(double maxWidth,
                       double minWidth,
                       bool async,
                       Dart_Handle callback_handle) {
  tonic::DartPersistentValue* callback = new tonic::DartPersistentValue(
      tonic::DartState::Current(), callback_handle);
  const auto& task_runners = UIDartState::Current()->GetTaskRunners();
  m_paragraphImpl->setAsyncMode(async);

  auto closure = [weak_paragraphImpl =
                      std::weak_ptr<ParagraphImpl>(m_paragraphImpl),
                  ui_task_runner = task_runners.GetUITaskRunner(), maxWidth,
                  minWidth, callback]() {
    if (auto paragraphImpl = weak_paragraphImpl.lock()) {
      paragraphImpl->layout(maxWidth);

      if (minWidth >= 0 && minWidth != maxWidth) {
        double newWidth =
            clamp(ceil(paragraphImpl->maxIntrinsicWidth()), minWidth, maxWidth);
        if (newWidth != ceil(paragraphImpl->width())) {
          paragraphImpl->layout(newWidth);
        }
      }
    }

    fml::TaskRunner::RunNowOrPostTask(ui_task_runner, [callback]() {
      std::shared_ptr<tonic::DartState> dart_state =
          callback->dart_state().lock();
      if (!dart_state) {
        return;
      }
      tonic::DartState::Scope scope(dart_state);
      if (Dart_IsClosure(callback->value())) {
        tonic::DartInvoke(callback->value(), {});
      }
      delete callback;
    });
  };

  if (async) {
    task_runners.GetIOTaskRunner()->PostTask(closure);
  } else {
    closure();
  }
}
// end

void Paragraph::paint(Canvas* canvas, double x, double y) {
  m_paragraphImpl->paint(canvas, x, y);
}

std::vector<TextBox> Paragraph::getRectsForRange(unsigned start,
                                                 unsigned end,
                                                 unsigned boxHeightStyle,
                                                 unsigned boxWidthStyle) {
  return m_paragraphImpl->getRectsForRange(
      start, end, static_cast<txt::Paragraph::RectHeightStyle>(boxHeightStyle),
      static_cast<txt::Paragraph::RectWidthStyle>(boxWidthStyle));
}

Dart_Handle Paragraph::getPositionForOffset(double dx, double dy) {
  return m_paragraphImpl->getPositionForOffset(dx, dy);
}

Dart_Handle Paragraph::getWordBoundary(unsigned offset) {
  return m_paragraphImpl->getWordBoundary(offset);
}

}  // namespace flutter
