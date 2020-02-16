// BD ADD: START
//
//  FlutterBinaryMessengerProvider.h
//  sources
//
//  Created by 邱鑫玥 on 2019/8/20.
//

#ifndef FLUTTER_SHELL_PLATFORM_DARWIN_COMMON_FRAMEWORK_SOURCE_FLUTTERBINARYMESSENGERPROVIDER_H_
#define FLUTTER_SHELL_PLATFORM_DARWIN_COMMON_FRAMEWORK_SOURCE_FLUTTERBINARYMESSENGERPROVIDER_H_

#import "flutter/fml/memory/weak_ptr.h"
#import "flutter/shell/platform/darwin/common/framework/Headers/FlutterBinaryMessenger.h"

NS_ASSUME_NONNULL_BEGIN

@protocol FlutterBinaryMessengerProvider <NSObject>

- (fml::WeakPtr<NSObject<FlutterBinaryMessenger>>)getWeakBinaryMessengerPtr;

@end

NS_ASSUME_NONNULL_END

#endif  // FLUTTER_SHELL_PLATFORM_DARWIN_COMMON_FRAMEWORK_SOURCE_FLUTTERBINARYMESSENGERPROVIDER_H_
// END
