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
#include "mzgl/util/TextInput.h"
#include "mzgl/util/log.h"
#include "Vbo.h"
#include "PluginEditor.h"
#include "sokol_gfx.h"
#include "SokolSetup.h"
using namespace std;

#pragma mark - MZMetaliOSView (Metal render base)

@implementation MZMetaliOSView {
    bool firstFrame;
    sg_pass_action pass_action;
}

static sg_pixel_format ios_depth_format = SG_PIXELFORMAT_NONE;

static sg_environment ios_environment(MTKView *mtkView) {
    return (sg_environment) {.defaults =
                                 {
                                     .sample_count = mzglSokolSampleCount,
                                     .color_format = SG_PIXELFORMAT_BGRA8,
                                     .depth_format = ios_depth_format,
                                 },
                             .metal = {
                                 .device = (__bridge const void *) mtkView.device,
                             }};
}

static sg_swapchain ios_swapchain(MTKView *mtkView) {
    return (sg_swapchain) {.width        = (int) [mtkView drawableSize].width,
                           .height       = (int) [mtkView drawableSize].height,
                           .sample_count = mzglSokolSampleCount,
                           .color_format = SG_PIXELFORMAT_BGRA8,
                           .depth_format = ios_depth_format,
                           .metal        = {
                                  .current_drawable      = (__bridge const void *) [mtkView currentDrawable],
                                  .depth_stencil_texture = (__bridge const void *) [mtkView depthStencilTexture],
                                  .msaa_color_texture    = (__bridge const void *) [mtkView multisampleColorTexture],
                          }};
}

- (id)initWithApp:(std::shared_ptr<App>)_app andGraphics:(std::shared_ptr<Graphics>)_graphics {
    self = [super initWithFrame:CGRectZero device:MTLCreateSystemDefaultDevice()];
    if (self != nil) {
        app             = _app;
        graphics        = _graphics;
        eventDispatcher = std::make_shared<EventDispatcher>(app);
        firstFrame      = true;

        self.delegate = self;
        [self setSampleCount:(NSUInteger) mzglSokolSampleCount];
        self.preferredFramesPerSecond = 60;
        self.colorPixelFormat         = MTLPixelFormatBGRA8Unorm;
        switch (ios_depth_format) {
            case SG_PIXELFORMAT_DEPTH_STENCIL:
                [self setDepthStencilPixelFormat:MTLPixelFormatDepth32Float_Stencil8];
                break;
            case SG_PIXELFORMAT_DEPTH: [self setDepthStencilPixelFormat:MTLPixelFormatDepth32Float]; break;
            default: [self setDepthStencilPixelFormat:MTLPixelFormatInvalid]; break;
        }
        self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

        mzglSokolSetup(ios_environment(self));

        // In an AUv3 extension MTKView pauses its display link when the host
        // backgrounds (UIApplicationDidEnterBackgroundNotification reaches the
        // extension) but UIApplicationWillEnterForegroundNotification is never
        // posted in an extension process, so it stays paused forever. Extensions
        // get the NSExtensionHost* notifications instead — mirror pause/resume
        // off those. Harmless in the standalone app: they never fire there.
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(hostDidEnterBackground)
                                                     name:NSExtensionHostDidEnterBackgroundNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(hostWillEnterForeground)
                                                     name:NSExtensionHostWillEnterForegroundNotification
                                                   object:nil];
    }
    return self;
}

- (void)hostDidEnterBackground {
    self.paused = YES;
    [self releaseDrawables];
}

- (void)hostWillEnterForeground {
    self.paused = NO;
}

- (void)deleteCppObjects {
    app             = nullptr;
    eventDispatcher = nullptr;
    graphics        = nullptr;
}

- (std::shared_ptr<App>)getApp {
    return app;
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
    return eventDispatcher;
}

- (void)dealloc {
    NSLog(@"Tearing down MZMetaliOSView");
    // Tear the C++ objects down in an explicit, safe order (app -> graphics).
    // Otherwise ARC's .cxx_destruct destroys the ivars in reverse declaration
    // order, freeing `graphics` before `app`; ~App then deletes its Layer tree,
    // and ~Layer dereferences the (now dangling) Graphics& -> crash. This path
    // matters for teardowns that don't go through the view controller's
    // deleteCppObjects (e.g. AUv3 extension scene invalidation). Idempotent:
    // resetting already-null shared_ptrs is a no-op.
    [self deleteCppObjects];
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    (void) view;
    if (app == nullptr) return;
    @autoreleasepool {
        if (firstFrame) {
            app->g.pixelScale = [[UIScreen mainScreen] nativeScale];
            initMZGL(app);
            app->g.width  = (int) [self drawableSize].width;
            app->g.height = (int) [self drawableSize].height;
            if (app->g.width == 0 || app->g.height == 0) {
                app->g.width  = 100;
                app->g.height = 100;
                printf("ERROR: WIDTH OR HEIGHT is ZERO\n");
            }
            eventDispatcher->setup();
            firstFrame = false;
        }

        sg_pass pass = {.action = pass_action, .swapchain = ios_swapchain(self)};
        sg_begin_pass(pass);
        eventDispatcher->runFrame();
        sg_end_pass();
        sg_commit();
    }
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    (void) view;
    (void) size;
}

- (BOOL)handleNormalOpen:(NSURL *)url {
    NSLog(@"Standard open");

    NSString *path = url.path;

    std::function<void()> deleter = []() {};
    if (![[NSFileManager defaultManager] isReadableFileAtPath:path]) {
        NSLog(@"Using security scoped url\n");
        [url startAccessingSecurityScopedResource];

        deleter = [url]() {
            NSLog(@"Releasing security scoped url\n");
            [url stopAccessingSecurityScopedResource];
        };
    }

    return eventDispatcher->openUrl(ScopedUrl::createWithCallback([path UTF8String], deleter));
}
@end

#pragma mark - EventsView (shared interaction handling)

API_AVAILABLE(ios(13.4))
int uikeyToMz(UIKey *key) {
    if (key.keyCode == UIKeyboardHIDUsageKeyboardDeleteOrBackspace) {
        return MZ_KEY_DELETE;
    }
    NSString *ch = key.charactersIgnoringModifiers;
    if ([ch isEqualToString:UIKeyInputUpArrow]) {
        return MZ_KEY_UP;
    } else if ([ch isEqualToString:UIKeyInputDownArrow]) {
        return MZ_KEY_DOWN;
    } else if ([ch isEqualToString:UIKeyInputLeftArrow]) {
        return MZ_KEY_LEFT;
    } else if ([ch isEqualToString:UIKeyInputRightArrow]) {
        return MZ_KEY_RIGHT;
    }
    string keyStr = [key.charactersIgnoringModifiers UTF8String];
    if (keyStr.size() == 1) {
        if (keyStr[0] == 9) {
            if (key.modifierFlags == UIKeyModifierShift) {
                return MZ_KEY_SHIFT_TAB;
            } else {
                return MZ_KEY_TAB;
            }
        }

        keyStr = [key.characters UTF8String];
        if (keyStr.size() == 1) {
            return keyStr[0];
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

// The drag/drop UIDropInteractionDelegate methods are implemented as a category
// on EventsView (in the app target).
API_AVAILABLE(ios(11)) @interface EventsView () <UIDropInteractionDelegate, UITextFieldDelegate>
@end

@implementation EventsView {
    NSMutableDictionary *activeTouches;
    // Invisible input sink: the Metal view can't host a visible UITextField, so
    // we park a real one off-screen. iOS owns the text buffer (giving us
    // autocorrect / predictive / dictation / IME for free); MZGL mirrors its
    // .text on every change and draws the field itself. See setupTextInput.
    UITextField *hiddenTextField;
}

- (id)initWithApp:(std::shared_ptr<App>)_app andGraphics:(std::shared_ptr<Graphics>)_graphics {
    self = [super initWithApp:_app andGraphics:_graphics];
    if (self != nil) {
        activeTouches               = [[NSMutableDictionary alloc] init];
        self.multipleTouchEnabled   = YES;
        self.userInteractionEnabled = YES;
        if (@available(iOS 11, *)) {
            [self addInteraction:[[UIDropInteraction alloc] initWithDelegate:self]];
        }
        [self setupTextInput];
    }
    return self;
}

- (void)setupTextInput {
    hiddenTextField = [[UITextField alloc] initWithFrame:CGRectMake(-100, -100, 1, 1)];
    hiddenTextField.delegate               = self;
    hiddenTextField.autocorrectionType     = UITextAutocorrectionTypeNo;
    hiddenTextField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    hiddenTextField.returnKeyType          = UIReturnKeySearch;
    // Not hidden=YES (a hidden field can't become first responder) — off-screen.
    hiddenTextField.hidden = NO;
    [self addSubview:hiddenTextField];
    [hiddenTextField addTarget:self
                        action:@selector(textFieldChanged:)
              forControlEvents:UIControlEventEditingChanged];

    // Bridge Graphics::showKeyboard()/hideKeyboard() to the hidden field.
    __weak EventsView *weakSelf = self;
    app->g.onShowKeyboard = [weakSelf](TextInputReceiver *r) {
        std::string cur = r ? r->getText() : std::string();
        dispatch_async(dispatch_get_main_queue(), ^{
          EventsView *s = weakSelf;
          if (s == nil) return;
          s->hiddenTextField.text = [NSString stringWithUTF8String:cur.c_str()];
          [s->hiddenTextField becomeFirstResponder];
        });
    };
    app->g.onHideKeyboard = [weakSelf]() {
        dispatch_async(dispatch_get_main_queue(), ^{
          EventsView *s = weakSelf;
          if (s == nil) return;
          [s->hiddenTextField resignFirstResponder];
        });
    };
}

- (void)textFieldChanged:(UITextField *)tf {
    std::string s = tf.text ? std::string([tf.text UTF8String]) : std::string();
    eventDispatcher->textSetString(s);
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    eventDispatcher->textDone();
    [textField resignFirstResponder];
    return YES;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        int touchIndex = 0;
        while ([[activeTouches allValues] containsObject:[NSNumber numberWithInt:touchIndex]]) {
            touchIndex++;
        }

        [activeTouches setObject:[NSNumber numberWithInt:touchIndex]
                          forKey:[NSValue valueWithNonretainedObject:touch]];
        //[activeTouches setObject:[NSNumber numberWithInt:touchIndex] forKey:touch];

        CGPoint touchPoint = [touch locationInView:self];

        touchPoint.x *=
            app->g
                .pixelScale; // this has to be done because retina still returns points in 320x240 but with high percision
        touchPoint.y *= app->g.pixelScale;
        eventDispatcher->touchDown(touchPoint.x, touchPoint.y, touchIndex);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];

        CGPoint touchPoint = [touch locationInView:self];

        // this has to be done because retina still returns points in 320x240 but with high percision
        touchPoint.x *= app->g.pixelScale;
        touchPoint.y *= app->g.pixelScale;
        eventDispatcher->touchMoved(touchPoint.x, touchPoint.y, touchIndex);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];

        [activeTouches removeObjectForKey:[NSValue valueWithNonretainedObject:touch]];

        CGPoint touchPoint = [touch locationInView:self];

        // this has to be done because retina still returns points in 320x240 but with high percision
        touchPoint.x *= app->g.pixelScale;
        touchPoint.y *= app->g.pixelScale;
        eventDispatcher->touchUp(touchPoint.x, touchPoint.y, touchIndex);
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        //int touchIndex = [[activeTouches objectForKey:[NSValue valueWithNonretainedObject:touch]] intValue];

        CGPoint touchPoint = [touch locationInView:self];
        // this has to be done because retina still returns points in 320x240 but with high percision
        touchPoint.x *= app->g.pixelScale;
        touchPoint.y *= app->g.pixelScale;
    }

    [self touchesEnded:touches withEvent:event];
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    bool handled = false;
    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key = press.key;
            if (key != nil) {
                int k = uikeyToMz(key);
                if (k != -1) {
                    //                  Log::d() << "KeyCode: " << k << " UIKey.keycode = " << key.keyCode;
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
    if (!handled) {
        [super pressesBegan:presses withEvent:event];
    }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    bool handled = false;

    if (@available(iOS 13.4, *)) {
        for (UIPress *press in presses) {
            UIKey *key = press.key;

            if (key != nil) {
                int k = uikeyToMz(key);
                if (k != -1) {
                    eventDispatcher->keyUp(k);
                }
                handled = true;
            }
        }
    }
    if (!handled) {
        [super pressesBegan:presses withEvent:event];
    }
}
@end
