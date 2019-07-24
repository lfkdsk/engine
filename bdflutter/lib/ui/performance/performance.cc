// BD ADD

#include "performance.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "bdflutter/common/fps_recorder.h"
#include "boost.h"

namespace flutter {

Performance::Performance():
    dart_image_memory_usage(0) {}

int64_t Performance::GetImageMemoryUsageKB() {
  int64_t sizeInKB = dart_image_memory_usage.load(std::memory_order_relaxed);
  return  sizeInKB > 0 ? sizeInKB : 0;
}

void Performance::SubImageMemoryUsage(int64_t sizeInKB) {
  dart_image_memory_usage.fetch_sub(sizeInKB, std::memory_order_relaxed);
}

void Performance::AddImageMemoryUsage(int64_t sizeInKB) {
  dart_image_memory_usage.fetch_add(sizeInKB, std::memory_order_relaxed);
}

void Performance_imageMemoryUsage(Dart_NativeArguments args) {
  Dart_SetReturnValue(args, tonic::DartConverter<int64_t>::ToDart(
      Performance::GetInstance()->GetImageMemoryUsageKB()));
}

void Performance_startRecordFps(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;
  std::string key = tonic::DartConverter<std::string>::FromArguments(args, 1, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  FpsRecorder::Current()->StartRecordFps(key);
}

void Performance_obtainFps(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;
  std::string key = tonic::DartConverter<std::string>::FromArguments(args, 1, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  bool stop_record = tonic::DartConverter<bool>::FromArguments(args, 2, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  std::vector<double> fpsInfo = FpsRecorder::Current()->ObtainFpsData(key, stop_record);
  int size = 3;
  Dart_Handle data_handle = Dart_NewList(size);
  for (int i = 0; i != size; i++) {
    Dart_Handle value = Dart_NewDouble(fpsInfo[i]);
    Dart_ListSetAt(data_handle, i, value);
  }
  Dart_SetReturnValue(args, data_handle);
}

void Performance_startBoost(Dart_NativeArguments args) {
  uint16_t flags = (uint16_t)tonic::DartConverter<int>::FromDart(Dart_GetNativeArgument(args, 0));
  int millis = tonic::DartConverter<int>::FromDart(Dart_GetNativeArgument(args, 1));
  Boost::Current()->StartUp(flags, millis);
}

void Performance_finishBoost(Dart_NativeArguments args) {
  uint16_t flags = (uint16_t)tonic::DartConverter<int>::FromDart(Dart_GetNativeArgument(args, 0));
  Boost::Current()->Finish(flags);
}

void Performance_forceGC(Dart_NativeArguments args) {
  Boost::Current()->ForceGC();
}

void Performance::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({
      {"Performance_imageMemoryUsage", Performance_imageMemoryUsage, 1, true},
      {"Performance_startRecordFps", Performance_startRecordFps, 2, true},
      {"Performance_obtainFps", Performance_obtainFps, 3, true},
      {"Performance_startBoost", Performance_startBoost, 3, true},
      {"Performance_finishBoost", Performance_finishBoost, 2, true},
      {"Performance_forceGC", Performance_forceGC, 1, true},
  });
}

}
