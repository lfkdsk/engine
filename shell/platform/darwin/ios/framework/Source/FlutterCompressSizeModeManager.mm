//
//  FlutterCompressSizeModeManager.m
//  sources
//
//  Created by 邱鑫玥 on 2019/9/18.
//

// BD ADD: START
#import "FlutterCompressSizeModeManager.h"
#import <dlfcn.h>
#import <mach-o/dyld.h>
#import <mach-o/getsect.h>
#import "FlutterZipArchive.h"
#import "NSData+FlutterGzip.h"

static const char* kSegmentName = "__BD_DATA";
static const char* kIsolateDataSectionName = "__isolate_data";
static const char* kVMDataSectionName = "__vm_data";
static NSString* const kDecompressedDataCacheDirectory = @"com.bytedance.flutter/decompressed_data";
static NSString* const kCompressedAssetsFilePath =
    @"Frameworks/App.framework/flutter_compress_assets.zip";
static NSErrorDomain const kFlutterCompressSizeModeErrorDomain = @"FlutterCompressSizeModeError";
static NSString* kFlutterAssets;

static uintptr_t FirstLoadCommandPtr(const struct mach_header* mh) {
  switch (mh->magic) {
    case MH_MAGIC:
    case MH_CIGAM:
      return (uintptr_t)(mh + 1);
    case MH_MAGIC_64:
    case MH_CIGAM_64:
      return (uintptr_t)(((struct mach_header_64*)mh) + 1);
    default:
      return 0;
  }
}

static const uint8_t* ImageUUID(const struct mach_header* mh) {
  if (mh != NULL) {
    uintptr_t cmdPtr = FirstLoadCommandPtr(mh);
    if (cmdPtr != 0) {
      for (uint32_t iCmd = 0; iCmd < mh->ncmds; iCmd++) {
        const struct load_command* loadCmd = (struct load_command*)cmdPtr;
        if (loadCmd->cmd == LC_UUID) {
          struct uuid_command* uuidCmd = (struct uuid_command*)cmdPtr;
          return uuidCmd->uuid;
        }
        cmdPtr += loadCmd->cmdsize;
      }
    }
  }

  return NULL;
}

static void ImageAdded(const struct mach_header* mh, intptr_t slide) {
  const flutter_section* isolateDataSection = flutter_getsectbynamefromheader(
      (flutter_mach_header*)mh, kSegmentName, kIsolateDataSectionName);
  const flutter_section* vmDataSection =
      flutter_getsectbynamefromheader((flutter_mach_header*)mh, kSegmentName, kVMDataSectionName);

  if (isolateDataSection == NULL && vmDataSection == NULL) {
    return;
  }
  const uint8_t* uuidPtr = ImageUUID(mh);
  NSString* uuidString = nil;
  if (uuidPtr != NULL) {
    NSUUID* uuid = [[[NSUUID alloc] initWithUUIDBytes:uuidPtr] autorelease];
    uuidString = [uuid UUIDString];
  }
  if (uuidString.length == 0) {
    uuidString = [NSString stringWithFormat:@"%f", [[NSDate date] timeIntervalSince1970]];
  }

  [[FlutterCompressSizeModeManager sharedInstance] configAppMH:(flutter_mach_header*)mh
                                                 appUUIDString:uuidString];
}

@interface FlutterCompressSizeModeManager ()

@property(nonatomic, assign) flutter_mach_header* appMachHeader;
@property(nonatomic, copy) NSString* appUUIDString;
@property(nonatomic, copy) NSString* cacheDirectoryForDecompressedData;
@property(nonatomic, copy) NSString* cacheDirectoryForCurrentUUID;
@property(nonatomic, copy) NSString* isolateDataPath;
@property(nonatomic, copy) NSString* vmDataPath;
@property(nonatomic, copy) NSString* icudtlDataPath;
@property(nonatomic, copy) NSString* flutterAssetsPath;

@end

@implementation FlutterCompressSizeModeManager

+ (void)initialize {
  kFlutterAssets = [[FlutterDartProject flutterAssetsPath] copy];
  _dyld_register_func_for_add_image(&ImageAdded);
}

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static FlutterCompressSizeModeManager* instance;
  dispatch_once(&onceToken, ^{
    instance = [[FlutterCompressSizeModeManager alloc] init];
  });
  return instance;
}

- (void)dealloc {
  [_appUUIDString release];
  [_cacheDirectoryForDecompressedData release];
  [_cacheDirectoryForCurrentUUID release];
  [_isolateDataPath release];
  [_vmDataPath release];
  [_icudtlDataPath release];
  [_flutterAssetsPath release];
  [super dealloc];
}

- (instancetype)init {
  if (self = [super init]) {
    NSArray* paths =
        NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* path = [paths objectAtIndex:0];
    self.cacheDirectoryForDecompressedData =
        [[path stringByAppendingPathComponent:kDecompressedDataCacheDirectory] copy];
    self.isCompressSizeMode = NO;
  }

  return self;
}

- (void)configAppMH:(flutter_mach_header*)mh appUUIDString:(NSString*)appUUIDString {
  self.appMachHeader = mh;
  self.appUUIDString = [appUUIDString copy];
  self.isCompressSizeMode = YES;

  self.cacheDirectoryForCurrentUUID = [[self.cacheDirectoryForDecompressedData
      stringByAppendingPathComponent:self.appUUIDString] copy];

  self.isolateDataPath = [[self.cacheDirectoryForCurrentUUID
      stringByAppendingPathComponent:FlutterIsolateDataFileName] copy];
  self.vmDataPath = [[self.cacheDirectoryForCurrentUUID
      stringByAppendingPathComponent:FlutterVMDataFileName] copy];
  self.icudtlDataPath = [[self.cacheDirectoryForCurrentUUID
      stringByAppendingPathComponent:FlutterIcudtlDataFileName] copy];
  self.flutterAssetsPath =
      [[self.cacheDirectoryForCurrentUUID stringByAppendingPathComponent:kFlutterAssets] copy];
}

- (NSString*)getDecompressedDataPath:(FlutterCompressSizeModeMonitor)completion
                               error:(NSError**)error {
  return [self getDecompressedDataPath:completion isAsync:NO error:error];
}

- (NSString*)getDecompressedDataPath:(FlutterCompressSizeModeMonitor)completion
                             isAsync:(BOOL)isAsync
                               error:(NSError**)error {
  NSError* internalError = nil;
  BOOL succeeded = YES;
  BOOL needDecompress = NO;

  [self removePreviousDecompressedData];

  if ([self needDecompressData]) {
    needDecompress = YES;
    succeeded = [self decompressData:&internalError];
  }

  [internalError retain];
  dispatch_async(dispatch_get_main_queue(), ^{
    [internalError autorelease];
    if (completion) {
      completion(needDecompress, isAsync, succeeded, internalError);
    }
  });

  if (error) {
    *error = internalError;
  }

  return succeeded ? self.cacheDirectoryForCurrentUUID : nil;
}

- (void)decompressDataAsync:(FlutterCompressSizeModeMonitor)completion {
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    [self getDecompressedDataPath:completion isAsync:YES error:nil];
  });
}

- (void)removePreviousDecompressedData {
  // 存在已经解压缩过的数据，则清除历史数据
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSString* cacheDirectoryForDecompressedData = self.cacheDirectoryForDecompressedData;
  BOOL isDirectory = false;
  BOOL succeeded = [fileManager fileExistsAtPath:cacheDirectoryForDecompressedData
                                     isDirectory:&isDirectory];
  if (!succeeded || !isDirectory) {
    return;
  }
  NSArray<NSString*>* files =
      [fileManager contentsOfDirectoryAtPath:cacheDirectoryForDecompressedData error:nil];
  for (NSString* fileName in files) {
    if (!self.appUUIDString || ![fileName isEqualToString:self.appUUIDString]) {
      NSString* obsoleteFilePath =
          [cacheDirectoryForDecompressedData stringByAppendingPathComponent:fileName];
      [fileManager removeItemAtPath:obsoleteFilePath error:nil];
    }
  }
}

- (BOOL)decompressData:(NSError**)error {
  NSFileManager* fileManager = [NSFileManager defaultManager];

  // 根据App.framework的UUID创建解压缩数据的目录
  BOOL succeeded = [fileManager createDirectoryAtPath:self.cacheDirectoryForCurrentUUID
                          withIntermediateDirectories:YES
                                           attributes:nil
                                                error:error];
  if (!succeeded) {
    return NO;
  }

  // 解压缩isolate_data
  succeeded = [self decompressIsolateData:error];
  if (!succeeded) {
    return NO;
  }

  // 解压缩vm_data
  succeeded = [self decompressVMData:error];
  if (!succeeded) {
    return NO;
  }

  // 解压缩assets
  succeeded = [self decompressAssetsData:error];
  if (!succeeded) {
    return NO;
  }

  return YES;
}

- (BOOL)decompressIsolateData:(NSError**)error {
  unsigned long size = 0;
  uint8_t* sectiondata =
      getsectiondata(self.appMachHeader, kSegmentName, kIsolateDataSectionName, &size);
  NSData* zippedData = [[[NSData alloc] initWithBytes:sectiondata length:size] autorelease];
  NSData* gunzippedData = [zippedData flutter_gunzippedData];
  BOOL succeeded = [[NSFileManager defaultManager] createFileAtPath:self.isolateDataPath
                                                           contents:gunzippedData
                                                         attributes:nil];
  if (!succeeded) {
    if (error) {
      *error = [NSError
          errorWithDomain:kFlutterCompressSizeModeErrorDomain
                     code:-1001
                 userInfo:@{NSLocalizedDescriptionKey : @"解压缩并存储isolate data数据失败"}];
    }
    return NO;
  } else {
    return YES;
  }
}

- (BOOL)decompressVMData:(NSError**)error {
  unsigned long size = 0;
  uint8_t* sectiondata =
      getsectiondata(self.appMachHeader, kSegmentName, kVMDataSectionName, &size);
  NSData* zippedData = [[[NSData alloc] initWithBytes:sectiondata length:size] autorelease];
  NSData* gunzippedData = [zippedData flutter_gunzippedData];
  BOOL succeeded = [[NSFileManager defaultManager] createFileAtPath:self.vmDataPath
                                                           contents:gunzippedData
                                                         attributes:nil];
  if (!succeeded) {
    if (error) {
      *error =
          [NSError errorWithDomain:kFlutterCompressSizeModeErrorDomain
                              code:-1002
                          userInfo:@{NSLocalizedDescriptionKey : @"解压缩并存储vm data数据失败"}];
    }
    return NO;
  } else {
    return YES;
  }
}

- (BOOL)decompressAssetsData:(NSError**)error {
  return [FlutterZipArchive
      unzipFileAtPath:[[[NSBundle mainBundle] bundlePath]
                          stringByAppendingPathComponent:kCompressedAssetsFilePath]
        toDestination:self.cacheDirectoryForCurrentUUID
            overwrite:YES
             password:nil
                error:error];
}

- (BOOL)needDecompressData {
  if (self.isCompressSizeMode) {
    return !([self isIsolateDataValid] && [self isVMDataValid] && [self isIcudtlDataValid] &&
             [self isFlutterAssetsDataValid]);
  } else {
    return NO;
  }
}

- (BOOL)isIsolateDataValid {
  return [[NSFileManager defaultManager] fileExistsAtPath:self.isolateDataPath];
}

- (BOOL)isVMDataValid {
  return [[NSFileManager defaultManager] fileExistsAtPath:self.vmDataPath];
}

- (BOOL)isIcudtlDataValid {
  return [[NSFileManager defaultManager] fileExistsAtPath:self.icudtlDataPath];
}

- (BOOL)isFlutterAssetsDataValid {
  return [[NSFileManager defaultManager] fileExistsAtPath:self.flutterAssetsPath];
}

@end
// END
