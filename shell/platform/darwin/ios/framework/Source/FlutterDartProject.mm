// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include "flutter/shell/platform/darwin/ios/framework/Source/FlutterDartProject_Internal.h"

#include "flutter/common/task_runners.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/platform/darwin/scoped_nsobject.h"
#include "flutter/runtime/dart_vm.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/platform/darwin/common/command_line.h"
#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterViewController.h"

static const char* kApplicationKernelSnapshotFileName = "kernel_blob.bin";

static blink::Settings CreateEmptySettings() {
  auto command_line = shell::CommandLineFromNSProcessInfo();

  auto settings = shell::SettingsFromCommandLine(command_line);

  settings.task_observer_add = [](intptr_t key, fml::closure callback) {
    fml::MessageLoop::GetCurrent().AddTaskObserver(key, std::move(callback));
  };

  settings.task_observer_remove = [](intptr_t key) {
    fml::MessageLoop::GetCurrent().RemoveTaskObserver(key);
  };

  return settings;
}

static NSString * GetIcuDataPath(NSBundle* bundle = nil) {
  NSBundle* engineBundle = [NSBundle bundleForClass:[FlutterViewController class]];

  // Flutter ships the ICU data file in the the bundle of the engine. Look for it there.
  return [engineBundle pathForResource:@"icudtl" ofType:@"dat"];
}

static blink::Settings DefaultSettingsForProcess(NSBundle* bundle = nil) {
  // Precedence:
  // 1. Settings from the specified NSBundle.
  // 2. Settings passed explicitly via command-line arguments.
  // 3. Settings from the NSBundle with the default bundle ID.
  // 4. Settings from the main NSBundle and default values.

  NSBundle* mainBundle = [NSBundle mainBundle];
  NSBundle* engineBundle = [NSBundle bundleForClass:[FlutterViewController class]];

  bool hasExplicitBundle = bundle != nil;
  if (bundle == nil) {
    bundle = [NSBundle bundleWithIdentifier:[FlutterDartProject defaultBundleIdentifier]];
  }
  if (bundle == nil) {
    bundle = mainBundle;
  }

  auto settings = CreateEmptySettings();

  // The command line arguments may not always be complete. If they aren't, attempt to fill in
  // defaults.

  // Flutter ships the ICU data file in the the bundle of the engine. Look for it there.
  if (settings.icu_data_path.size() == 0) {
    NSString* icuDataPath = [engineBundle pathForResource:@"icudtl" ofType:@"dat"];
    if (icuDataPath.length > 0) {
      settings.icu_data_path = icuDataPath.UTF8String;
    }
  }

  if (blink::DartVM::IsRunningPrecompiledCode()) {
    if (hasExplicitBundle) {
      NSString* executablePath = bundle.executablePath;
      if ([[NSFileManager defaultManager] fileExistsAtPath:executablePath]) {
        settings.application_library_path = executablePath.UTF8String;
      }
    }

    // No application bundle specified.  Try a known location from the main bundle's Info.plist.
    if (settings.application_library_path.size() == 0) {
      NSString* libraryName = [mainBundle objectForInfoDictionaryKey:@"FLTLibraryPath"];
      NSString* libraryPath = [mainBundle pathForResource:libraryName ofType:@""];
      if (libraryPath.length > 0) {
        NSString* executablePath = [NSBundle bundleWithPath:libraryPath].executablePath;
        if (executablePath.length > 0) {
          settings.application_library_path = executablePath.UTF8String;
        }
      }
    }

    // In case the application bundle is still not specified, look for the App.framework in the
    // Frameworks directory.
    if (settings.application_library_path.size() == 0) {
      NSString* applicationFrameworkPath = [mainBundle pathForResource:@"Frameworks/App.framework"
                                                                ofType:@""];
      if (applicationFrameworkPath.length > 0) {
        NSString* executablePath =
            [NSBundle bundleWithPath:applicationFrameworkPath].executablePath;
        if (executablePath.length > 0) {
          settings.application_library_path = executablePath.UTF8String;
        }
      }
    }
  }

  // Checks to see if the flutter assets directory is already present.
  if (settings.assets_path.size() == 0) {
    NSString* assetsName = [FlutterDartProject flutterAssetsName:bundle];
    NSString* assetsPath = [bundle pathForResource:assetsName ofType:@""];

    if (assetsPath.length == 0) {
      assetsPath = [mainBundle pathForResource:assetsName ofType:@""];
    }

    if (assetsPath.length == 0) {
      NSLog(@"Failed to find assets path for \"%@\"", assetsName);
    } else {
      settings.assets_path = assetsPath.UTF8String;

      // Check if there is an application kernel snapshot in the assets directory we could
      // potentially use.  Looking for the snapshot makes sense only if we have a VM that can use
      // it.
      if (!blink::DartVM::IsRunningPrecompiledCode()) {
        NSURL* applicationKernelSnapshotURL =
            [NSURL URLWithString:@(kApplicationKernelSnapshotFileName)
                   relativeToURL:[NSURL fileURLWithPath:assetsPath]];
        if ([[NSFileManager defaultManager] fileExistsAtPath:applicationKernelSnapshotURL.path]) {
          settings.application_kernel_asset = applicationKernelSnapshotURL.path.UTF8String;
        } else {
          NSLog(@"Failed to find snapshot: %@", applicationKernelSnapshotURL.path);
        }
      }
    }
  }

  return settings;
}

@implementation FlutterDartProject {
  fml::scoped_nsobject<NSBundle> _precompiledDartBundle;
  blink::Settings _settings;
}

#pragma mark - Override base class designated initializers

- (instancetype)init {
  return [self initWithPrecompiledDartBundle:nil];
}

#pragma mark - Designated initializers

- (instancetype)initWithPrecompiledDartBundle:(NSBundle*)bundle {
  self = [super init];

  if (self) {
    _precompiledDartBundle.reset([bundle retain]);
    _settings = DefaultSettingsForProcess(bundle);
  }

  return self;
}

- (instancetype)initAOTSettingsWithLibPath:(NSString *)libraryPath
                                assetsPath:(NSString *)assetsPath {
  if (!blink::DartVM::IsRunningPrecompiledCode()) {
    NSLog(@"initAOTSettingsWithLibPath only work for aot mode");
    return [self init];
  }

  self = [super init];

  if (self) {
    // TODO: _precompiledDartBundle is not used
    // _precompiledDartBundle.reset(nil);
    _settings = CreateEmptySettings();
    NSString *icuDataPath = GetIcuDataPath();
    if (icuDataPath.length > 0) {
      _settings.icu_data_path = icuDataPath.UTF8String;
    }

    _settings.assets_path = assetsPath.UTF8String;
    _settings.application_library_path = libraryPath.UTF8String;
  }

  return self;
}

- (instancetype)initKernalSettingsWithAssetsPath:(NSString *)assetsPath
                      applicationKernelAssetPath:(NSString *)applicationKernelAssetPath {
  self = [super init];

  if (self) {
    _precompiledDartBundle.reset(nil);

    _settings = CreateEmptySettings();
    NSString *icuDataPath = GetIcuDataPath();
    if (icuDataPath.length > 0) {
      _settings.icu_data_path = icuDataPath.UTF8String;
    }

    _settings.assets_path = assetsPath.UTF8String;
    _settings.application_kernel_asset = applicationKernelAssetPath.UTF8String;
    _settings.use_symbol_snapshot = true;
  }

  return self;
}

- (instancetype)initCoreSnapshotSettingsWithAssetsPath:(NSString *)assetsPath
                                        vmSnapshotPath:(NSString *)vmSnapshotPath
                                   isolateSnapshotPath:(NSString *)isolateSnapshotPath {
  self = [super init];

  if (self) {
    _precompiledDartBundle.reset(nil);

    _settings = CreateEmptySettings();
    NSString *icuDataPath = GetIcuDataPath();
    if (icuDataPath.length > 0) {
      _settings.icu_data_path = icuDataPath.UTF8String;
    }

    _settings.assets_path = assetsPath.UTF8String;
    _settings.vm_snapshot_data_path = vmSnapshotPath.UTF8String;
    _settings.isolate_snapshot_data_path = isolateSnapshotPath.UTF8String;
  }

  return self;
}

#pragma mark - Settings accessors

- (const blink::Settings&)settings {
  return _settings;
}

- (shell::RunConfiguration)runConfiguration {
  return [self runConfigurationForEntrypoint:nil];
}

- (shell::RunConfiguration)runConfigurationForEntrypoint:(NSString*)entrypointOrNil {
  return [self runConfigurationForEntrypoint:entrypointOrNil libraryOrNil:nil];
}

- (shell::RunConfiguration)runConfigurationForEntrypoint:(NSString*)entrypointOrNil
                                            libraryOrNil:(NSString*)dartLibraryOrNil {
  shell::RunConfiguration config = shell::RunConfiguration::InferFromSettings(_settings);
  if (dartLibraryOrNil && entrypointOrNil) {
    config.SetEntrypointAndLibrary(std::string([entrypointOrNil UTF8String]),
                                   std::string([dartLibraryOrNil UTF8String]));

  } else if (entrypointOrNil) {
    config.SetEntrypoint(std::string([entrypointOrNil UTF8String]));
  }
  return config;
}

#pragma mark - Settings setters

- (void)enableObservatory:(BOOL)enableObservatory {
  _settings.enable_observatory = enableObservatory;
}

- (void)enableLogVerbose:(BOOL)enableLogVerbose {
  _settings.verbose_logging = enableLogVerbose;
}

/**
 * Set observatory port.
 */
- (void)setObservatoryPort:(NSUInteger)port {
  _settings.observatory_port = port;
}

/**
 * Set observatory host.
 */
- (void)setObservatoryHost:(NSString *)host {
  if (host == nil) {
    return;
  }
  _settings.observatory_host = host.UTF8String;
}

/**
 * Config ipv6 setting.
 */
- (void)enableIPV6:(BOOL)useIPV6 {
  _settings.ipv6 = useIPV6;
}

#pragma mark - Assets-related utilities

+ (NSString*)flutterAssetsName:(NSBundle*)bundle {
  NSString* flutterAssetsName = [bundle objectForInfoDictionaryKey:@"FLTAssetsPath"];
  if (flutterAssetsName == nil) {
    flutterAssetsName = @"Frameworks/App.framework/flutter_assets";
  }
  return flutterAssetsName;
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset {
  NSString* flutterAssetsName = [FlutterDartProject flutterAssetsName:[NSBundle mainBundle]];
  return [NSString stringWithFormat:@"%@/%@", flutterAssetsName, asset];
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset fromPackage:(NSString*)package {
  return [self lookupKeyForAsset:[NSString stringWithFormat:@"packages/%@/%@", package, asset]];
}

+ (NSString*)defaultBundleIdentifier {
  return @"io.flutter.flutter.app";
}

@end
