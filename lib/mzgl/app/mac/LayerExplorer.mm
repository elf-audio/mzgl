//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "LayerExplorer.h"
#include <AppKit/AppKit.h>
using namespace std;
#include <typeinfo>
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

class SelectionLayer : public Layer {
public:
	SelectionLayer(Graphics &g)
		: Layer(g) {}
	void draw() override {
		ScopedAlphaBlend ab(g, true);
		g.setColor(1, 1, 0, 0.25);
		g.drawRect(*this);
		g.setColor(1, 1, 0);
		g.noFill();
		g.setStrokeWeight(3);
		g.drawRect(*this);
		g.fill();
	}
};

SelectionLayer *selectionLayer;

std::string demangle(const char *name) {
	int status = -4; // some arbitrary value to eliminate the compiler warning

	// enable c++11 by passing the flag -std=c++11 to g++
	std::unique_ptr<char, void (*)(void *)> res {abi::__cxa_demangle(name, NULL, NULL, &status), std::free};

	return (status == 0) ? res.get() : name;
}

void printLayer(Layer *l, int indent = 0) {
	for (int i = 0; i < indent; i++)
		printf("\t");

	printf("%s\n", demangle(typeid(*l).name()).c_str());

	for (int i = 0; i < l->getNumChildren(); i++) {
		printLayer(l->getChild(i), indent + 1);
	}
}

@interface LayerNode : NSObject {
}
- (NSInteger)getNumChildren;
- (LayerNode *)getChild:(NSInteger)index;
@end

@implementation LayerNode {
	NSMutableArray<LayerNode *> *children;
	Layer *layer;
}

- (id)initWithLayer:(Layer *)layerCpp {
	self	 = [super init];
	layer	 = layerCpp;
	children = [[NSMutableArray alloc] init];
	for (int i = 0; i < layerCpp->getNumChildren(); i++) {
		[children addObject:[[LayerNode alloc] initWithLayer:layerCpp->getChild(i)]];
	}
	return self;
}
- (Layer *)getLayer {
	return layer;
}
- (NSInteger)getNumChildren {
	return [children count];
}
- (LayerNode *)getChild:(NSInteger)index {
	return [children objectAtIndex:index];
}
@end

@interface SCTableCellView : NSTableCellView
@end

@implementation SCTableCellView

- (id)initWithFrame:(NSRect)frameRect {
	self = [super initWithFrame:frameRect];
	[self setAutoresizingMask:NSViewWidthSizable];
	NSImageView *iv = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 6, 16, 16)];
	NSTextField *tf = [[NSTextField alloc] initWithFrame:NSMakeRect(21, 6, 200, 14)];
	//  NSButton* btn = [[NSButton alloc] initWithFrame:NSMakeRect(0, 3, 16, 16)];
	[iv setImageScaling:NSImageScaleProportionallyUpOrDown];
	[iv setImageAlignment:NSImageAlignCenter];
	[tf setBordered:NO];
	[tf setDrawsBackground:NO];
	//  [[btn cell] setControlSize:NSSmallControlSize];
	//  [[btn cell] setBezelStyle:NSInlineBezelStyle];
	//  [[btn cell] setButtonType:NSMomentaryPushInButton];
	//  [[btn cell] setFont:[NSFont boldSystemFontOfSize:10]];
	//  [[btn cell] setAlignment:NSCenterTextAlignment];
	[self setImageView:iv];
	[self setTextField:tf];
	[self addSubview:iv];
	[self addSubview:tf];
	//  [self addSubview:btn];
	return self;
}
@end

@interface LayerExplorerDelegate : NSObject <NSOutlineViewDelegate>
@end
@implementation LayerExplorerDelegate
- (id)init {
	self = [super init];
	return self;
}

//- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
- (NSView *)outlineView:(NSOutlineView *)outlineView
	 viewForTableColumn:(NSTableColumn *)tableColumn
				   item:(id)item {
	NSTableCellView *v = [[SCTableCellView alloc]
		initWithFrame:
			CGRectMake(
				0,
				0,
				100,
				100)]; //[outlineView makeViewWithIdentifier:NSUserInterfaceItemIdentifier(@"Layer") owner:self];

	LayerNode *l = item;
	Layer *layer = [l getLayer];

	if ([tableColumn.identifier isEqualToString:@"Layer"]) {
		NSString *s = [NSString stringWithFormat:@"%s", demangle(typeid(*layer).name()).c_str()];
		[v.textField setStringValue:s];
	} else if ([tableColumn.identifier isEqualToString:@"Name"]) {
		[v.textField setStringValue:[NSString stringWithFormat:@"%s", layer->name.c_str()]];
	} else if ([tableColumn.identifier isEqualToString:@"x"]) {
		[v.textField setStringValue:[NSString stringWithFormat:@"%g", layer->x]];
	} else if ([tableColumn.identifier isEqualToString:@"y"]) {
		[v.textField setStringValue:[NSString stringWithFormat:@"%g", layer->y]];
	} else if ([tableColumn.identifier isEqualToString:@"width"]) {
		[v.textField setStringValue:[NSString stringWithFormat:@"%g", layer->width]];
	} else if ([tableColumn.identifier isEqualToString:@"height"]) {
		[v.textField setStringValue:[NSString stringWithFormat:@"%g", layer->height]];
	}
	if (!layer->visible) {
		v.textField.textColor = [NSColor systemGrayColor];
	}
	return v;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
	[outlineView reloadData];

	LayerNode *l			= item;
	Layer *layer			= [l getLayer];
	selectionLayer->visible = true;
	selectionLayer->sendToFront();
	selectionLayer->set(layer->getAbsoluteRect());
	return YES;
}
//- (NSCell *)outlineView:(NSOutlineView *)outlineView
// dataCellForTableColumn:(NSTableColumn *)tableColumn
//				   item:(id)item {
//	printf("test here\n");
//	return nil;
//}
//
//- (NSTableRowView *)outlineView:(NSOutlineView *)outlineView rowViewForItem:(id)item {
////	v.textField.stringValue = "hola";
//	printf("Getting row\n");
//	return nil;
//}
@end

@interface LayerExplorerDataSource : NSObject <NSOutlineViewDataSource> {
}

@end

@implementation LayerExplorerDataSource {
	Layer *root;
	LayerNode *rootNode;
}

- (id)initWithLayer:(Layer *)layer {
	self	 = [super init];
	root	 = layer;
	rootNode = [[LayerNode alloc] initWithLayer:root];
	return self;
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
	if (item == nil) {
		return [rootNode getNumChildren];
	} else {
		LayerNode *n = item;
		return [n getNumChildren];
	}
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
	if (item == nil) {
		// return root item
		return [rootNode getChild:index];
	} else {
		// return item
		LayerNode *n = item;
		return [n getChild:index];
	}
}
- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
	LayerNode *n = item;
	return [n getLayer]->getNumChildren() > 0;
}

@end

void LayerExplorer::setup(Layer *root) {
	//	printLayer(root);
	rootLayer	   = root;
	selectionLayer = new SelectionLayer(root->getGraphics());
	rootLayer->addChild(selectionLayer);
	selectionLayer->visible = false;
}

NSOutlineView *browser;
LayerExplorerDelegate *layerDelegate;
LayerExplorerDataSource *layerDataSource;

NSTableColumn *addColumn(NSString *name, int colWidth = 200) {
	NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:NSUserInterfaceItemIdentifier(name)];
	col.title		   = name;
	col.width		   = colWidth;
	[browser addTableColumn:col];
	return col;
}

NSWindow *window;
void LayerExplorer::hide() {
	isShowing = false;
	dispatch_async(dispatch_get_main_queue(), ^{ [window close]; });
}

void LayerExplorer::show() {
	if (isShowing) return;
	isShowing = true;
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSRect windowRect = NSMakeRect(0, 0, 500, 200);
	  window			= [[NSWindow alloc] initWithContentRect:windowRect
											  styleMask:NSTitledWindowMask | NSWindowStyleMaskResizable
												backing:NSBackingStoreBuffered
												  defer:NO];

	  [window cascadeTopLeftFromPoint:NSMakePoint(500, 20)];
	  [window setTitle:@"Layer Explorer"];

	  NSScrollView *scrollview = [[NSScrollView alloc] initWithFrame:[[window contentView] frame]];

	  NSSize contentSize = [scrollview contentSize];

	  [scrollview setBorderType:NSNoBorder];
	  [scrollview setHasVerticalScroller:YES];
	  [scrollview setHasHorizontalScroller:NO];
	  [scrollview setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

	  browser  = [[NSOutlineView alloc] initWithFrame:NSMakeRect(0, 0, contentSize.width, contentSize.height)];
	  browser_ = (__bridge void *) browser;

	  layerDelegate				  = [[LayerExplorerDelegate alloc] init];
	  layerDataSource			  = [[LayerExplorerDataSource alloc] initWithLayer:rootLayer];
	  browser.delegate			  = layerDelegate;
	  browser.dataSource		  = layerDataSource;
	  browser.indentationPerLevel = 16.f;
	  //		browser.indentationMarkerFollowsCell = true;
	  browser.selectionHighlightStyle = NSTableViewSelectionHighlightStyleRegular;

	  NSTableColumn *col		 = addColumn(@"Layer", 200);
	  browser.outlineTableColumn = col;
	  addColumn(@"Name", 140);
	  addColumn(@"x", 50);
	  addColumn(@"y", 50);
	  addColumn(@"width", 50);
	  addColumn(@"height", 50);
	  //		NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:NSUserInterfaceItemIdentifier(@"Layer")];
	  //		col.title = @"Layer";
	  //		col.width = 200;
	  //		[browser addTableColumn:col];
	  //		browser.outlineTableColumn = col;
	  //		NSTableColumn *col2 = [[NSTableColumn alloc] initWithIdentifier:NSUserInterfaceItemIdentifier(@"Name")];
	  //		col2.title = @"Name";
	  //		col2.width = 200;
	  //		[browser addTableColumn:col2];

	  [browser setAutoresizingMask:NSViewWidthSizable];

	  [[browser enclosingScrollView] setHasHorizontalScroller:YES];
	  [browser setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

	  [browser expandItem:nil expandChildren:NO];

	  [scrollview setDocumentView:browser];
	  [window setContentView:scrollview];
	  [window makeKeyAndOrderFront:nil];
	  [window makeFirstResponder:browser];
	});
}

void LayerExplorer::setBgColor(glm::vec3 c) {
	NSTextView *browser = (__bridge NSTextView *) browser_;
	dispatch_async(dispatch_get_main_queue(),
				   ^{ browser.backgroundColor = [NSColor colorWithRed:c.r green:c.g blue:c.b alpha:1.f]; });
}

void LayerExplorer::setText(string text) {
	NSTextView *browser = (__bridge NSTextView *) browser_;
	dispatch_async(dispatch_get_main_queue(),
				   ^{ [browser setString:[NSString stringWithUTF8String:text.c_str()]]; });
}
