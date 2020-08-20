/// BD ADD

// @dart = 2.9
part of dart.ui;

class Performance {

  /// Memory usage of decoded image in dart heap external, in KB
  int getImageMemoryUsage() native 'Performance_imageMemoryUsage';

  void startRecordFps(String key) native 'Performance_startRecordFps';

  List obtainFps(String key, bool stopRecord) native 'Performance_obtainFps';
}

/// The [Performance] singleton.
final Performance performance = Performance();
