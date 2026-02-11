#include "Dialogs.h"
#include "App.h"
#include "log.h"

#ifdef __APPLE__
#	include <TargetConditionals.h>
#	import <WebKit/WebKit.h>
#	if TARGET_OS_IOS
#		import <UIKit/UIKit.h>
#	else
#		import <AppKit/AppKit.h>
#	endif
#elif defined(__ANDROID__)
#	include "androidUtil.h"
#elif defined(_WIN32)
#	include "winUtil.h"
#elif defined(__linux__)
#	include "linuxUtil.h"
#endif
#include "mzAssert.h"

#include "filesystem.h"
#include "mainThread.h"
#include "pathUtil.h"

namespace unit_test::dialogs {

	[[nodiscard]] bool dialogSpoofingIsEnabled() {
		const static bool spoof = hasCommandLineFlag("--enable-dialog-spoofing");
		return spoof;
	}

	std::function<void()> dialogOkPressed;
	std::function<void()> dialogCancelPressed;
	std::function<void()> dialogButtonOnePressed;
	std::function<void()> dialogButtonTwoPressed;
	std::function<void()> dialogButtonThreePressed;

	bool dialogOpen = false;
	bool isDialogOpen() {
		return dialogOpen;
	}
	void dismissAlert() {
		dialogOpen = false;
	}
	void pressOk() {
		dialogOpen = false;
		dialogOkPressed();
	}

	void pressCancel() {
		dialogCancelPressed();
	}
	void pressButtonOne() {
		dialogButtonOnePressed();
	}
	void pressButtonTwo() {
		dialogButtonTwoPressed();
	}
	void pressButtonThree() {
		dialogButtonThreePressed();
	}
} // namespace unit_test::dialogs

#ifdef __APPLE__
#	if TARGET_OS_IOS
UIViewController *getTopController(App &app) {
	UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
	if (topController == nil) {
		topController = ((__bridge UIViewController *) app.viewController);
	}
	while (topController.presentedViewController) {
		topController = topController.presentedViewController;
	}
	return topController;
}

#	endif
#endif

void Dialogs::inputbox(std::string title,
					   std::string msg,
					   std::string text,
					   InputBoxType type,
					   std::function<void(std::string, bool)> completionCallback) const {
#ifdef AUTO_TEST
	completionCallback(title, randi(2) == 0);
	return;
#endif

#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Ok"
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) {
											  UITextField *alertTextField = alert.textFields.firstObject;
											  completionCallback([alertTextField.text UTF8String], true);
											}]];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
											  style:UIAlertActionStyleCancel
											handler:^(UIAlertAction *action) { completionCallback("", false); }]];

	[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
	  textField.text = [NSString stringWithUTF8String:text.c_str()];
	  textField.keyboardType =
		  type == InputBoxType::Number ? UIKeyboardTypeNumbersAndPunctuation : UIKeyboardTypeDefault;
	  dispatch_async(dispatch_get_main_queue(), ^{ [textField selectAll:nil]; });
	}];
	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];
	  [alert addButtonWithTitle:@"OK"];
	  [alert addButtonWithTitle:@"Cancel"];
	  [alert setMessageText:[NSString stringWithCString:msg.c_str() encoding:NSUTF8StringEncoding]];
	  NSTextField *label = [[NSTextField alloc] initWithFrame:NSRectFromCGRect(CGRectMake(0, 0, 300, 40))];
	  [label setStringValue:[NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding]];
	  [label selectText:nil];
	  [alert setAccessoryView:label];

	  std::function<void(NSInteger, NSTextField *)> handleResult = [completionCallback, this](NSInteger returnCode,
																							  NSTextField *label) {
		  std::string txt = "";
		  if (returnCode == NSAlertFirstButtonReturn) {
			  txt = [[label stringValue] UTF8String];
		  }
		  app.main.runOnMainThread(true, [txt, returnCode, completionCallback]() {
			  completionCallback(txt, returnCode == NSAlertFirstButtonReturn);
		  });
	  };
#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result, label); }];
	  [label becomeFirstResponder];
#		else
        [label becomeFirstResponder];
        handleResult([alert runModal], label);
#		endif
	});
#	endif
#elif defined(__ANDROID__)
	if (type == InputBoxType::Number) {
		androidNumberboxDialog(title, msg, text, completionCallback);
	} else {
		androidTextboxDialog(title, msg, text, completionCallback);
	}
#elif defined(_WIN32)
	windowsTextboxDialog(static_cast<HWND>(app.nativeWindowHandle), title, msg, text, completionCallback);
#elif defined(__linux__)
	linuxTextboxDialog(title, msg, text, completionCallback);
#endif
}

void Dialogs::textbox(std::string title,
					  std::string msg,
					  std::string text,
					  std::function<void(std::string, bool)> completionCallback) const {
	inputbox(title, msg, text, InputBoxType::Text, completionCallback);
}

void Dialogs::numberbox(std::string title,
						std::string msg,
						std::string initialValue,
						std::function<void(std::string, bool)> completionCallback) const {
	inputbox(title, msg, initialValue, InputBoxType::Number, completionCallback);
}

void Dialogs::confirm(std::string title,
					  std::string msg,
					  std::function<void()> okPressed,
					  std::function<void()> cancelPressed) const {
#ifdef AUTO_TEST
	int i = randi(100) % 2;
	if (i == 0) okPressed();
	else cancelPressed();
	return;
#endif

	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogOkPressed		= okPressed;
		unit_test::dialogs::dialogCancelPressed = cancelPressed;
		unit_test::dialogs::dialogOpen			= true;
		return;
	}

#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];

	[alert addAction:[UIAlertAction actionWithTitle:@"Ok"
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { okPressed(); }]];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
											  style:UIAlertActionStyleCancel
											handler:^(UIAlertAction *action) { cancelPressed(); }]];

	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else
	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];
	  [alert addButtonWithTitle:@"OK"];
	  [alert addButtonWithTitle:@"Cancel"];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  //[alert setInformativeText:@"Deleted records cannot be restored."];
	  [alert setAlertStyle:NSAlertStyleWarning];

	  std::function<void(NSInteger)> handleResult = [okPressed, cancelPressed, this](NSInteger result) {
		  if (result == NSAlertFirstButtonReturn) {
			  app.main.runOnMainThread(okPressed);
		  } else {
			  app.main.runOnMainThread(cancelPressed);
		  }
	  };
#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
        handleResult([alert runModal]);
#		endif
	});
#	endif
#elif defined(__ANDROID__)
	androidConfirmDialog(title, msg, okPressed, cancelPressed);
#elif defined(_WIN32)
	Log::d() << "Calling here";
	windowsConfirmDialog(static_cast<HWND>(app.nativeWindowHandle), title, msg, okPressed, cancelPressed);
#elif defined(__linux__)
	linuxConfirmDialog(title, msg, okPressed, cancelPressed);
#endif
}

void Dialogs::alert(std::string title, std::string msg) const {
#ifdef AUTO_TEST
	return;
#endif

	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogOpen = true;
		return;
	}

#ifdef _WIN32
	// we need to convert error message to a wide char message.
	// first, figure out the length and allocate a wchar_t at that length + 1 (the +1 is for a terminating character)
	int length		   = strlen(msg.c_str());
	wchar_t *widearray = new wchar_t[length + 1];
	memset(widearray, 0, sizeof(wchar_t) * (length + 1));
	// then, call mbstowcs:
	// http://www.cplusplus.com/reference/clibrary/cstdlib/mbstowcs/
	mbstowcs(widearray, msg.c_str(), length);
	// launch the alert:
	MessageBoxW(NULL, widearray, L"alert", MB_OK);
	// clear the allocated memory:
	delete[] widearray;
#elif defined(__APPLE__)

#	if TARGET_OS_IOS
	dispatch_async(dispatch_get_main_queue(), ^{
	  UIAlertController *alert =
		  [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											  message:[NSString stringWithUTF8String:msg.c_str()]
									   preferredStyle:UIAlertControllerStyleAlert];

	  [alert addAction:[UIAlertAction actionWithTitle:@"Ok"
												style:UIAlertActionStyleDefault
											  handler:^(UIAlertAction *action) {
												  //okPressed();
											  }]];

	  [getTopController(app) presentViewController:alert animated:YES completion:nil];
	});

#	else
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];
	  [alert addButtonWithTitle:@"OK"];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  [alert setAlertStyle:NSAlertStyleWarning];
#		ifndef MZGLAU
	  NSWindow *window = [NSApp mainWindow];
	  if (window == nil) {
		  Log::e() << "No main window found for alert dialog, using runModal instead";
		  [alert runModal];
		  return;
	  }
	  [alert beginSheetModalForWindow:window completionHandler:^(NSInteger result) { NSLog(@"Success"); }];
#		else
        [alert runModal];
#		endif
	});
#	endif
#elif defined(__ANDROID__)
	androidAlertDialog(title, msg);
#elif defined(__linux__)
	linuxAlertDialog(title, msg);
#endif
}

void Dialogs::twoOptionCancelDialog(std::string title,
									std::string msg,
									std::string buttonOneText,
									std::function<void()> buttonOnePressed,
									std::string buttonTwoText,
									std::function<void()> buttonTwoPressed,
									std::function<void()> cancelPressed) const {
	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogButtonOnePressed = buttonOnePressed;
		unit_test::dialogs::dialogButtonTwoPressed = buttonTwoPressed;
		unit_test::dialogs::dialogCancelPressed	   = cancelPressed;
		unit_test::dialogs::dialogOpen			   = true;
		return;
	}

#ifdef AUTO_TEST
	int i = randi(100) % 3;
	if (i == 0) buttonOnePressed();
	else if (i == 1) buttonTwoPressed();
	else cancelPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonOnePressed(); }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonTwoPressed(); }]];

	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
											  style:UIAlertActionStyleCancel
											handler:^(UIAlertAction *action) { cancelPressed(); }]];

	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else

	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];

	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]];
	  [alert addButtonWithTitle:@"Cancel"];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  //[alert setInformativeText:@"Deleted records cannot be restored."];
	  [alert setAlertStyle:NSAlertStyleWarning];
	  std::function<void(NSInteger)> handleResult =
		  [buttonOnePressed, buttonTwoPressed, cancelPressed, this](NSInteger result) {
			  if (result == NSAlertFirstButtonReturn) {
				  // OK clicked, delete the record
				  app.main.runOnMainThread(buttonOnePressed);
			  } else if (result == NSAlertSecondButtonReturn) {
				  app.main.runOnMainThread(buttonTwoPressed);
			  } else {
				  app.main.runOnMainThread(cancelPressed);
			  }
		  };

#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
        handleResult([alert runModal]);
#		endif
	});
#	endif

#elif defined(__ANDROID__)
	androidTwoOptionCancelDialog(
		title, msg, buttonOneText, buttonOnePressed, buttonTwoText, buttonTwoPressed, cancelPressed);
#elif defined(_WIN32)
	windowsTwoOptionCancelDialog(static_cast<HWND>(app.nativeWindowHandle),
								 title,
								 msg,
								 buttonOneText,
								 buttonOnePressed,
								 buttonTwoText,
								 buttonTwoPressed,
								 cancelPressed);
#elif defined(__linux__)
	linuxTwoOptionCancelDialog(
		title, msg, buttonOneText, buttonOnePressed, buttonTwoText, buttonTwoPressed, cancelPressed);
#endif
}

void Dialogs::twoOptionDialog(std::string title,
							  std::string msg,
							  std::string buttonOneText,
							  std::function<void()> buttonOnePressed,
							  std::string buttonTwoText,
							  std::function<void()> buttonTwoPressed) const {
	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogButtonOnePressed = buttonOnePressed;
		unit_test::dialogs::dialogButtonTwoPressed = buttonTwoPressed;
		unit_test::dialogs::dialogOpen			   = true;
		return;
	}

#ifdef AUTO_TEST
	int i = randi(100) % 2;
	if (i == 0) buttonOnePressed();
	else if (i == 1) buttonTwoPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS

	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonOnePressed(); }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonTwoPressed(); }]];

	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else

	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];

	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  //[alert setInformativeText:@"Deleted records cannot be restored."];
	  [alert setAlertStyle:NSAlertStyleWarning];
	  std::function<void(NSInteger)> handleResult = [buttonOnePressed, buttonTwoPressed, this](NSInteger result) {
		  if (result == NSAlertFirstButtonReturn) {
			  // OK clicked, delete the record
			  app.main.runOnMainThread(buttonOnePressed);
		  } else if (result == NSAlertSecondButtonReturn) {
			  app.main.runOnMainThread(buttonTwoPressed);
		  }
	  };

#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
        handleResult([alert runModal]);
#		endif
	});
#	endif

#elif defined(__ANDROID__)
	androidTwoOptionDialog(title, msg, buttonOneText, buttonOnePressed, buttonTwoText, buttonTwoPressed);
#elif defined(_WIN32)
	//	windowsTwoOptionCancelDialog(title, msg,
	//								 buttonOneText, buttonOnePressed,
	//								 buttonTwoText, buttonTwoPressed, cancelPressed);
	mzAssert(false);
#elif defined(__linux__)
	linuxTwoOptionDialog(title, msg, buttonOneText, buttonOnePressed, buttonTwoText, buttonTwoPressed);
	mzAssert(false);
#endif
}

void Dialogs::threeOptionCancelDialog(std::string title,
									  std::string msg,
									  std::string buttonOneText,
									  std::function<void()> buttonOnePressed,
									  std::string buttonTwoText,
									  std::function<void()> buttonTwoPressed,
									  std::string buttonThreeText,
									  std::function<void()> buttonThreePressed,
									  std::function<void()> cancelPressed) const {
	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogButtonOnePressed	 = buttonOnePressed;
		unit_test::dialogs::dialogButtonTwoPressed	 = buttonTwoPressed;
		unit_test::dialogs::dialogButtonThreePressed = buttonThreePressed;
		unit_test::dialogs::dialogCancelPressed		 = cancelPressed;
		unit_test::dialogs::dialogOpen				 = true;
		return;
	}

#ifdef AUTO_TEST
	int i = randi(100) % 4;
	if (i == 0) buttonOnePressed();
	else if (i == 1) buttonTwoPressed();
	else if (i == 2) buttonThreePressed();
	else cancelPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonOnePressed(); }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonTwoPressed(); }]];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonThreeText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonThreePressed(); }]];

	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
											  style:UIAlertActionStyleCancel
											handler:^(UIAlertAction *action) { cancelPressed(); }]];

	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else

	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];

	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonThreeText.c_str()]];
	  [alert addButtonWithTitle:@"Cancel"];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  //[alert setInformativeText:@"Deleted records cannot be restored."];
	  [alert setAlertStyle:NSAlertStyleWarning];

	  std::function<void(NSInteger)> handleResult =
		  [this, buttonOnePressed, buttonTwoPressed, buttonThreePressed, cancelPressed](NSInteger result) {
			  if (result == NSAlertFirstButtonReturn) {
				  // OK clicked, delete the record
				  app.main.runOnMainThread(buttonOnePressed);
			  } else if (result == NSAlertSecondButtonReturn) {
				  app.main.runOnMainThread(buttonTwoPressed);
			  } else if (result == NSAlertThirdButtonReturn) {
				  app.main.runOnMainThread(buttonThreePressed);
			  } else {
				  app.main.runOnMainThread(cancelPressed);
			  }
		  };

#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
        handleResult([alert runModal]);
#		endif
	});
#	endif

#elif defined(__ANDROID__)
	androidThreeOptionCancelDialog(title,
								   msg,
								   buttonOneText,
								   buttonOnePressed,
								   buttonTwoText,
								   buttonTwoPressed,
								   buttonThreeText,
								   buttonThreePressed,
								   cancelPressed);
#elif defined(_WIN32)
	windowsThreeOptionCancelDialog(static_cast<HWND>(app.nativeWindowHandle),
								   title,
								   msg,
								   buttonOneText,
								   buttonOnePressed,
								   buttonTwoText,
								   buttonTwoPressed,
								   buttonThreeText,
								   buttonThreePressed,
								   cancelPressed);
#elif defined(__linux__)
	linuxThreeOptionCancelDialog(title,
								 msg,
								 buttonOneText,
								 buttonOnePressed,
								 buttonTwoText,
								 buttonTwoPressed,
								 buttonThreeText,
								 buttonThreePressed,
								 cancelPressed);
#endif
}

void Dialogs::threeOptionDialog(std::string title,
								std::string msg,
								std::string buttonOneText,
								std::function<void()> buttonOnePressed,
								std::string buttonTwoText,
								std::function<void()> buttonTwoPressed,
								std::string buttonThreeText,
								std::function<void()> buttonThreePressed) const {
	if (unit_test::dialogs::dialogSpoofingIsEnabled()) {
		unit_test::dialogs::dialogButtonOnePressed	 = buttonOnePressed;
		unit_test::dialogs::dialogButtonTwoPressed	 = buttonTwoPressed;
		unit_test::dialogs::dialogButtonThreePressed = buttonThreePressed;
		unit_test::dialogs::dialogOpen				 = true;
		return;
	}

#ifdef AUTO_TEST
	int i = randi(100) % 3;
	if (i == 0) buttonOnePressed();
	else if (i == 1) buttonTwoPressed();
	else buttonThreePressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert =
		[UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
											message:[NSString stringWithUTF8String:msg.c_str()]
									 preferredStyle:UIAlertControllerStyleAlert];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonOnePressed(); }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonTwoPressed(); }]];

	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String:buttonThreeText.c_str()]
											  style:UIAlertActionStyleDefault
											handler:^(UIAlertAction *action) { buttonThreePressed(); }]];

	[getTopController(app) presentViewController:alert animated:YES completion:nil];

#	else

	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSAlert *alert = [[NSAlert alloc] init];

	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonOneText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonTwoText.c_str()]];
	  [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonThreeText.c_str()]];
	  [alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
	  //[alert setInformativeText:@"Deleted records cannot be restored."];
	  [alert setAlertStyle:NSAlertStyleWarning];

	  std::function<void(NSInteger)> handleResult =
		  [buttonOnePressed, buttonTwoPressed, buttonThreePressed, this](NSInteger result) {
			  if (result == NSAlertFirstButtonReturn) {
				  // OK clicked, delete the record
				  app.main.runOnMainThread(buttonOnePressed);
			  } else if (result == NSAlertSecondButtonReturn) {
				  app.main.runOnMainThread(buttonTwoPressed);
			  } else if (result == NSAlertThirdButtonReturn) {
				  app.main.runOnMainThread(buttonThreePressed);
			  }
		  };

#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
            handleResult([alert runModal]);
#		endif
	});
#	endif

#else
	mzAssert(0);
#endif
}

#if TARGET_OS_IOS
@interface BGPickerDelegate : NSObject <UIImagePickerControllerDelegate> {
	std::function<void(bool success, std::string imgPath)> callback;
}
- (void)setCompletionCallback:(std::function<void(bool success, std::string imgPath)>)cb;
@end

@implementation BGPickerDelegate

- (void)setCompletionCallback:(std::function<void(bool success, std::string imgPath)>)cb {
	callback = cb;
}
- (UIImage *)normalizedImage:(UIImage *)img {
	if (img.imageOrientation == UIImageOrientationUp) return img;

	UIGraphicsBeginImageContextWithOptions(img.size, NO, img.scale);
	[img drawInRect:(CGRect) {0, 0, img.size}];
	UIImage *normalizedImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return normalizedImage;
}

- (void)imagePickerController:(UIImagePickerController *)picker
	didFinishPickingMediaWithInfo:(NSDictionary<UIImagePickerControllerInfoKey, id> *)info {
	NSArray *arr = [info allKeys];
	for (int i = 0; i < [arr count]; i++) {
		NSString *str = [arr objectAtIndex:i];
		NSLog(@"%@", str);
	}
	UIImage *img = [info objectForKey:UIImagePickerControllerEditedImage];
	if (img == nil) {
		img = [info objectForKey:UIImagePickerControllerOriginalImage];
	}

	if (img != nil) {
		// this fixes embedded rotations in the image
		img		   = [self normalizedImage:img];
		CLANG_IGNORE_WARNINGS_BEGIN("-Wdeprecated-declarations")
		NSURL *url = [info objectForKey:UIImagePickerControllerReferenceURL];
		CLANG_IGNORE_WARNINGS_END
		if (url == nil) {
			Log::d() << "UIImagePickerControllerReferenceURL was nil";
			callback(false, "");
		}
		NSString *filename = [url lastPathComponent];

		// save to temp dir
		NSString *path = [NSString
			pathWithComponents:@[ [[[NSFileManager defaultManager] temporaryDirectory] path], filename ]];

		[UIImagePNGRepresentation(img) writeToFile:path atomically:YES];

		[picker dismissViewControllerAnimated:NO completion:nil];
		callback(true, [path UTF8String]);
	} else {
		//		Log::e() << "Failed to open file at " << [vidUrl.absoluteString UTF8String];
		// should call completion and fail
		[picker dismissViewControllerAnimated:NO completion:nil];

		callback(false, "");
	}
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
	// should call completion and fail
	[picker dismissViewControllerAnimated:NO completion:nil];
	callback(false, "");
}

@end

BGPickerDelegate *bgpd = nil;
#	import <CoreServices/CoreServices.h>
#endif

//#include "Image.h"

void Dialogs::chooseImage(std::function<void(bool success, std::string imgPath)> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif
#if TARGET_OS_IOS
	if (bgpd == nil) {
		bgpd = [[BGPickerDelegate alloc] init];
	}
	UIImagePickerController *picker = [[UIImagePickerController alloc] init];
	picker.sourceType				= UIImagePickerControllerSourceTypePhotoLibrary;
	picker.mediaTypes				= [[NSArray alloc] initWithObjects:(NSString *) kUTTypeImage, nil];
	picker.delegate					= (id <UINavigationControllerDelegate, UIImagePickerControllerDelegate>)bgpd;

	[bgpd setCompletionCallback:completionCallback];
	[getTopController(app) presentViewController:picker animated:YES completion:^ {}];

#elif defined(__ANDROID__)
	androidImageDialog(docsPath("tmpImg.jpg"), completionCallback);

#else // defined(__APPLE__)
	loadFile("Please choose an image",
			 {"jpg", "jpeg", "bmp", "gif", "png", "tif", "tiff"},
			 [&, completionCallback](std::string path, bool success) {
				 if (!success) completionCallback(false, "");

				 fs::path p(path);
				 std::string outPath = tempDir() + "/" + p.filename().string();
				 Log::d() << "Copying image to " << outPath;

				 try {
					 fs::copy_file(p, outPath, fs::copy_options::overwrite_existing);
					 completionCallback(true, outPath);
				 } catch (const fs::filesystem_error &err) {
					 Log::d() << "Got filesystem error " << err.what();
					 completionCallback(false, "");
				 }
			 });
#endif
}
#ifdef __APPLE__
#	if TARGET_OS_IOS
#		include "UIBlockButton.h"
#	else
#		include "NSBlockButton.h"
#	endif
#endif

void Dialogs::launchUrlInWebView(std::string url, std::function<void()> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif

#ifdef __APPLE__
	NSURL * (^createURLFromString)(const std::string &) = ^NSURL *(const std::string &urlString) {
	  NSString *nsUrlString = [NSString stringWithUTF8String:urlString.c_str()];
	  NSURL *url			= [NSURL URLWithString:nsUrlString];
	  if ((url && url.scheme && ([url.scheme isEqualToString:@"http"] || [url.scheme isEqualToString:@"https"])
		   && url.host != nil)) {
		  return url;
	  }
	  return [NSURL fileURLWithPath:nsUrlString];
	};

#	if TARGET_OS_IOS
	WKWebView *wv	  = [[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
	NSURL *URL		  = createURLFromString(url);
	NSURLRequest *req = [[NSURLRequest alloc] initWithURL:URL];
	[wv loadRequest:req];
	[wv setTranslatesAutoresizingMaskIntoConstraints:NO];

	UIViewController *targetController = [[UIViewController alloc] init];

	UIBlockButton *butt = [UIBlockButton buttonWithType:UIButtonTypeSystem];
	[butt setTitle:@"Close" forState:UIControlStateNormal];
	butt.frame = CGRectMake(0, 0, 200, 40);
	[butt handleControlEvent:UIControlEventTouchUpInside
				   withBlock:^{
					 [targetController dismissViewControllerAnimated:true completion:nil];
					 if (completionCallback) completionCallback();
				   }];
	[butt setTranslatesAutoresizingMaskIntoConstraints:NO];
	butt.tintColor = [UIColor redColor];

	[targetController.view addSubview:wv];
	[targetController.view addSubview:butt];
	targetController.modalPresentationStyle = UIModalPresentationFormSheet;
	targetController.modalTransitionStyle	= UIModalTransitionStyleFlipHorizontal;

	targetController.view.backgroundColor = [UIColor whiteColor];
	NSDictionary *views					  = NSDictionaryOfVariableBindings(wv, butt);
	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-0-[wv]-0-|"
																				  options:0
																				  metrics:nil
																					views:views]];
	[targetController.view
		addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-10-[butt]-0-[wv]-0-|"
															   options:0
															   metrics:nil
																 views:views]];

	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[butt]-20-|"
																				  options:0
																				  metrics:nil
																					views:views]];

	[getTopController(app) presentViewController:targetController animated:YES completion:nil];
#	else // mac
	dispatch_async(dispatch_get_main_queue(), ^{
	  NSView *rootView = (__bridge NSView *) app.viewHandle;

	  NSView *containerView = [[NSView alloc] initWithFrame:rootView.bounds];
	  // Make contentView background black
	  containerView.wantsLayer			  = YES;
	  containerView.layer.backgroundColor = [NSColor blackColor].CGColor;
	  containerView.autoresizingMask	  = NSViewWidthSizable | NSViewHeightSizable;

	  int buttonHeight = 20;
	  int padding	   = 10;

	  NSRect wvRect =
		  NSMakeRect(0, 0, rootView.bounds.size.width, rootView.bounds.size.height - buttonHeight - padding * 2);

	  // Instantiate WKWebView on the main thread
	  WKWebView *webView	   = [[WKWebView alloc] initWithFrame:wvRect];
	  webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

	  // Load content on the main thread but asynchronously to avoid blocking the UI
	  NSURL *nsUrl			= createURLFromString(url);
	  NSURLRequest *request = [NSURLRequest requestWithURL:nsUrl];
	  [webView loadRequest:request];

	  // Create and configure the close button
	  NSBlockButton *closeButton =
		  [[NSBlockButton alloc] initWithFrame:NSMakeRect(rootView.bounds.size.width - 30,
														  rootView.bounds.size.height - buttonHeight - padding,
														  buttonHeight,
														  buttonHeight)];
	  closeButton.title = @"✕";
	  [closeButton setButtonType:NSButtonTypeMomentaryPushIn];
	  [closeButton setBezelStyle:NSBezelStyleInline];
	  closeButton.font			   = [NSFont systemFontOfSize:16];
	  closeButton.bordered		   = NO;
	  closeButton.autoresizingMask = NSViewMinXMargin | NSViewMinYMargin;

	  __weak NSView *weakContainerView = containerView;
	  [closeButton setActionBlock:^{
		// Create slide-out animation
		NSRect endFrame = weakContainerView.frame;
		endFrame.origin.y -= weakContainerView.bounds.size.height;

		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration				   = 0.5;
		  weakContainerView.animator.frame = endFrame;
		}
			completionHandler:^{ [weakContainerView removeFromSuperview]; }];
	  }];
	  [containerView addSubview:webView];
	  [containerView addSubview:closeButton];
	  [rootView addSubview:containerView];

	  // Initial off-screen position for animation
	  NSRect startFrame = containerView.frame;
	  NSRect endFrame	= startFrame;
	  startFrame.origin.y -= startFrame.size.height;

	  containerView.frame = startFrame;

	  // Animate the web view sliding in
	  [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		context.duration				 = 0.5;
		weakContainerView.animator.frame = endFrame;
	  }];
	});
#	endif
#else
	launchUrl(url);
#endif
}
#ifdef __APPLE__
@interface OpenLinksInSafariDelegate : NSObject <WKNavigationDelegate>
@end
;
@implementation OpenLinksInSafariDelegate

- (void)webView:(WKWebView *)webView
	decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
					decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
	// this handles the initial case of the html being loaded, which uses a url of "about:blank"
	// - all other links should go to safari by cancelling the navigation action.
	if ([[navigationAction.request.URL absoluteString] isEqualToString:@"about:blank"]) {
		decisionHandler(WKNavigationActionPolicyAllow);
		return;
	}
	decisionHandler(WKNavigationActionPolicyCancel);

#	if TARGET_OS_IOS
	[[UIApplication sharedApplication] openURL:navigationAction.request.URL options:@{} completionHandler:nil];
	NSLog(@"%@", navigationAction.request.URL);
#	else
	[[NSWorkspace sharedWorkspace] openURL:navigationAction.request.URL];
#	endif
}

@end
OpenLinksInSafariDelegate *navDelegate = nil;
#endif
void Dialogs::displayHtmlInWebView(const std::string &html, std::function<void()> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif

#ifdef __APPLE__

#	if TARGET_OS_IOS

	WKWebView *wv	  = [[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
	NSString *htmlStr = [NSString stringWithUTF8String:html.c_str()];
	[wv loadHTMLString:htmlStr baseURL:nil];
	[wv setTranslatesAutoresizingMaskIntoConstraints:NO];
	navDelegate			  = [[OpenLinksInSafariDelegate alloc] init];
	wv.navigationDelegate = navDelegate;
	//////////////////////////

	UIViewController *targetController = [[UIViewController alloc] init];

	UIBlockButton *butt = [UIBlockButton buttonWithType:UIButtonTypeSystem];
	[butt setTitle:@"Close" forState:UIControlStateNormal];
	butt.frame = CGRectMake(0, 0, 200, 40);
	[butt handleControlEvent:UIControlEventTouchUpInside
				   withBlock:^{
					 [targetController dismissViewControllerAnimated:true completion:nil];
					 if (completionCallback) completionCallback();
				   }];
	[butt setTranslatesAutoresizingMaskIntoConstraints:NO];
	butt.tintColor = [UIColor redColor];

	targetController.view.backgroundColor = [UIColor blackColor];
	//	wv.backgroundColor = [UIColor blackColor];

	[targetController.view addSubview:wv];
	[targetController.view addSubview:butt];
	targetController.modalPresentationStyle = UIModalPresentationFormSheet;
	targetController.modalTransitionStyle	= UIModalTransitionStyleFlipHorizontal;

	//	targetController.view.backgroundColor = [UIColor whiteColor];
	NSDictionary *views = NSDictionaryOfVariableBindings(wv, butt);
	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-0-[wv]-0-|"
																				  options:0
																				  metrics:nil
																					views:views]];
	[targetController.view
		addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-10-[butt]-0-[wv]-0-|"
															   options:0
															   metrics:nil
																 views:views]];

	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[butt]-20-|"
																				  options:0
																				  metrics:nil
																					views:views]];

	[getTopController(app) presentViewController:targetController animated:YES completion:nil];
#	else // mac

	NSString *tmpPath = [NSString stringWithFormat:@"%@/index.html", NSTemporaryDirectory()];

	writeStringToFile([tmpPath UTF8String], html);
	NSURL *url = [NSURL fileURLWithPath:tmpPath];
	launchUrl([[url absoluteString] UTF8String]);
	if (completionCallback) completionCallback();
#	endif
#elif defined(__ANDROID__)
	androidDisplayHtml(html);
#else
	// other impls here
#endif
}

void Dialogs::share(std::string message, std::string path, std::function<void(bool)> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif

#if TARGET_OS_IOS
	NSURL *URL	  = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];

	UIActivityViewController *activityViewController =
		[[UIActivityViewController alloc] initWithActivityItems:@[ URL ] applicationActivities:nil];

	UIViewController *vc = getTopController(app);

	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
		[vc presentViewController:activityViewController animated:YES completion:^{ completionCallback(true); }];
	}
	//if iPad
	else {
		// Change Rect to position Popover
		CLANG_IGNORE_WARNINGS_BEGIN("-Wdeprecated-declarations")
		// TODO: Replace this with the new style UIViewController presentation
		UIPopoverController *popup =
			[[UIPopoverController alloc] initWithContentViewController:activityViewController];
		CLANG_IGNORE_WARNINGS_END

		[popup

			  presentPopoverFromRect:CGRectMake(vc.view.frame.size.width / 2, vc.view.frame.size.height / 4, 0, 0)
							  inView:vc.view
			permittedArrowDirections:UIPopoverArrowDirectionAny
							animated:YES];
	}

#elif defined(__ANDROID__)
	androidShareDialog(message, path, completionCallback);
#else

	std::string destPath = "/Users/marek/Desktop/" + fs::path(path).filename().string();
	printf("No sharing pane on mac for now - saved to desktop\n");
	fs::ifstream src(u8path(path), std::ios::binary);
	printf("Copying %s to %s\n", path.c_str(), destPath.c_str());
	fs::ofstream dst(u8path(destPath), std::ios::binary);

	dst << src.rdbuf();
#endif
}

#if defined(__APPLE__)

#	if TARGET_OS_IOS

@interface FilePickerDelegate : NSObject <UIDocumentPickerDelegate> {
@public
	std::function<void(std::string, bool)> completionCallback;
}
@end

@implementation FilePickerDelegate {
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller
	didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
	for (int i = 0; i < [urls count]; i++) {
		NSURL *url		 = [urls objectAtIndex:i];
		std::string path = [[url path] UTF8String];
		completionCallback(path, true);
	}
}

@end

FilePickerDelegate *fpd = nil;

#	else
#		import "MacFilePickerDelegate.h"
FilePickerDelegate *impikD = nil;
#	endif
#endif
void Dialogs::loadFile(std::string msg, std::function<void(std::string, bool)> completionCallback) const {
	loadFile(msg, {}, completionCallback);
}

void Dialogs::loadFile(std::string msg,
					   const std::vector<std::string> &allowedExtensions,
					   std::function<void(std::string, bool)> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif

#ifdef _WIN32
	windowsChooseEntryDialog(static_cast<HWND>(app.nativeWindowHandle), true, msg, completionCallback);

#elif defined(__APPLE__)

#	if TARGET_OS_IOS

	if (fpd == nil) {
		fpd = [[FilePickerDelegate alloc] init];
	}
	fpd->completionCallback = completionCallback;

	NSMutableArray<NSString *> *docTypes = [[NSMutableArray alloc] init];

	// catchall
	if (allowedExtensions.size() == 0) {
		[docTypes addObject:(NSString *) kUTTypeData];
	}

	for (const auto &ext: allowedExtensions) {
		if (ext == "json") {
			[docTypes addObject:(NSString *) kUTTypeJSON];
		} else if (ext == "bmp") {
			[docTypes addObject:(NSString *) kUTTypeBMP];
		} else if (ext == "gif") {
			[docTypes addObject:(NSString *) kUTTypeGIF];
		} else if (ext == "pdf") {
			[docTypes addObject:(NSString *) kUTTypePDF];
		} else if (ext == "png") {
			[docTypes addObject:(NSString *) kUTTypePNG];
		} else if (ext == "rtf") {
			[docTypes addObject:(NSString *) kUTTypeRTF];
		} else if (ext == "xml") {
			[docTypes addObject:(NSString *) kUTTypeXML];
		} else if (ext == "mp3") {
			[docTypes addObject:(NSString *) kUTTypeMP3];
		} else if (ext == "ttf") {
			[docTypes addObject:(NSString *) kUTTypeFont];
		} else if (ext == "html" || ext == "htm") {
			[docTypes addObject:(NSString *) kUTTypeHTML];
		} else if (ext == "jpg" || ext == "jpeg") {
			[docTypes addObject:(NSString *) kUTTypeJPEG];
		} else if (ext == "mov") {
			[docTypes addObject:(NSString *) kUTTypeMPEG];
		} else if (ext == "tiff") {
			[docTypes addObject:(NSString *) kUTTypeTIFF];
		} else if (ext == "txt") {
			[docTypes addObject:(NSString *) kUTTypeText];
		} else if (ext == "mp4" || ext == "mov") {
			[docTypes addObject:(NSString *) kUTTypeMPEG4];
		} else if (ext == "avi") {
			[docTypes addObject:(NSString *) kUTTypeAVIMovie];
		} else if (ext == "wav") {
			[docTypes addObject:(NSString *) kUTTypeWaveformAudio];
		} else if (ext == "aif" || ext == "aiff") {
			[docTypes addObject:(NSString *) kUTTypeAudioInterchangeFileFormat];
		} else if (ext == "m4a") {
			[docTypes addObject:(NSString *) kUTTypeMPEG4Audio];
		} else {
			Log::e() << "Can't find uttype for extension" << ext;
			[docTypes addObject:(NSString *) kUTTypeData];
		}
	}
	//= @[(NSString*)kUTTypeAudio, (NSString*)kUTTypeVideo, (NSString*)kUTTypeQuickTimeMovie, (NSString*)kUTTypeMPEG4];
	UIDocumentPickerViewController *filePicker =
		[[UIDocumentPickerViewController alloc] initWithDocumentTypes:docTypes inMode:UIDocumentPickerModeImport];

	filePicker.delegate = fpd;

	//	if (@available(iOS 11.0, *)) {
	//		filePicker.allowsMultipleSelection = YES;
	//	}

	[getTopController(app) presentViewController:filePicker
										animated:YES
									  completion:^ {
										  //		if (@available(iOS 11.0, *)) {
										  //			filePicker.allowsMultipleSelection = YES;
										  //		}
									  }];

#	else
	auto allowedExts = allowedExtensions;
	dispatch_async(dispatch_get_main_queue(), ^{
	  // do work here
	  NSInteger buttonClicked;
	  std::string filePath = "";
	  @autoreleasepool {
		  NSOpenPanel *loadDialog = [NSOpenPanel openPanel];

		  if (allowedExts.size() > 0) {
			  if (impikD == nil) {
				  impikD = [[FilePickerDelegate alloc] init];
			  }
			  std::vector<NSString *> nsExts;
			  for (auto ext: allowedExts) {
				  if (ext[0] == '.') {
					  ext.erase(0, 1);
				  }
				  nsExts.push_back([NSString stringWithUTF8String:ext.c_str()]);
			  }
			  NSArray *exts = [NSArray arrayWithObjects:&nsExts[0] count:nsExts.size()];
			  [impikD setAllowedExtensions:exts];
			  [impikD enableFoldersOnly:NO];
			  loadDialog.delegate = impikD;
		  }

		  NSOpenGLContext *context = [NSOpenGLContext currentContext];
		  [loadDialog setMessage:[NSString stringWithUTF8String:msg.c_str()]];
		  //			[Dialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileName.c_str()]];

		  buttonClicked = [loadDialog runModal];

		  [context makeCurrentContext];

		  if (buttonClicked == NSModalResponseOK) {
			  filePath = std::string([[[loadDialog URL] path] UTF8String]);
		  }
	  }
	  completionCallback(filePath, buttonClicked == NSModalResponseOK);
	});
#	endif
#elif defined(__ANDROID__)
	androidFileDialog(msg, allowedExtensions, completionCallback);
#elif defined(__linux__)
	linuxLoadFileDialog(msg, allowedExtensions, completionCallback);
#endif
}

void Dialogs::chooseFolder(std::string msg, std::function<void(std::string, bool)> completionCallback) const {
#if !defined(__ANDROID__) && defined(__linux__)

	linuxChooseFolderDialog(msg, completionCallback);

#elif (WIN32)
	windowsChooseEntryDialog(static_cast<HWND>(app.nativeWindowHandle), false, msg, completionCallback);
#elif defined(__APPLE__)
#	if TARGET_OS_IOS

#	else
	dispatch_async(dispatch_get_main_queue(), ^{
	  @autoreleasepool {
		  NSOpenPanel *loadDialog		  = [NSOpenPanel openPanel];
		  loadDialog.canChooseFiles		  = NO;
		  loadDialog.canChooseDirectories = YES;
		  loadDialog.message			  = [NSString stringWithUTF8String:msg.c_str()];
		  NSInteger buttonClicked		  = [loadDialog runModal];
		  completionCallback([[[loadDialog URL] path] cStringUsingEncoding:NSUTF8StringEncoding],
							 buttonClicked == NSModalResponseOK);
	  }
	});
#	endif
#endif
}

#if defined(__APPLE__) && TARGET_OS_IOS
#	include "TextboxSegmentedViewController.h"
#endif

void Dialogs::textboxWithSegmented(std::string title,
								   std::string msg,
								   std::string text,
								   std::vector<std::string> options,
								   int defaultOption,
								   std::function<void(std::string, int, bool)> completionCallback) const {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	NSString *deviceType = [UIDevice currentDevice].model;

	TextboxSegmentedViewController *alertVC =
		[[TextboxSegmentedViewController alloc] initWithTitle:[NSString stringWithUTF8String:title.c_str()]
													  message:[NSString stringWithUTF8String:msg.c_str()]
														 text:[NSString stringWithUTF8String:text.c_str()]
													  options:@[ deviceType, @"iCloud" ]
													 selected:defaultOption];
	alertVC.completionHandler = ^(NSString *filename, NSInteger selectedSegment) {
	  dispatch_async(dispatch_get_main_queue(),
					 ^{ completionCallback([filename UTF8String], static_cast<int>(selectedSegment), true); });
	};

	alertVC.modalPresentationStyle = UIModalPresentationOverFullScreen;
	alertVC.modalTransitionStyle   = UIModalTransitionStyleCrossDissolve;

	[getTopController(app) presentViewController:alertVC animated:YES completion:nil];

#	else
	dispatch_async(dispatch_get_main_queue(), ^{
	  // Create an alert
	  NSAlert *alert = [[NSAlert alloc] init];
	  [alert setMessageText:[NSString stringWithCString:title.c_str() encoding:NSUTF8StringEncoding]];
	  [alert setInformativeText:[NSString stringWithCString:msg.c_str() encoding:NSUTF8StringEncoding]];
	  [alert addButtonWithTitle:@"OK"];
	  [alert addButtonWithTitle:@"Cancel"];

	  // Create a text field
	  NSTextField *textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
	  [textField setPlaceholderString:@""];
	  [textField setStringValue:[NSString stringWithUTF8String:text.c_str()]];

	  // Create a container view for the accessory view
	  NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 60)];

	  // Create a segmented control for choosing save location
	  NSSegmentedControl *locationControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect(50, 0, 200, 24)];
	  [locationControl setSegmentCount:options.size()];

	  for (int i = 0; i < options.size(); i++) {
		  [locationControl setLabel:[NSString stringWithUTF8String:options[i].c_str()] forSegment:i];
	  }
	  [locationControl setSelectedSegment:defaultOption];

	  // Add the text field and segmented control to the accessory view
	  [accessoryView addSubview:locationControl];
	  [accessoryView addSubview:textField];

	  // Adjust the text field's position
	  NSRect textFieldFrame	  = [textField frame];
	  textFieldFrame.origin.y = locationControl.frame.size.height + 10;
	  [textField setFrame:textFieldFrame];

	  // Set the accessory view of the alert to the container view
	  [alert setAccessoryView:accessoryView];

	  std::function<void(NSInteger, NSTextField *, NSSegmentedControl *)> handleResult =
		  [completionCallback, this](NSInteger returnCode, NSTextField *label, NSSegmentedControl *seg) {
			  std::string txt			= "";
			  NSInteger selectedSegment = [seg selectedSegment];

			  if (returnCode == NSAlertFirstButtonReturn) {
				  txt = [[label stringValue] UTF8String];
			  }
			  app.main.runOnMainThread(true, [txt, returnCode, selectedSegment, completionCallback]() {
				  completionCallback(txt, static_cast<int>(selectedSegment), returnCode == NSAlertFirstButtonReturn);
			  });
		  };
#		ifndef MZGLAU
	  [alert beginSheetModalForWindow:[NSApp mainWindow]
					completionHandler:^(NSInteger result) { handleResult(result, textField, locationControl); }];
	  [textField becomeFirstResponder];
#		else
            [label becomeFirstResponder];
            handleResult([alert runModal], label, locationControl);
#		endif
	});
#	endif
#else
	alert("Not implemented", "Not implemented on this platform");
#endif
}
