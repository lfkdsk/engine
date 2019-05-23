// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_NATIVE_INTERPRETER_H_
#define FLUTTER_LIB_NATIVE_INTERPRETER_H_

#include "flutter/fml/macros.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/dart_library_natives.h"

namespace flutter {

class NativeInterpreter {
 public:
  static void RegisterNatives(tonic::DartLibraryNatives* natives);

 private:
  FML_DISALLOW_IMPLICIT_CONSTRUCTORS(NativeInterpreter);
};

}  // namespace flutter

#endif  // FLUTTER_LIB_NATIVE_INTERPRETER_H_
