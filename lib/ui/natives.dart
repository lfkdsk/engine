// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(dnfield): remove unused_element ignores when https://github.com/dart-lang/sdk/issues/35164 is resolved.

part of dart.ui;

// Corelib 'print' implementation.
void _print(dynamic arg) {
  _Logger._printString(arg.toString());
}

class _Logger {
  static void _printString(String s) native 'Logger_PrintString';
}

// A service protocol extension to schedule a frame to be rendered into the
// window.
Future<developer.ServiceExtensionResponse> _scheduleFrame(
    String method,
    Map<String, String> parameters
    ) async {
  // Schedule the frame.
  window.scheduleFrame();
  // Always succeed.
  return new developer.ServiceExtensionResponse.result(json.encode(<String, String>{
    'type': 'Success',
  }));
}

@pragma('vm:entry-point')
void _setupHooks() {  // ignore: unused_element
  assert(() {
    // In debug mode, register the schedule frame extension.
    developer.registerExtension('ext.ui.window.scheduleFrame', _scheduleFrame);
    return true;
  }());
}

/// Returns runtime Dart compilation trace as a UTF-8 encoded memory buffer.
///
/// The buffer contains a list of symbols compiled by the Dart JIT at runtime up
/// to the point when this function was called. This list can be saved to a text
/// file and passed to tools such as `flutter build` or Dart `gen_snapshot` in
/// order to pre-compile this code offline.
///
/// The list has one symbol per line of the following format:
/// `<namespace>,<class>,<symbol>\n`.
///
/// Here are some examples:
///
/// ```
/// dart:core,Duration,get:inMilliseconds
/// package:flutter/src/widgets/binding.dart,::,runApp
/// file:///.../my_app.dart,::,main
/// ```
///
/// This function is only effective in debug and dynamic modes, and will throw in AOT mode.
List<int> saveCompilationTrace() {
  final dynamic result = _saveCompilationTrace();
  if (result is Error)
    throw result;
  return result;
}

dynamic _saveCompilationTrace() native 'SaveCompilationTrace';

void _scheduleMicrotask(void callback()) native 'ScheduleMicrotask';

int _getCallbackHandle(Function closure) native 'GetCallbackHandle';
Function _getCallbackFromHandle(int handle) native 'GetCallbackFromHandle';

// Required for gen_snapshot to work correctly.
int _isolateId; // ignore: unused_element

@pragma('vm:entry-point')
Function _getPrintClosure() => _print;  // ignore: unused_element
@pragma('vm:entry-point')
Function _getScheduleMicrotaskClosure() => _scheduleMicrotask; // ignore: unused_element


// === native interpreter begin ===
bool nativeDetect(Function function) native 'NativeDetect';

dynamic nativeCall0(Function closure, int optionalNamedParametersCount) native 'NativeCall0';
dynamic nativeCall1(Function closure, int optionalNamedParametersCount, dynamic arg1) native 'NativeCall1';
dynamic nativeCall2(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2) native 'NativeCall2';
dynamic nativeCall3(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3) native 'NativeCall3';
dynamic nativeCall4(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4) native 'NativeCall4';
dynamic nativeCall5(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5) native 'NativeCall5';
dynamic nativeCall6(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5, dynamic arg6) native 'NativeCall6';
dynamic nativeCall7(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5, dynamic arg6, dynamic arg7) native 'NativeCall7';
dynamic nativeCall8(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5, dynamic arg6, dynamic arg7, dynamic arg8) native 'NativeCall8';
dynamic nativeCall9(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5, dynamic arg6, dynamic arg7, dynamic arg8, dynamic arg9) native 'NativeCall9';
dynamic nativeCall10(Function closure, int optionalNamedParametersCount, dynamic arg1, dynamic arg2, dynamic arg3, dynamic arg4, dynamic arg5, dynamic arg6, dynamic arg7, dynamic arg8, dynamic arg9, dynamic arg10) native 'NativeCall10';
