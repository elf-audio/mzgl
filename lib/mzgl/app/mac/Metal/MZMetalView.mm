#import "MZMetalView.h"
#include "sokol_gfx.h"
#include "EventDispatcher.h"
#include "SokolSetup.h"
#include "Image.h"
#include <mutex>
#include <string>

@implementation MZMetalView {
	sg_pass_action pass_action;
	id<MTLCommandQueue> captureQueue;
	std::mutex screenshotMutex;
	std::string screenshotPath;
	bool screenshotPending;
}

static sg_pixel_format depth_format = SG_PIXELFORMAT_NONE;

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	eventDispatcher = evtDispatcher;
	self			= [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
	if (self != nil) {
		self.delegate = self;
		[self setSampleCount:(NSUInteger) mzglSokolSampleCount];
		[self setDevice:MTLCreateSystemDefaultDevice()];
		self.preferredFramesPerSecond = 60;

		self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
		switch (depth_format) {
			case SG_PIXELFORMAT_DEPTH_STENCIL:
				[self setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
				break;
			case SG_PIXELFORMAT_DEPTH: [self setDepthStencilPixelFormat:MTLPixelFormatDepth32Float]; break;
			default: [self setDepthStencilPixelFormat:MTLPixelFormatInvalid]; break;
		}
		self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
		[self setDrawableSize:[self convertSizeToBacking:frame.size]];

		// Allow the drawable texture to be used as a blit source so we can read
		// it back for screenshots (saveScreen). Without this the drawable is
		// framebuffer-only and the blit below is invalid.
		self.framebufferOnly = NO;
		captureQueue		 = [self.device newCommandQueue];
		screenshotPending	 = false;

		// TODO: this might be why it looks a bit crispy, try linear?
		//		[self.layer setMagnificationFilter:kCAFilterNearest];

		// setup sokol
		mzglSokolSetup(osx_environment(self));

		// Install the deferred screenshot hook. Graphics::saveScreen() (and the
		// WS test server's save-screen action) just flag a request here; the
		// actual readback happens at the end of the next frame in drawInMTKView,
		// where a fully-rendered drawable exists - reading it synchronously from
		// the render thread would otherwise deadlock or capture an empty frame.
		// Plain pointer capture: this is a C++ lambda (not an ObjC block) in an
		// MRC file, so it does not retain self - no retain cycle. The view lives
		// for the app's lifetime, alongside the Graphics that owns this lambda.
		MZMetalView *rawSelf = self;
		eventDispatcher->app->g.deferredSaveScreen = [rawSelf](const std::string &path) -> bool {
			if (rawSelf == nil) return false;
			std::lock_guard<std::mutex> lock(rawSelf->screenshotMutex);
			rawSelf->screenshotPath	   = path;
			rawSelf->screenshotPending = true;
			return true;
		};
	}
	return self;
}

- (void)captureScreenshotIfRequested {
	std::string path;
	{
		std::lock_guard<std::mutex> lock(screenshotMutex);
		if (!screenshotPending) return;
		screenshotPending = false;
		path			  = screenshotPath;
	}

	id<CAMetalDrawable> drawable = self.currentDrawable;
	if (drawable == nil) return;
	id<MTLTexture> tex = drawable.texture;

	NSUInteger w		   = tex.width;
	NSUInteger h		   = tex.height;
	NSUInteger bytesPerRow = w * 4;
	id<MTLBuffer> buffer   = [self.device newBufferWithLength:bytesPerRow * h
													 options:MTLResourceStorageModeShared];

	id<MTLCommandBuffer> cmd	  = [captureQueue commandBuffer];
	id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
	[blit copyFromTexture:tex
				 sourceSlice:0
				 sourceLevel:0
				sourceOrigin:MTLOriginMake(0, 0, 0)
				  sourceSize:MTLSizeMake(w, h, 1)
					toBuffer:buffer
		   destinationOffset:0
	  destinationBytesPerRow:bytesPerRow
	destinationBytesPerImage:bytesPerRow * h];
	[blit endEncoding];
	[cmd commit];
	[cmd waitUntilCompleted];

	// Drawable is BGRA8; convert to RGBA and force opaque. Metal's drawable
	// origin is top-left so no vertical flip is needed (unlike the GL path).
	const uint8_t *src = (const uint8_t *) buffer.contents;
	std::vector<uint8_t> rgba(w * h * 4);
	for (NSUInteger i = 0; i < w * h; i++) {
		rgba[i * 4 + 0] = src[i * 4 + 2];
		rgba[i * 4 + 1] = src[i * 4 + 1];
		rgba[i * 4 + 2] = src[i * 4 + 0];
		rgba[i * 4 + 3] = 255;
	}
	Image::save(path, rgba.data(), (int) w, (int) h, 4, 1, false);
}

- (void)disableDrawing {
}
- (void)enableDrawing {
}
- (void)shutdown {
}

- (void)windowResized:(NSNotification *)notification {
	NSWindow *window = notification.object;

	Graphics &g = eventDispatcher->app->g;
	g.width		= window.contentLayoutRect.size.width;
	g.height	= window.contentLayoutRect.size.height;

	bool hasTransparentTitleBar = true;

	if (hasTransparentTitleBar) {
		g.width	 = window.frame.size.width;
		g.height = window.frame.size.height;
	}
	g.pixelScale = [window backingScaleFactor];

	// Resize the view in points; MTKView (autoResizeDrawable) updates its
	// drawableSize to points * backingScaleFactor, which osx_swapchain reads.
	auto f		  = self.frame;
	f.size.width  = g.width;
	f.size.height = g.height;
	self.frame	  = f;

	g.width *= 2;
	g.height *= 2;

	eventDispatcher->app->main.runOnMainThread(true,
											   [evtDispatcher = eventDispatcher]() { evtDispatcher->resized(); });
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

sg_environment osx_environment(MTKView *mtkView) {
	return (sg_environment) {.defaults =
								 {
									 .color_format = SG_PIXELFORMAT_BGRA8,
									 .depth_format = depth_format,
								     .sample_count = mzglSokolSampleCount,
								 },
							 .metal = {
								 .device = (__bridge const void *) mtkView.device,
							 }};
}

sg_swapchain osx_swapchain(MTKView *mtkView) {
	return (sg_swapchain) {.width		 = (int) [mtkView drawableSize].width,
						   .height		 = (int) [mtkView drawableSize].height,
						   .sample_count = mzglSokolSampleCount,
						   .color_format = SG_PIXELFORMAT_BGRA8,
						   .depth_format = depth_format,
						   .metal		 = {
									  .current_drawable		 = (__bridge const void *) [mtkView currentDrawable],
									  .depth_stencil_texture = (__bridge const void *) [mtkView depthStencilTexture],
									  .msaa_color_texture	 = (__bridge const void *) [mtkView multisampleColorTexture],
							  }};
}

- (void)drawInMTKView:(nonnull MTKView *)view {
	(void) view;
	@autoreleasepool {
		sg_pass pass = {.action = pass_action, .swapchain = osx_swapchain(self)};
		sg_begin_pass(pass);
		if (eventDispatcher->app->g.firstFrame) {
			initMZGL(eventDispatcher->app);
			eventDispatcher->setup();
			eventDispatcher->app->g.firstFrame = false;
		}

		eventDispatcher->runFrame();
		sg_end_pass();
		sg_commit();

		[self captureScreenshotIfRequested];
	}
}
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
	(void) view;
	(void) size;
}

@end
