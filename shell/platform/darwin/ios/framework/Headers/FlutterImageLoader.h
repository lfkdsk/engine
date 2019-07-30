#ifndef FLUTTER_FLUTTERIMAGELOADER_H_
#define FLUTTER_FLUTTERIMAGELOADER_H_

#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

#include "FlutterMacros.h"

NS_ASSUME_NONNULL_BEGIN

FLUTTER_EXPORT
@protocol FlutterImageLoader <NSObject>
typedef void(^callback)(CVPixelBufferRef, void *, int, int, int, int);
- (void)loadImage:(NSString*)url complete:(callback)complete;
@end

FLUTTER_EXPORT
@protocol FlutterImageLoaderRegistry <NSObject>
- (void)registerImageLoader:(NSObject<FlutterImageLoader>*)imageLoader;
@end

NS_ASSUME_NONNULL_END

#endif  // FLUTTER_FLUTTERIMAGELOADER_H_

