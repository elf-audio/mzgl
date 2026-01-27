#import "GLRenderer.h"
#import "MZGLView.h"
#include "log.h"
#include "EventDispatcher.h"

static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
									const CVTimeStamp *inNow,
									const CVTimeStamp *inOutputTime,
									CVOptionFlags flagsIn,
									CVOptionFlags *flagsOut,
									void *displayLinkContext) {
	CVDisplayLinkRenderer *renderer = static_cast<CVDisplayLinkRenderer *>(displayLinkContext);
	if (renderer->isDrawing()) {
		renderer->render();
	}
	return kCVReturnSuccess;
}

CVDisplayLinkRenderer::CVDisplayLinkRenderer(MZGLView *v)
	: view(v) {
}

CVDisplayLinkRenderer::~CVDisplayLinkRenderer() {
	stop();
}

void CVDisplayLinkRenderer::start() {
	drawing = true;

	CGDirectDisplayID displayID = CGMainDisplayID();
	CVReturn error				= CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);

	if (kCVReturnSuccess == error) {
		CVDisplayLinkSetOutputCallback(displayLink, displayLinkCallback, this);
		CVDisplayLinkStart(displayLink);
	} else {
		NSLog(@"Display Link created with error: %d", error);
		displayLink = nullptr;
	}
}

void CVDisplayLinkRenderer::stop() {
	drawing = false;

	if (displayLink != nullptr) {
		CVDisplayLinkStop(displayLink);
	}

	std::lock_guard<std::mutex> guard(mutex);
	if (displayLink != nullptr) {
		CVDisplayLinkRelease(displayLink);
		displayLink = nullptr;
	}

	view = nil;
}

void CVDisplayLinkRenderer::render() {
	std::lock_guard<std::mutex> guard(mutex);

	if (!drawing || view == nil) {
		return;
	}

	[view performRender];
}

@interface TimerRendererHelper : NSObject {
	TimerRenderer *renderer;
}
- (instancetype)initWithRenderer:(TimerRenderer *)r;
- (void)timerFired:(NSTimer *)timer;
- (void)clearRenderer;
@end

@implementation TimerRendererHelper

- (instancetype)initWithRenderer:(TimerRenderer *)r {
	self = [super init];
	if (self) {
		renderer = r;
	}
	return self;
}

- (void)timerFired:(NSTimer *)timer {
	if (renderer) {
		renderer->render();
	}
}

- (void)clearRenderer {
	renderer = nullptr;
}

@end

TimerRenderer::TimerRenderer(MZGLView *v)
	: view(v) {
}

TimerRenderer::~TimerRenderer() {
	stop();
}

void TimerRenderer::start() {
	drawing = true;

	timerHelper = [[TimerRendererHelper alloc] initWithRenderer:this];

	renderTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
												   target:timerHelper
												 selector:@selector(timerFired:)
												 userInfo:nil
												  repeats:YES];

	[[NSRunLoop currentRunLoop] addTimer:renderTimer forMode:NSRunLoopCommonModes];
}

void TimerRenderer::stop() {
	drawing = false;

	if (renderTimer != nil) {
		[renderTimer invalidate];
		renderTimer = nil;
	}

	if (timerHelper != nil) {
		[timerHelper clearRenderer];
#if defined(MZGL_PLUGIN_VST)
		[timerHelper release];
#endif
		timerHelper = nil;
	}

	view = nil;
}

void TimerRenderer::render() {
	if (!drawing || view == nil) {
		return;
	}

	[view performRender];
}
