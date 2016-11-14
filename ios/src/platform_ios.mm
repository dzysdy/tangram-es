#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <utility>
#import <cstdio>
#import <cstdarg>
#import <fstream>
#import <regex>

#include "platform_ios.h"
#include "log.h"
#include "TGMapViewController.h"

static TGMapViewController* viewController;
static NSBundle* tangramFramework;
NSURLSession* defaultSession;

void init(TGMapViewController* _controller) {

    viewController = _controller;

    /* Setup NSURLSession configuration : cache path and size */
    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSString *cachePath = @"/tile_cache";
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    defaultConfigObject.URLCache = tileCache;
    defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    defaultConfigObject.timeoutIntervalForRequest = 30;
    defaultConfigObject.timeoutIntervalForResource = 60;

    /* create a default NSURLSession using the defaultConfigObject*/
    defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject ];

    // Get handle to tangram framework
    tangramFramework = [NSBundle bundleWithIdentifier:@"com.mapzen.tangramMap"];
}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

}

void requestRender() {

    [viewController renderOnce];

}

void setContinuousRendering(bool _isContinuous) {

    [viewController setContinuous:_isContinuous];

}

bool isContinuousRendering() {

    return [viewController continuous];

}

NSURL* resolvePath(const char* _path) {

    NSString* path = [NSString stringWithUTF8String:_path];

    NSURL* appResources = [[NSBundle mainBundle] resourceURL];
    NSURL* appBundlePath = [NSURL URLWithString:path relativeToURL:appResources];
    NSFileManager* fileManager = [NSFileManager defaultManager];

    bool fileExistsInBundle = [fileManager fileExistsAtPath:[appBundlePath absoluteString]];

    if (!fileExistsInBundle && tangramFramework) {
        NSURL* frameworkResources = [tangramFramework resourceURL];
        frameworkBundlePath = [resources stringByAppendingPathComponent:path];

        if ([fileManager fileExistsAtPath:[frameworkBundlePath absoluteString]]) {
            return frameworkBundlePath;
        }
    }

    return appBundlePath;
}

std::string stringFromFile(const char* _path) {

    NSURL* url = resolvePath(_path);

    NSString* str = [NSString stringWithContentsOfURL:url
                                         usedEncoding:NULL
                                                error:NULL];

    if (str == nil) {
        LOGW("Failed to read file at path: %s (resolved to: %s)\n", _path, [[url absoluteString] UTF8String]);
        return std::string();
    }

    return std::string([str UTF8String]);
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    NSURL* url = resolvePath(_path);
    NSMutableData* data = [NSMutableData dataWithContentsOfURL:url];

    if (data == nil) {
        LOGW("Failed to read file at path: %s (resolved to: %s)\n", _path, [[url absoluteString] UTF8String]);
        _size = 0;
        return nullptr;
    }

    _size = data.length;
    unsigned char* ptr = (unsigned char*)malloc(_size);
    [data getBytes:ptr length:_size];

    return ptr;
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight, const std::string& _face) {
    return "";
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {

        NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;

        int statusCode = [httpResponse statusCode];

        std::vector<char> rawDataVec;

        if (error != nil) {

            LOGE("Response \"%s\" with error \"%s\".", response, [error.localizedDescription UTF8String]);

        } else if (statusCode < 200 || statusCode >= 300) {

            LOGE("Unsuccessful status code %d: \"%s\" from: %s",
                statusCode,
                [[NSHTTPURLResponse localizedStringForStatusCode: statusCode] UTF8String],
                [response.URL.absoluteString UTF8String]);
            _callback(std::move(rawDataVec));

        } else {

            int dataLength = [data length];
            rawDataVec.resize(dataLength);
            memcpy(rawDataVec.data(), (char *)[data bytes], dataLength);
            _callback(std::move(rawDataVec));

        }

    };

    NSURLSessionDataTask* dataTask = [defaultSession dataTaskWithURL:[NSURL URLWithString:nsUrl]
                                                    completionHandler:handler];

    [dataTask resume];

    return true;

}

void cancelUrlRequest(const std::string& _url) {

    NSString* nsUrl = [NSString stringWithUTF8String:_url.c_str()];

    [defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* task in dataTasks) {
            if([[task originalRequest].URL.absoluteString isEqualToString:nsUrl]) {
                [task cancel];
                break;
            }
        }
    }];

}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}

#endif //PLATFORM_IOS
