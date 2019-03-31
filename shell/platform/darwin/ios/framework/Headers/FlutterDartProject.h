// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLUTTERDARTPROJECT_H_
#define FLUTTER_FLUTTERDARTPROJECT_H_

#import <Foundation/Foundation.h>

#include "FlutterMacros.h"

/**
 * A set of Flutter and Dart assets used by a `FlutterEngine` to initialize execution.
 */
FLUTTER_EXPORT
@interface FlutterDartProject : NSObject

/**
 * Initializes a Flutter Dart project from a bundle.
 */
- (instancetype)initWithPrecompiledDartBundle:(NSBundle*)bundle;


/**
 * Initializes a Flutter Dart project with specific aot settings.
 * Only for AOT mode.
 */
- (instancetype)initAOTSettingsWithLibPath:(NSString *)libraryPath
                                assetsPath:(NSString *)assetsPath;

/**
 * Initializes a Flutter Dart project with specific kernel settings.
 * Only for JIT mode.
 */
- (instancetype)initKernalSettingsWithAssetsPath:(NSString *)assetsPath
                      applicationKernelAssetPath:(NSString *)applicationKernelAssetPath;

/**
 * Initializes a Flutter Dart project with specific core snapshot settings.
 * Only for JIT mode.
 */
- (instancetype)initCoreSnapshotSettingsWithAssetsPath:(NSString *)assetsPath
                                        vmSnapshotPath:(NSString *)vmSnapshotPath
                                   isolateSnapshotPath:(NSString *)isolateSnapshotPath;

/**
 * Unavailable - use `init` instead.
 */
- (instancetype)initFromDefaultSourceForConfiguration FLUTTER_UNAVAILABLE("Use -init instead.");

/**
 * Returns the file name for the given asset.
 * The returned file name can be used to access the asset in the application's main bundle.
 *
 * @param asset The name of the asset. The name can be hierarchical.
 * @return the file name to be used for lookup in the main bundle.
 */
+ (NSString*)lookupKeyForAsset:(NSString*)asset;

/**
 * Returns the file name for the given asset which originates from the specified package.
 * The returned file name can be used to access the asset in the application's main bundle.
 *
 * @param asset The name of the asset. The name can be hierarchical.
 * @param package The name of the package from which the asset originates.
 * @return the file name to be used for lookup in the main bundle.
 */
+ (NSString*)lookupKeyForAsset:(NSString*)asset fromPackage:(NSString*)package;

/**
 * Returns the default identifier for the bundle where we expect to find the Flutter Dart
 * application.
 */
+ (NSString*)defaultBundleIdentifier;

**
 * Config observatory, only for JIT mode.
 */
- (void)enableObservatory:(BOOL)enableObservatory;

/**
 * Config log verbose.
 */
- (void)enableLogVerbose:(BOOL)enableLogVerbose;

/**
 * Set observatory port.
 */
- (void)setObservatoryPort:(NSUInteger)port;

/**
 * Set observatory host.
 */
- (void)setObservatoryHost:(NSString *)host;

/**
 * Config ipv6 setting.
 */
- (void)enableIPV6:(BOOL)useIPV6;

/**
 * Config observatory, only for JIT mode.
 */
- (void)enableObservatory:(BOOL)enableObservatory;

/**
 * Config log verbose.
 */
- (void)enableLogVerbose:(BOOL)enableLogVerbose;

/**
 * Set observatory port.
 */
- (void)setObservatoryPort:(NSUInteger)port;

/**
 * Set observatory host.
 */
- (void)setObservatoryHost:(NSString *)host;

/**
 * Config ipv6 setting.
 */
- (void)enableIPV6:(BOOL)useIPV6;


@end

#endif  // FLUTTER_FLUTTERDARTPROJECT_H_
