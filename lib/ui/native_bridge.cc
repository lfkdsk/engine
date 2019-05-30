#include "flutter/lib/ui/native_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <third_party/dart/runtime/vm/os.h>

#include "flutter/common/settings.h"
#include "flutter/fml/build_config.h"
#include "flutter/fml/logging.h"
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/dart/runtime/include/bin/dart_io_api.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/dart/runtime/include/dart_tools_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_microtask_queue.h"
#include "third_party/tonic/dart_state.h"
#include "third_party/tonic/logging/dart_error.h"
#include "third_party/tonic/logging/dart_invoke.h"
#include "third_party/tonic/scopes/dart_api_scope.h"
#include "third_party/tonic/scopes/dart_isolate_scope.h"



using tonic::DartConverter;
using tonic::LogIfError;
using tonic::ToDart;

namespace flutter {


static void PropagateIfError(Dart_Handle result) {
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}

void NativeDetect(Dart_NativeArguments args) {
  Dart_Handle func = Dart_GetNativeArgument(args, 0);
  Dart_SetReturnValue(args, ToDart(Dart_NeedInvokeInterpreter(func)));
}

void NativeCall(Dart_NativeArguments args) {
  // 第一个参数为需要调用的function
  Dart_Handle closure = Dart_GetNativeArgument(args, 0);

  // 第二个参数为Named可选参数的个数
  int optional_named_args_length = tonic::DartConverter<int>::FromDart(Dart_GetNativeArgument(args, 1));


  int start = 2;

  // 构建Named可选参数
  Dart_Handle optional_args_names[optional_named_args_length];
  for (int j = 0; j < optional_named_args_length; ++j) {
    optional_args_names[j] = Dart_GetNativeArgument(args, start + j);
  }

  // 构建实际参数
  int all_args_length = Dart_GetNativeArgumentCount(args) - optional_named_args_length - 2;
  Dart_Handle all_args[all_args_length];
  for (int i = 0; i < all_args_length; i++) {
    all_args[i] = Dart_GetNativeArgument(args, start + optional_named_args_length + i);
  }

  Dart_Handle result = Dart_InvokeInterpreter(closure, all_args_length, all_args, optional_named_args_length, optional_args_names);
  PropagateIfError(result);
  Dart_SetReturnValue(args, result);
}

void NativeCall0(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall1(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall2(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall3(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall4(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall5(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall6(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall7(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall8(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall9(Dart_NativeArguments args) { NativeCall(args); }
void NativeCall10(Dart_NativeArguments args) { NativeCall(args); }

void NativeBridge::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({
                        {"NativeDetect", NativeDetect, 1, true},
                        {"NativeCall0", NativeCall0, 2, true},
                        {"NativeCall1", NativeCall1, 3, true},
                        {"NativeCall2", NativeCall2, 4, true},
                        {"NativeCall3", NativeCall3, 5, true},
                        {"NativeCall4", NativeCall4, 6, true},
                        {"NativeCall5", NativeCall5, 7, true},
                        {"NativeCall6", NativeCall6, 8, true},
                        {"NativeCall7", NativeCall7, 9, true},
                        {"NativeCall8", NativeCall8, 10, true},
                        {"NativeCall9", NativeCall9, 11, true},
                        {"NativeCall10", NativeCall10, 12, true},
                    });
}

}  // namespace flutter
