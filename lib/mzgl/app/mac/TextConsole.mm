//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "TextConsole.h"
#include <AppKit/AppKit.h>
using namespace std;

void TextConsole::show() {
	//CGRect f  = [[[NSApplication sharedApplication] keyWindow] frame];
	//printf("HERE IS WIN: %f %f %f %f\n", f.origin.x, f.origin.y, f.size.width, f.size.height);

	NSRect windowRect = NSMakeRect(0, 0, 500, 200);
	NSWindow *window  = [[NSWindow alloc]
		 initWithContentRect:windowRect
				   styleMask:NSWindowStyleMaskResizable | NSTitledWindowMask | NSWindowStyleMaskUtilityWindow

					 backing:NSBackingStoreBuffered
					   defer:NO];
	[window setLevel:NSFloatingWindowLevel];
	//window.acceptsMouseMovedEvents = YES;

	[window cascadeTopLeftFromPoint:NSMakePoint(500, 20)];
	[window setTitle:@"Console"];

	//		view = [[MZGLView alloc] initWithFrame:windowRect];
	//		window.delegate = view;
	//		[[window contentView] addSubview:view];
	//[window makeKeyAndOrderFront:nil];
	//		[window makeMainWindow];

	NSScrollView *scrollview = [[NSScrollView alloc] initWithFrame:[[window contentView] frame]];

	NSSize contentSize = [scrollview contentSize];

	[scrollview setBorderType:NSNoBorder];
	[scrollview setHasVerticalScroller:YES];
	[scrollview setHasHorizontalScroller:NO];
	[scrollview setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

	NSTextView *theTextView =
		[[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, contentSize.width, contentSize.height)];
	theTextView_ = (__bridge void *) theTextView;
	[theTextView setMinSize:NSMakeSize(0.0, contentSize.height)];
	[theTextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
	[theTextView setVerticallyResizable:YES];
	[theTextView setHorizontallyResizable:NO];
	[theTextView setAutoresizingMask:NSViewWidthSizable];

	if (1) {
		[[theTextView enclosingScrollView] setHasHorizontalScroller:YES];
		[theTextView setHorizontallyResizable:YES];
		[theTextView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
		[[theTextView textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
		[[theTextView textContainer] setWidthTracksTextView:NO];
	}

	[[theTextView textContainer] setContainerSize:NSMakeSize(contentSize.width, FLT_MAX)];
	[[theTextView textContainer] setWidthTracksTextView:YES];
	[scrollview setDocumentView:theTextView];
	[window setContentView:scrollview];
	[window makeKeyAndOrderFront:nil];
	[window makeFirstResponder:theTextView];
	[theTextView setString:@"console"];
	[theTextView setEditable:NO];
	[[theTextView textStorage] setFont:[NSFont fontWithName:@"Menlo" size:11]];
}

void TextConsole::setBgColor(glm::vec3 c) {
	NSTextView *theTextView = (__bridge NSTextView *) theTextView_;
	dispatch_async(dispatch_get_main_queue(),
				   ^{ theTextView.backgroundColor = [NSColor colorWithRed:c.r green:c.g blue:c.b alpha:1.f]; });
}

void TextConsole::setText(string text) {
	NSTextView *theTextView = (__bridge NSTextView *) theTextView_;
	dispatch_async(dispatch_get_main_queue(),
				   ^{ [theTextView setString:[NSString stringWithUTF8String:text.c_str()]]; });
}
