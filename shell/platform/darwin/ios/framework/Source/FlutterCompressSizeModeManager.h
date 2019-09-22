//
//  FlutterCompressSizeModeManager.h
//  sources
//
//  Created by 邱鑫玥 on 2019/9/18.
//

// BD ADD: START
#import <Foundation/Foundation.h>
#import <dlfcn.h>
#import "flutter/shell/platform/darwin/ios/framework/Source/FlutterDartProject_Internal.h"

#ifdef __LP64__
typedef struct mach_header_64 flutter_mach_header;
typedef struct section_64 flutter_section;
#define flutter_getsectbynamefromheader getsectbynamefromheader_64
#else
typedef struct mach_header flutter_mach_header;
typedef struct section flutter_section;
#define flutter_getsectbynamefromheader getsectbynamefromheader
#endif

NS_ASSUME_NONNULL_BEGIN

@interface FlutterCompressSizeModeManager : NSObject

@property(nonatomic, assign) BOOL isCompressSizeMode;

+ (instancetype)sharedInstance;

- (NSString*)getDecompressedDataPath:(FlutterCompressSizeModeMonitor _Nullable)completion;

- (void)configAppMH:(flutter_mach_header*)mh appUUIDString:(NSString*)appUUIDString;

- (void)decompressDataAsync:(FlutterCompressSizeModeMonitor _Nullable)completion;

- (void)removePreviousDecompressedData;

@end

NS_ASSUME_NONNULL_END
// END
