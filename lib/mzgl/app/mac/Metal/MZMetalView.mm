#import "MZMetalView.h"
#include "sokol_gfx.h"
#include "EventDispatcher.h"
#include "sokol_log.h"

@implementation MZMetalView {
	sg_pass_action pass_action;
}

static int sample_count				= 4;
static sg_pixel_format depth_format = SG_PIXELFORMAT_NONE;

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	eventDispatcher = evtDispatcher;
	self			= [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
	if (self != nil) {
		self.delegate = self;
		[self setSampleCount:(NSUInteger) sample_count];
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
		[self setDrawableSize:frame.size];

		// TODO: this might be why it looks a bit crispy, try linear?
		//		[self.layer setMagnificationFilter:kCAFilterNearest];

		// setup sokol
		sg_desc desc = {
			.environment	  = osx_environment(self),
			.logger.func	  = slog_func,
			.buffer_pool_size = 4096, // default is 128, // 1024 * 16, // DUBIOUS - do we really need so many?
			.shader_pool_size = 128,
		};

		sg_setup(desc);
	}
	return self;
}

- (void)disableDrawing {
}
- (void)enableDrawing {
}
- (void)shutdown {
}

- (void)windowResized:(NSNotification *)notification {
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

sg_environment osx_environment(MTKView *mtkView) {
	return (sg_environment) {.defaults =
								 {
									 .sample_count = sample_count,
									 .color_format = SG_PIXELFORMAT_BGRA8,
									 .depth_format = depth_format,
								 },
							 .metal = {
								 .device = (__bridge const void *) mtkView.device,
							 }};
}

sg_swapchain osx_swapchain(MTKView *mtkView) {
	return (sg_swapchain) {.width		 = (int) [mtkView drawableSize].width,
						   .height		 = (int) [mtkView drawableSize].height,
						   .sample_count = sample_count,
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
	}
}
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
	(void) view;
	(void) size;
}

@end
