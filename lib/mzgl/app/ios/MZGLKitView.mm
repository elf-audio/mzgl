//
//  MZGLES3Renderer.m
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLKitView.h"
#include "App.h"
#include "EventDispatcher.h"
#include "mzgl/util/log.h"
#include "Vbo.h"
#include "PluginEditor.h"
using namespace std;
API_AVAILABLE(ios(11)) @interface MZGLKitView (DragDropExtensions)<UIDropInteractionDelegate>
@end


@implementation MZGLKitView {

	std::shared_ptr<App> app;
	std::shared_ptr<EventDispatcher> eventDispatcher;
	
	NSMutableDictionary	* activeTouches;
	bool firstFrame;
	NSString *urlToOpen;
	bool appIsSetup;
}

- (std::shared_ptr<App>) getApp {
	return app;
}

-(void) dealloc {
	NSLog(@"Tearing down MZGLKitView");
	

}

- (id) initWithApp: (std::shared_ptr<App>) _app {
	self = [super init];
	if(self!=nil) {
		app = _app;
		appIsSetup = false;
		eventDispatcher = std::make_shared<EventDispatcher>(app);
		 
		urlToOpen = nil;

		activeTouches = [[NSMutableDictionary alloc] init];
		self.multipleTouchEnabled = YES;
		self.userInteractionEnabled = YES;
		if(@available(iOS 11, *)) {
			[self addInteraction:[[UIDropInteraction alloc] initWithDelegate:self]];
		}
		firstFrame = true;
	}
	return self;
}




- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent *)event {
//    GLKViewController *ctrl = (__bridge GLKViewController *)app->viewController;
//    NSLog(@"%d", ctrl.paused);
//    if(ctrl.paused) ctrl.paused = NO;
    
    for(UITouch *touch in touches) {
		int touchIndex = 0;
		while([[activeTouches allValues] containsObject:[NSNumber numberWithInt:touchIndex]]){
			touchIndex++;
		}
		
		[activeTouches setObject:[NSNumber numberWithInt:touchIndex] forKey:[NSValue valueWithNonretainedObject:touch]];
		//[activeTouches setObject:[NSNumber numberWithInt:touchIndex] forKey:touch];
		
		CGPoint touchPoint = [touch locationInView:self];
		
		touchPoint.x *= app->g.pixelScale; // this has to be done because retina still returns points in 320x240 but with high percision
		touchPoint.y *= app->g.pixelScale;
		eventDispatcher->touchDown(touchPoint.x, touchPoint.y, touchIndex);
	}
}


- (void) touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {

	for(UITouch *touch in touches){
		int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];
		
		CGPoint touchPoint = [touch locationInView:self];
		
		// this has to be done because retina still returns points in 320x240 but with high percision
		touchPoint.x *= app->g.pixelScale;
		touchPoint.y *= app->g.pixelScale;
		eventDispatcher->touchMoved(touchPoint.x, touchPoint.y, touchIndex);
	}
}

- (void) touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    
	for(UITouch *touch in touches){
		int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];
		
		[activeTouches removeObjectForKey:[NSValue valueWithNonretainedObject:touch]];
		
		CGPoint touchPoint = [touch locationInView:self];
		
		// this has to be done because retina still returns points in 320x240 but with high percision
		touchPoint.x *= app->g.pixelScale;
		touchPoint.y *= app->g.pixelScale;
		eventDispatcher->touchUp(touchPoint.x, touchPoint.y, touchIndex);

	}
}

- (void) touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
	for(UITouch *touch in touches){
		//int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];
		
		CGPoint touchPoint = [touch locationInView:self];
		// this has to be done because retina still returns points in 320x240 but with high percision
		touchPoint.x *= app->g.pixelScale;
		touchPoint.y *= app->g.pixelScale;

	}
	
	[self touchesEnded:touches withEvent:event];
}
API_AVAILABLE(ios(13.4))
int uikeyToMz(UIKey *key) {
	if(key.keyCode==UIKeyboardHIDUsageKeyboardDeleteOrBackspace) {
		return MZ_KEY_DELETE;
	}
	NSString *ch = key.charactersIgnoringModifiers;
	if([ch isEqualToString:UIKeyInputUpArrow]) {
		return MZ_KEY_UP;
	} else if([ch isEqualToString:UIKeyInputDownArrow]) {
		return MZ_KEY_DOWN;
	} else if([ch isEqualToString:UIKeyInputLeftArrow]) {
		return MZ_KEY_LEFT;
	} else if([ch isEqualToString:UIKeyInputRightArrow]) {
		return MZ_KEY_RIGHT;
	}
	string keyStr = [key.charactersIgnoringModifiers UTF8String];
	if(keyStr.size()==1) {
		if(keyStr[0]==9) {
			if(key.modifierFlags==UIKeyModifierShift) {
				return MZ_KEY_SHIFT_TAB;
			} else {
				return MZ_KEY_TAB;
			}
		}
		
		keyStr = [key.characters UTF8String];
		if(keyStr.size()==1) {
			return keyStr[0];
		} else {
			return -1;
		}
	} else {
		return -1;
		
	}
}


- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
	bool handled = false;
	if (@available(iOS 13.4, *)) {
		for(UIPress *press in presses) {
			UIKey *key = press.key;
			if(key!=nil) {
				int k = uikeyToMz(key);
				if(k!=-1) {
//					Log::d() << "KeyCode: " << k << " UIKey.keycode = " << key.keyCode;
					eventDispatcher->keyDown(k);
				} else {
					Log::d() << "Don't know how to handle key";
				}
				handled = true;
			} else {
				Log::d() << "Key was null";
			}
		}
		
	}
	if(!handled) {
		[super pressesBegan:presses withEvent:event];
	}
}


- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
	
	bool handled = false;
	
	if (@available(iOS 13.4, *)) {

		for(UIPress *press in presses) {
			UIKey *key = press.key;
			
			if(key!=nil) {
				int k = uikeyToMz(key);
				if(k!=-1) {
					eventDispatcher->keyUp(k);
				}
				handled = true;
			}
		}
	}
	if(!handled) {
		[super pressesBegan:presses withEvent:event];
	}
}

- (void)drawRect:(CGRect)rect {
	if(firstFrame) {
		app->g.pixelScale = [[UIScreen mainScreen] nativeScale];
		//NSRect r = [[UIScreen mainScreen] nativeBounds];

		initMZGL(app);

		app->g.width = rect.size.width * app->g.pixelScale;
		app->g.height = rect.size.height * app->g.pixelScale;
		
		if(app->g.width==0 || app->g.height==0) {
			app->g.width = 100;
			app->g.height = 100;
			printf("ERROR: WIDTH OR HEIGHT is ZERO\n");
		}
		
		eventDispatcher->setup();
		appIsSetup = true;
		// if the app was opened by a url,
		// we need to defer sending that to
		// the event dispatcher until the app
		// is ready (i.e. after setup())
		if(urlToOpen!=nil) {
			string url = [urlToOpen UTF8String];
			eventDispatcher->openUrl(ScopedUrl::createWithDeleter(url));
		}
		firstFrame = false;
	}
	eventDispatcher->runFrame();
}


-(std::shared_ptr<EventDispatcher>) getEventDispatcher {
	return eventDispatcher;
}

-(void) openURLWhenLoadedAndDeleteFile: (NSString*) url {
	urlToOpen = url;
}

- (BOOL) handleNormalOpen: (NSURL*) url {
	NSLog(@"Standard open");
	bool mustStopAccessing = false;
	NSString *path = url.path;
	if(![[NSFileManager defaultManager] isReadableFileAtPath:path]) {
		NSLog(@"Warning: can't open this file");
		[url startAccessingSecurityScopedResource];
		mustStopAccessing = true;
	}
	
	fs::path src([path UTF8String]);
	fs::path dst = docsPath() / src.filename();
	try {
		fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
	} catch(const fs::filesystem_error &err) {
		Log::e() << "Got exception trying to copy the file " << err.what();
	}
	
	NSString *destination = [NSString stringWithUTF8String: dst.c_str()];
	
//	NSString *destination = moveFileToDocsDir(path, /* copying: */ true);
	if(![[NSFileManager defaultManager] fileExistsAtPath:destination]) {
		NSLog(@"File does not exist at path");
	}
	
	
	if(mustStopAccessing) {
		[url stopAccessingSecurityScopedResource];
	}

	if(destination==nil) {
		NSLog(@"temp file copy error %@", destination);
		return false;
	}

	if(appIsSetup) {
		
		std::string url = [destination UTF8String];
		return eventDispatcher->openUrl(ScopedUrl::createWithDeleter(url));

	} else {
		[self openURLWhenLoadedAndDeleteFile: destination];
		Log::e() << "Event dispatcher not ready";
		return true;
	}
}
@end
