// BD ADD: START
//
//  FlutterBinaryMessengerProvider.h
//  sources
//
//  Created by 邱鑫玥 on 2019/8/20.
//

#import "flutter/fml/memory/weak_ptr.h"
#import "flutter/shell/platform/darwin/ios/framework/Headers/FlutterBinaryMessenger.h"

NS_ASSUME_NONNULL_BEGIN

@protocol FlutterBinaryMessengerProvider <NSObject>

- (fml::WeakPtr<NSObject<FlutterBinaryMessenger>>)getWeakBinaryMessengerPtr;

@end

NS_ASSUME_NONNULL_END
// END
