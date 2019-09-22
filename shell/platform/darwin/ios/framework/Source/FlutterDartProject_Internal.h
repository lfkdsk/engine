// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_PLATFORM_IOS_FRAMEWORK_SOURCE_FLUTTERDARTPROJECT_INTERNAL_H_
#define SHELL_PLATFORM_IOS_FRAMEWORK_SOURCE_FLUTTERDARTPROJECT_INTERNAL_H_

#include "flutter/common/settings.h"
#include "flutter/shell/common/engine.h"
#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterDartProject.h"

// BD ADD: START
FOUNDATION_EXTERN NSString* const FlutterIsolateDataFileName;
FOUNDATION_EXTERN NSString* const FlutterVMDataFileName;
FOUNDATION_EXTERN NSString* const FlutterIcudtlDataFileName;
// END

@interface FlutterDartProject ()

// BD ADD:
@property(nonatomic, assign) BOOL isValid;

- (const flutter::Settings&)settings;

- (flutter::RunConfiguration)runConfiguration;
- (flutter::RunConfiguration)runConfigurationForEntrypoint:(NSString*)entrypointOrNil;
- (flutter::RunConfiguration)runConfigurationForEntrypoint:(NSString*)entrypointOrNil
                                              libraryOrNil:(NSString*)dartLibraryOrNil;

+ (NSString*)flutterAssetsName:(NSBundle*)bundle;

// BD ADD:
+ (NSString*)flutterAssetsPath;

@end

#endif  // SHELL_PLATFORM_IOS_FRAMEWORK_SOURCE_FLUTTERDARTPROJECT_INTERNAL_H_
