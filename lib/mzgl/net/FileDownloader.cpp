#include "FileDownloader.h"

#ifdef __ANDROID__

#include "androidUtil.h"

FileDownloader::~FileDownloader() {
    cancel();
}

void FileDownloader::download(const std::string &url,
                              const fs::path &downloadLocation,
                              const FileDownloaderCallbacks &callbacks) {
    downloadURL = url;
    androidDownloadFile(url, downloadLocation.string(), callbacks);
}

void FileDownloader::cancel() {
    androidCancelFileDownload(downloadURL);
}

#endif

#ifdef __APPLE__

#	include "log.h"
#	import <Foundation/Foundation.h>
#	import <Foundation/NSURLSession.h>

@interface FoundationFileDownloader : NSObject <NSURLSessionDownloadDelegate>
- (void)download;
- (void)cancel;
@end

@implementation FoundationFileDownloader {
    NSURL *_sourceURL;
    NSString *_destinationPath;
    FileDownloaderCallbacks _callbacks;
    NSURLSessionConfiguration *_configuration;
    NSURLSession *_session;
    NSURLSessionDownloadTask *_downloadTask;
    BOOL _failed;
}

- (id)initWithURL:(NSURL *)url
    toDestination:(NSString *)destinationPath
    withCallbacks:(const FileDownloaderCallbacks &)callbacks {
    self = [super init];
    if (self != nil) {
        _sourceURL		 = url;
        _destinationPath = destinationPath;
        _callbacks		 = callbacks;
        _configuration	 = [NSURLSessionConfiguration defaultSessionConfiguration];
        _session		 = [NSURLSession sessionWithConfiguration:_configuration delegate:self delegateQueue:nil];
        _downloadTask	 = [_session downloadTaskWithURL:url];
    }

    return self;
}

- (void)download {
    if (_callbacks.downloadStarting != nullptr) {
        _callbacks.downloadStarting();
    }
    [_downloadTask resume];
}

- (void)cancel {
    if (_downloadTask != nil) {
        [_downloadTask cancel];
        if (_callbacks.downloadCancelled != nullptr) {
            _callbacks.downloadCancelled();
        }
    }
}

- (void)URLSession:(NSURLSession *)session
                 downloadTask:(NSURLSessionDownloadTask *)downloadTask
                 didWriteData:(int64_t)bytesWritten
            totalBytesWritten:(int64_t)totalBytesWritten
    totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
    if (totalBytesExpectedToWrite > 0 && _callbacks.downloadProgressChanged != nullptr) {
        _callbacks.downloadProgressChanged(static_cast<float>(static_cast<double>(totalBytesWritten)
                                                              / static_cast<double>(totalBytesExpectedToWrite)));
    }
}

// First bytes of the body for the log, so a rejected download says what the
// server actually sent ("forbidden", an HTML error page, ...).
static std::string bodySnippet(NSURL *location) {
    NSData *data = [NSData dataWithContentsOfURL:location];
    if (data == nil) return "";
    const auto *bytes = static_cast<const char *>(data.bytes);
    std::string out;
    for (NSUInteger i = 0; i < data.length && i < 100; i++) {
        const char c = bytes[i];
        out += (c >= 32 && c < 127) ? c : '.';
    }
    if (data.length > 100) out += "...";
    return out;
}

- (void)URLSession:(NSURLSession *)session
                 downloadTask:(NSURLSessionDownloadTask *)downloadTask
    didFinishDownloadingToURL:(NSURL *)location {
    // NSURLSession only reports transport failures through
    // didCompleteWithError. An HTTP 403, 404 or 500 arrives here as a
    // "finished" download whose body is the server's error message, so check
    // the status before treating the temp file as the thing we asked for.
    NSInteger status = 0;
    if ([downloadTask.response isKindOfClass:[NSHTTPURLResponse class]]) {
        status = ((NSHTTPURLResponse *) downloadTask.response).statusCode;
    }
    if (status != 0 && (status < 200 || status >= 300)) {
        Log::e() << "Download of " << [[_sourceURL absoluteString] UTF8String] << " failed: HTTP " << status
                 << " - " << bodySnippet(location);
        _failed = YES;
        if (_callbacks.downloadFailed != nullptr) {
            _callbacks.downloadFailed("The server returned an error (HTTP " + std::to_string(status) + ")");
        }
        return;
    }

    NSError *error = nil;
    [[NSFileManager defaultManager] removeItemAtPath:_destinationPath error:nil];

    if ([[NSFileManager defaultManager] moveItemAtPath:[location path] toPath:_destinationPath error:&error]) {
        if (_callbacks.downloadCompleted) {
            _callbacks.downloadCompleted(fs::path {[_destinationPath UTF8String]});
        }
    } else {
        if (error != nil) {
            Log::e() << "Download of " << [[_sourceURL absoluteString] UTF8String] << " to " <<
                [_destinationPath UTF8String] << " failed because " << [error.localizedDescription UTF8String];
            Log::e() << "Failed to move from " << [[location path] UTF8String];
            if (_callbacks.downloadFailed != nullptr) {
                _callbacks.downloadFailed([error.localizedDescription UTF8String]);
            }
        }
    }
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    // A status rejection above already reported this download.
    if (error != nil && !_failed) {
        Log::e() << "Download of " << [[_sourceURL absoluteString] UTF8String] << " to "
                 << [_destinationPath UTF8String] << " failed because " << [error.localizedDescription UTF8String];
        if (_callbacks.downloadFailed != nullptr) {
            _callbacks.downloadFailed([error.localizedDescription UTF8String]);
        }
    }
}

@end

FileDownloader::~FileDownloader() {
    cancel();

    if (platformHandle != nullptr) {
        CFBridgingRelease(platformHandle);
        platformHandle = nullptr;
    }
}

void FileDownloader::download(const std::string &url,
                              const fs::path &downloadLocation,
                              const FileDownloaderCallbacks &callbacks) {
    FoundationFileDownloader *downloader = [[FoundationFileDownloader alloc]
          initWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]
        toDestination:[NSString stringWithUTF8String:downloadLocation.string().c_str()]
        withCallbacks:callbacks];

    platformHandle = (void *) CFBridgingRetain(downloader);
    [downloader download];
}

void FileDownloader::cancel() {
    if (platformHandle == nullptr) {
        return;
    }

    FoundationFileDownloader *downloader = (__bridge FoundationFileDownloader *) (platformHandle);
    [downloader cancel];
}

#endif