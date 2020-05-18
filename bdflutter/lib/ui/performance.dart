/// BD ADD

// @dart = 2.9
part of dart.ui;

typedef NotifyIdleCallback = void Function(Duration duration);

class Performance {

  /// Memory usage of decoded image in dart heap external, in KB
  int getImageMemoryUsage() native 'Performance_imageMemoryUsage';

  void startRecordFps(String key) native 'Performance_startRecordFps';

  List obtainFps(String key, bool stopRecord) native 'Performance_obtainFps';

  void startBoost(int flags, int millis) native 'Performance_startBoost';

  void finishBoost(int flags) native 'Performance_finishBoost';

  void forceGC() native 'Performance_forceGC';

  void disableMips(bool disable) native 'Performance_disableMips';

  NotifyIdleCallback? get onNotifyIdle => _onNotifyIdle;
  NotifyIdleCallback? _onNotifyIdle;
  Zone? _onNotifyIdleZone;
  set onNotifyIdle(NotifyIdleCallback callback) {
    _onNotifyIdle = callback;
    _onNotifyIdleZone = Zone.current;
  }
}

/// The [Performance] singleton.
final Performance performance = Performance();
