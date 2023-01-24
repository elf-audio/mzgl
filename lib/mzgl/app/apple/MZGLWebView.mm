//
//  EventsView.m
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#import "MZGLWebView.h"

#include <string>
#include "EventDispatcher.h"
#include "log.h"
#include "filesystem.h"
@implementation MZGLWebView {
    EventDispatcher *eventDispatcher;
}
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr andUrl: (NSString*) url {
    eventDispatcher = (EventDispatcher*)evtDispatcherPtr;
    
    ((WebViewApp*)eventDispatcher->app)->callJS = [self](const std::string &s) {
        printf("callJS(%s)\n", s.c_str());
        [self evaluateJavaScript:[NSString stringWithUTF8String:s.c_str()] completionHandler:nil];
    };

    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    [config.userContentController addScriptMessageHandler:self name:@"updateHandler"];

    
    
    [config.preferences setValue: [NSNumber numberWithBool: YES] forKey: @"fullScreenEnabled"];
        [config.preferences setValue: [NSNumber numberWithBool: YES] forKey: @"DOMPasteAllowed"];
        [config.preferences setValue: [NSNumber numberWithBool: YES] forKey: @"javaScriptCanAccessClipboard"];
#ifdef DEBUG
//    blah
        [config.preferences setValue: [NSNumber numberWithBool: YES] forKey: @"developerExtrasEnabled"];
#endif
    
    
	self = [super initWithFrame:frame configuration:config];
	std::string customUrl = [url UTF8String];
	if(self!=nil) {
		
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
		self.UIDelegate = self;
		
		if(customUrl!="" && customUrl.find("http")!=-1) {
			NSURL *url = [NSURL URLWithString:[NSString stringWithFormat:@"%s", customUrl.c_str()]];
			NSURLRequest *request = [NSURLRequest requestWithURL:url];
			[self loadRequest:request];
		} else {
			
		
			NSURL *baseURL = [[NSBundle mainBundle] resourceURL];
			baseURL = [baseURL URLByAppendingPathComponent:@"www"];
			NSURL *url = [baseURL URLByAppendingPathComponent:@"index.html"];
			
			
			if(customUrl!="") {
				NSLog(@"Got custom URL!!");
				auto p = fs::path(customUrl);
                if(!fs::exists(p)) {
                    NSLog(@"Path does not exist! %s", p.string().c_str());
                }
				baseURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String: p.parent_path().string().c_str()]];
				url = [baseURL URLByAppendingPathComponent:[NSString stringWithUTF8String:p.filename().string().c_str()]];
			}
			NSLog(@"%@", url);
			
			[self loadFileURL:url allowingReadAccessToURL:baseURL];
		}
	}
	return self;
}
- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler
{
 
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:message];
    [alert setAlertStyle:NSAlertStyleWarning];

    [alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        completionHandler();
    }];
}

- (void)windowDidResize:(NSNotification *)notification {
	Log::d() << "windowDidResize";
	NSWindow *window = notification.object;
	
	auto w = window.contentLayoutRect.size.width;
	auto h = window.contentLayoutRect.size.height;
	
	Log::d() << w << " " << h;
	
	NSRect f = self.frame;
	f.size = CGSizeMake(w, h);;
	self.frame = f;
}

- (BOOL) acceptsFirstResponder {
	return YES;
}

- (BOOL) becomeFirstResponder {
	return YES;
}

- (BOOL) isOpaque {
	return YES;
}


- (void) shutdown {
//	[super shutdown];
//	eventDispatcher->exit();
}


- (void)userContentController:(nonnull WKUserContentController *)userContentController didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
    NSDictionary *dict = message.body;
    std::string key = [dict[@"key"] UTF8String];
    std::string value = [dict[@"value"] UTF8String];
    eventDispatcher->receivedJSMessage(key, value);
}


@end
