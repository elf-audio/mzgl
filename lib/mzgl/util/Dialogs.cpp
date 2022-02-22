#include "Dialogs.h"
#include "log.h"
#include "App.h"

#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		import <UIKit/UIKit.h>
#		import <WebKit/WebKit.h>
#	else
#		import <AppKit/AppKit.h>
#	endif
#elif defined(__ANDROID__)
#   include "androidUtil.h"
#elif defined(_WIN32)
#include "winUtil.h"
#elif defined(__linux__)
#include "linuxUtil.h"
#endif

using namespace std;

#include "filesystem.h"


#ifdef UNIT_TEST
namespace unit_test {
    function<void()> dialogOkPressed;
    function<void()> dialogCancelPressed;
    function<void()> dialogButtonOnePressed;
    function<void()> dialogButtonTwoPressed;
    function<void()> dialogButtonThreePressed;

    bool dialogOpen = false;
    bool isDialogOpen() { return dialogOpen; }
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
}
#endif




void Dialogs::textbox(std::string title, std::string msg, std::string text, function<void(string, bool)> completionCallback) const {
#ifdef AUTO_TEST
	completionCallback(title, randi(2)==0);
	return;
#endif
	
#ifdef __APPLE__
#   if TARGET_OS_IOS

	
	
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		UITextField *alertTextField = alert.textFields.firstObject;
		completionCallback([alertTextField.text UTF8String], true);
	}]];
	[alert addAction:[UIAlertAction actionWithTitle: @"Cancel"
											  style:UIAlertActionStyleCancel handler:
					  ^(UIAlertAction * action) {
						  completionCallback("", false);
					  }
					  ]];
	
	
	
	[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
		textField.placeholder = [NSString stringWithUTF8String:text.c_str()];
	}];
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
#   else
	dispatch_async(dispatch_get_main_queue(), ^{
		// do work here
	
		// create alert dialog
		NSAlert *alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"OK"];
		[alert addButtonWithTitle:@"Cancel"];
		[alert setMessageText:[NSString stringWithCString:msg.c_str()
												 encoding:NSUTF8StringEncoding]];
		// create text field
		NSTextField* label = [[NSTextField alloc] initWithFrame:NSRectFromCGRect(CGRectMake(0,0,300,40))];
		[label setStringValue:[NSString stringWithCString:text.c_str()
												 encoding:NSUTF8StringEncoding]];
		// add text field to alert dialog
		[alert setAccessoryView:label];
	

		
		
		function<void(NSInteger, NSTextField*)> handleResult = [completionCallback](NSInteger returnCode, NSTextField *label) {
			string txt = "";
			if ( returnCode == NSAlertFirstButtonReturn )
				txt = [[label stringValue] UTF8String];
//			[label resignFirstResponder];
//			[[[NSApp mainWindow].contentView.subviews firstObject] becomeFirstResponder];
			completionCallback(txt, returnCode == NSAlertFirstButtonReturn);
		};
#ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
			handleResult(result, label);

		}];
		[label becomeFirstResponder];
#else
		[label becomeFirstResponder];
		
		handleResult([alert runModal], label);
#endif
		
		
		
//
//		NSInteger returnCode = [alert runModal];
//		handleResult(returnCode, label);
//
		
		// if OK was clicked, assign value to text
		
	});
#   endif
#elif defined(__ANDROID__)
	androidTextboxDialog(title, msg, text, completionCallback);
#elif defined(_WIN32)
		windowsTextboxDialog(title, msg, text, completionCallback);
#elif defined(__linux__)
	linuxTextboxDialog(title, msg, text, completionCallback);
#endif
}


void Dialogs::confirm(std::string title, std::string msg,
				   std::function<void()> okPressed,
				   std::function<void()> cancelPressed) const {
#ifdef AUTO_TEST
	int i = randi(100)%2;
	if(i==0) okPressed();
	else cancelPressed();
	return;
#endif
	
#ifdef UNIT_TEST
	unit_test::dialogOkPressed = okPressed;
	unit_test::dialogCancelPressed = cancelPressed;
	unit_test::dialogOpen = true;
	return;
#endif
	
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		okPressed();
	}]];
	[alert addAction:[UIAlertAction actionWithTitle: @"Cancel"
											  style:UIAlertActionStyleCancel handler:
					  ^(UIAlertAction * action) {
						  cancelPressed();
					  }
					  ]];
	
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
	
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
		
		function<void(NSInteger)> handleResult = [okPressed, cancelPressed](NSInteger result) {
			if (result == NSAlertFirstButtonReturn) {
				runOnMainThread(okPressed);
			} else {
				runOnMainThread(cancelPressed);
			}
		};
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
		handleResult([alert runModal]);
#		endif
		

		
	});
#	endif
#elif defined(__ANDROID__)
	androidConfirmDialog(title, msg, okPressed, cancelPressed);
#elif defined(_WIN32)
	Log::d() << "Calling here";
	windowsConfirmDialog(title, msg, okPressed, cancelPressed);
#elif defined(__linux__)
	linuxConfirmDialog(title, msg, okPressed, cancelPressed);
#endif
}


void Dialogs::alert(std::string title, std::string msg) const {
	Log::d() << "alertDialog("<<title<<", "<<msg<<")";
#ifdef AUTO_TEST
	return;
#endif
	
#if defined(UNIT_TEST)
	unit_test::dialogOpen = true;
	return;
#endif
#ifdef _WIN32
		// we need to convert error message to a wide char message.
		// first, figure out the length and allocate a wchar_t at that length + 1 (the +1 is for a terminating character)
		int length = strlen(msg.c_str());
		wchar_t * widearray = new wchar_t[length+1];
		memset(widearray, 0, sizeof(wchar_t)*(length+1));
		// then, call mbstowcs:
		// http://www.cplusplus.com/reference/clibrary/cstdlib/mbstowcs/
		mbstowcs(widearray, msg.c_str(), length);
		// launch the alert:
		MessageBoxW(NULL, widearray, L"alert", MB_OK);
		// clear the allocated memory:
		delete [] widearray;
#elif defined(__APPLE__)

#	if TARGET_OS_IOS
	dispatch_async(dispatch_get_main_queue(), ^{
		
		
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
		
		[alert addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
			//okPressed();
		}]];
		
		[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	});
	
	
#	else
	dispatch_async(dispatch_get_main_queue(), ^{
		NSAlert *alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"OK"];
		[alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
		[alert setAlertStyle:NSAlertStyleWarning];
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
			NSLog(@"Success");
		}];
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


void Dialogs::twoOptionCancelDialog(std::string title, std::string msg,
						   std::string buttonOneText, std::function<void()> buttonOnePressed,
						   std::string buttonTwoText, std::function<void()> buttonTwoPressed,
				   std::function<void()> cancelPressed) const {
	
	
#ifdef UNIT_TEST
	unit_test::dialogButtonOnePressed = buttonOnePressed;
	unit_test::dialogButtonTwoPressed = buttonTwoPressed;
	unit_test::dialogCancelPressed = cancelPressed;
	unit_test::dialogOpen = true;
	return;
#endif
	
#ifdef AUTO_TEST
	int i = randi(100)%3;
	if(i==0) buttonOnePressed();
	else if(i==1) buttonTwoPressed();
	else cancelPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonOnePressed();
											  }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonTwoPressed();
											  }]];
	
	
	[alert addAction:[UIAlertAction actionWithTitle: @"Cancel"
											  style:UIAlertActionStyleCancel handler:
					  ^(UIAlertAction * action) {
						  cancelPressed();
					  }
					  ]];
	
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
	
#	else
	
	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{

		NSAlert *alert = [[NSAlert alloc] init];
		
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]];
		[alert addButtonWithTitle: @"Cancel"];
		[alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
		//[alert setInformativeText:@"Deleted records cannot be restored."];
		[alert setAlertStyle:NSAlertStyleWarning];
		function<void(NSInteger)> handleResult = [buttonOnePressed, buttonTwoPressed, cancelPressed](NSInteger result) {
			if (result == NSAlertFirstButtonReturn) {
				// OK clicked, delete the record
				runOnMainThread(buttonOnePressed);
			} else if(result == NSAlertSecondButtonReturn) {
				runOnMainThread(buttonTwoPressed);
			} else {
				runOnMainThread(cancelPressed);
			}
		};
		
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
		handleResult([alert runModal]);
#		endif
		
	});
#	endif
	
	
#elif defined(__ANDROID__)
	androidTwoOptionCancelDialog(title, msg,
			buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, cancelPressed);
#elif defined(_WIN32)
	windowsTwoOptionCancelDialog(title, msg,
								 buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, cancelPressed);
#elif defined(__linux__)
linuxTwoOptionCancelDialog(title, msg,
								 buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, cancelPressed);
#endif

}



void Dialogs::twoOptionDialog(std::string title, std::string msg,
						   std::string buttonOneText, std::function<void()> buttonOnePressed,
						   std::string buttonTwoText, std::function<void()> buttonTwoPressed) const {
	
	
#ifdef UNIT_TEST
	unit_test::dialogButtonOnePressed = buttonOnePressed;
	unit_test::dialogButtonTwoPressed = buttonTwoPressed;
	unit_test::dialogOpen = true;
	return;
#endif
	
#ifdef AUTO_TEST
	int i = randi(100)%2;
	if(i==0) buttonOnePressed();
	else if(i==1) buttonTwoPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonOnePressed();
											  }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonTwoPressed();
											  }]];
	
	
	
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
	
#	else
	
	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{

		NSAlert *alert = [[NSAlert alloc] init];
		
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]];
		[alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
		//[alert setInformativeText:@"Deleted records cannot be restored."];
		[alert setAlertStyle:NSAlertStyleWarning];
		function<void(NSInteger)> handleResult = [buttonOnePressed, buttonTwoPressed](NSInteger result) {
			if (result == NSAlertFirstButtonReturn) {
				// OK clicked, delete the record
				runOnMainThread(buttonOnePressed);
			} else if(result == NSAlertSecondButtonReturn) {
				runOnMainThread(buttonTwoPressed);
			}
		};
		
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
		handleResult([alert runModal]);
#		endif
		
	});
#	endif
	
	
#elif defined(__ANDROID__)
//	androidTwoOptionCancelDialog(title, msg,
//			buttonOneText, buttonOnePressed,
//								 buttonTwoText, buttonTwoPressed, cancelPressed);
	mzAssert(false);
#elif defined(_WIN32)
//	windowsTwoOptionCancelDialog(title, msg,
//								 buttonOneText, buttonOnePressed,
//								 buttonTwoText, buttonTwoPressed, cancelPressed);
	mzAssert(false);
#elif defined(__linux__)
linuxTwoOptionDialog(title, msg,
								 buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed);
	mzAssert(false);
#endif

}





void Dialogs::threeOptionCancelDialog(std::string title, std::string msg,
						   std::string buttonOneText, std::function<void()> buttonOnePressed,
						   std::string buttonTwoText, std::function<void()> buttonTwoPressed,
							 std::string buttonThreeText, std::function<void()> buttonThreePressed,
				   std::function<void()> cancelPressed) const {

#ifdef UNIT_TEST
	unit_test::dialogButtonOnePressed = buttonOnePressed;
	unit_test::dialogButtonTwoPressed = buttonTwoPressed;
	unit_test::dialogButtonTwoPressed = buttonThreePressed;
	unit_test::dialogCancelPressed = cancelPressed;
	unit_test::dialogOpen = true;
	return;
#endif
	
#ifdef AUTO_TEST
	int i = randi(100)%4;
	if(i==0) buttonOnePressed();
	else if(i==1) buttonTwoPressed();
	else if(i==2) buttonThreePressed();
	else cancelPressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonOnePressed();
											  }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]
	style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		buttonTwoPressed();
	}]];
	
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonThreeText.c_str()]
	style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		buttonThreePressed();
	}]];
	
	
	[alert addAction:[UIAlertAction actionWithTitle: @"Cancel"
											  style:UIAlertActionStyleCancel handler:
					  ^(UIAlertAction * action) {
						  cancelPressed();
					  }
					  ]];
	
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
	
#	else
	
	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{

		NSAlert *alert = [[NSAlert alloc] init];
		
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonThreeText.c_str()]];
		[alert addButtonWithTitle: @"Cancel"];
		[alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
		//[alert setInformativeText:@"Deleted records cannot be restored."];
		[alert setAlertStyle:NSAlertStyleWarning];
		
		function<void(NSInteger)> handleResult = [buttonOnePressed, buttonTwoPressed, buttonThreePressed, cancelPressed](NSInteger result) {
			if (result == NSAlertFirstButtonReturn) {
				// OK clicked, delete the record
				runOnMainThread(buttonOnePressed);
			} else if(result == NSAlertSecondButtonReturn) {
				runOnMainThread(buttonTwoPressed);
			} else if(result == NSAlertThirdButtonReturn) {
				runOnMainThread(buttonThreePressed);
			} else {
				runOnMainThread(cancelPressed);
			}
		};
		
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { handleResult(result); }];
#		else
		handleResult([alert runModal]);
#		endif
		
		
		
	});
#	endif
	
	
#elif defined(__ANDROID__)
	androidThreeOptionCancelDialog(title, msg,
			buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, buttonThreeText, buttonThreePressed, cancelPressed);
#elif defined(_WIN32)
	windowsThreeOptionCancelDialog(title, msg,
			buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, buttonThreeText, buttonThreePressed, cancelPressed);
#elif defined(__linux__)
linuxThreeOptionCancelDialog(title, msg,
			buttonOneText, buttonOnePressed,
								 buttonTwoText, buttonTwoPressed, buttonThreeText, buttonThreePressed, cancelPressed);
#endif


}

void Dialogs::threeOptionDialog(std::string title, std::string msg,
						   std::string buttonOneText, std::function<void()> buttonOnePressed,
						   std::string buttonTwoText, std::function<void()> buttonTwoPressed,
							 std::string buttonThreeText, std::function<void()> buttonThreePressed) const {

#ifdef UNIT_TEST
	unit_test::dialogButtonOnePressed = buttonOnePressed;
	unit_test::dialogButtonTwoPressed = buttonTwoPressed;
	unit_test::dialogButtonTwoPressed = buttonThreePressed;
	unit_test::dialogOpen = true;
	return;
#endif
	
#ifdef AUTO_TEST
	int i = randi(100)%3;
	if(i==0) buttonOnePressed();
	else if(i==1) buttonTwoPressed();
	else buttonThreePressed();
	return;
#endif
#ifdef __APPLE__
#	if TARGET_OS_IOS
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String: title.c_str()] message:[NSString stringWithUTF8String: msg.c_str()] preferredStyle:UIAlertControllerStyleAlert];
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]
											  style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
												  buttonOnePressed();
											  }]];
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]
	style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		buttonTwoPressed();
	}]];
	
	
	[alert addAction:[UIAlertAction actionWithTitle:[NSString stringWithUTF8String: buttonThreeText.c_str()]
	style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
		buttonThreePressed();
	}]];
	

	
	[((__bridge UIViewController*)app.viewController) presentViewController:alert animated:YES completion:nil];
	
	
	
#	else
	
	// this dispatch is slightly different from runOnMainThread on OS X
	// runOnMainThread actually runs on the renderer thread not the real
	// main thread, but this Cocoa stuff needs the real main thread.
	dispatch_async(dispatch_get_main_queue(), ^{

		NSAlert *alert = [[NSAlert alloc] init];
		
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonOneText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonTwoText.c_str()]];
		[alert addButtonWithTitle:[NSString stringWithUTF8String: buttonThreeText.c_str()]];
		[alert setMessageText:[NSString stringWithUTF8String:msg.c_str()]];
		//[alert setInformativeText:@"Deleted records cannot be restored."];
		[alert setAlertStyle:NSAlertStyleWarning];
		
		function<void(NSInteger)> handleResult = [buttonOnePressed, buttonTwoPressed, buttonThreePressed](NSInteger result) {
			if (result == NSAlertFirstButtonReturn) {
				// OK clicked, delete the record
				runOnMainThread(buttonOnePressed);
			} else if(result == NSAlertSecondButtonReturn) {
				runOnMainThread(buttonTwoPressed);
			} else if(result == NSAlertThirdButtonReturn) {
				runOnMainThread(buttonThreePressed);
			}
		};
		
#		ifndef MZGLAU
		[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { handleResult(result); }];
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
	std::function<void(bool success, string imgPath)> callback;
}
- (void) setCompletionCallback: (std::function<void(bool success, string imgPath)>) cb;
@end

@implementation BGPickerDelegate

- (void) setCompletionCallback: (std::function<void(bool success, string imgPath)>) cb {
	callback = cb;
}
- (UIImage *)normalizedImage: (UIImage *)img {
	if (img.imageOrientation == UIImageOrientationUp) return img;

	UIGraphicsBeginImageContextWithOptions(img.size, NO, img.scale);
	[img drawInRect:(CGRect){0, 0, img.size}];
	UIImage *normalizedImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return normalizedImage;
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary<UIImagePickerControllerInfoKey, id> *)info {


	NSArray *arr = [info allKeys];
	for(int i = 0; i < [arr count]; i++) {
		NSString *str = [arr objectAtIndex:i];
		NSLog(@"%@", str);
	}
	UIImage *img = [info objectForKey:UIImagePickerControllerEditedImage];
	if(img==nil) {
		img = [info objectForKey:UIImagePickerControllerOriginalImage];
	}
	
	if(img!=nil) {
		// this fixes embedded rotations in the image
		img = [self normalizedImage: img];
		NSURL *url = [info objectForKey:UIImagePickerControllerReferenceURL];
		if(url==nil) {
			Log::d() << "UIImagePickerControllerReferenceURL was nil";
			callback(false, "");
		}
		NSString *filename = [url lastPathComponent];

		// save to temp dir
		NSString *path = [NSString pathWithComponents:
						  @[
							  [[[NSFileManager defaultManager] temporaryDirectory] path],
							  filename
						  ]
						  ];
		
		[UIImagePNGRepresentation(img) writeToFile: path atomically:YES];
		
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
#import <CoreServices/CoreServices.h>
#endif


//#include "Image.h"

void Dialogs::chooseImage(std::function<void(bool success, string imgPath)> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif
#if TARGET_OS_IOS
	if(bgpd==nil) {
		bgpd = [[BGPickerDelegate alloc] init];
	}
	UIImagePickerController *picker = [[UIImagePickerController alloc] init];
	picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
	picker.mediaTypes = [[NSArray alloc] initWithObjects: (NSString *) kUTTypeImage, nil];
	picker.delegate = bgpd;
	
	[bgpd setCompletionCallback: completionCallback];
	UIViewController *vc = ((__bridge UIViewController*)app.viewController);

	[vc presentViewController:picker animated: YES completion:^{}];
	
#elif defined(__ANDROID__)
	androidImageDialog(docsPath("tmpImg.jpg"), completionCallback);

#else // defined(__APPLE__)
	loadFile("Please choose an image", {"jpg", "jpeg", "bmp", "gif", "png", "tif", "tiff"}, [&, completionCallback](string path, bool success) {
		if(!success) completionCallback(false, "");

		fs::path p(path);
		string outPath = tempDir() + "/" + p.filename().string();
		Log::d() << "Copying image to " << outPath;

		try {
			fs::copy_file(p, outPath, fs::copy_option::overwrite_if_exists);
			completionCallback(true, outPath);
		} catch(const fs::filesystem_error &err) {
			Log::d() << "Got filesystem error " << err.what();
			completionCallback(false, "");
		}
	});
#endif
	
}

#if TARGET_OS_IOS
#include "UIBlockButton.h"
#endif
void Dialogs::launchUrlInWebView(string url, function<void()> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif

	
	
	
#if TARGET_OS_IOS
	WKWebView *wv = [[WKWebView alloc] initWithFrame: CGRectMake(0, 0, 200, 200)];
	NSURL *URL = [[NSURL alloc] initWithString:[NSString stringWithUTF8String:url.c_str()]];
	NSURLRequest *req = [[NSURLRequest alloc] initWithURL:URL];
	[wv loadRequest:req];
	[wv setTranslatesAutoresizingMaskIntoConstraints:NO];
	
	
	
	UIViewController *targetController = [[UIViewController alloc] init];

	UIBlockButton *butt = [UIBlockButton buttonWithType:UIButtonTypeSystem];
	[butt setTitle: @"Close" forState:UIControlStateNormal];
	butt.frame = CGRectMake(0, 0, 200, 40);
	[butt handleControlEvent:UIControlEventTouchUpInside withBlock:^ {
		[targetController dismissViewControllerAnimated:true completion:nil];
		if(completionCallback) completionCallback();
	}];
	[butt setTranslatesAutoresizingMaskIntoConstraints:NO];
	butt.tintColor = [UIColor redColor];
	
	
	[targetController.view addSubview:wv];
	[targetController.view addSubview:butt];
	targetController.modalPresentationStyle = UIModalPresentationFormSheet;
	targetController.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;

	targetController.view.backgroundColor = [UIColor whiteColor];
	NSDictionary *views = NSDictionaryOfVariableBindings(wv, butt);
	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-0-[wv]-0-|" options:0 metrics:nil views:views]];
	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-10-[butt]-0-[wv]-0-|" options:0 metrics:nil views:views]];

	[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[butt]-20-|" options:0 metrics:nil views:views]];

	[((__bridge UIViewController*)app.viewController) presentViewController:targetController animated:YES completion:nil];

#else
	launchUrl(url);
#endif
}



void Dialogs::share(std::string message, std::string path, function<void(bool)> completionCallback) const {
#ifdef AUTO_TEST
	return;
#endif
	
#if TARGET_OS_IOS
	NSString *str = [NSString stringWithUTF8String:message.c_str()];
	NSURL *URL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
	
	UIActivityViewController *activityViewController =
	[[UIActivityViewController alloc] initWithActivityItems:@[URL]
									  applicationActivities:nil];
	
	UIViewController *vc = ((__bridge UIViewController*)app.viewController);

	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
		[vc presentViewController:activityViewController
												animated:YES
											  completion:^{
												  completionCallback(true);
											  }];
	}
	//if iPad
	else {
		// Change Rect to position Popover
		UIPopoverController *popup = [[UIPopoverController alloc] initWithContentViewController:activityViewController];
		
		[popup
		 
		 presentPopoverFromRect:
		 CGRectMake(vc.view.frame.size.width/2,
					vc.view.frame.size.height/4, 0, 0)
		 inView:vc.view permittedArrowDirections:UIPopoverArrowDirectionAny
		 animated:YES];
	}
	

#elif defined(__ANDROID__)
		androidShareDialog(message, path, completionCallback);
#else
	
	string destPath = "/Users/marek/Desktop/" + fs::path(path).filename().string();
	printf("No sharing pane on mac for now - saved to desktop\n");
	std::ifstream  src(path, std::ios::binary);
	printf("Copying %s to %s\n", path.c_str(), destPath.c_str());
	std::ofstream  dst(destPath.c_str(),   std::ios::binary);
	
	dst << src.rdbuf();
#endif
	
}




#if defined(__APPLE__)

#if TARGET_OS_IOS


@interface FilePickerDelegate : NSObject <UIDocumentPickerDelegate> {
	
	@public std::function<void(std::string, bool)> completionCallback;
}
@end

@implementation FilePickerDelegate {
}

-(void) documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {}

-(void) documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
	for(int i = 0; i < [urls count]; i++) {
		NSURL *url = [urls objectAtIndex:i];
		std::string path = [[url path] UTF8String];
		completionCallback(path, true);
	}
}

@end




FilePickerDelegate *fpd = nil;






#else
@interface FilePickerDelegate : NSObject <NSOpenSavePanelDelegate> {

}
-(void) setAllowedExtensions:(NSArray *)extensions;
@end

@implementation FilePickerDelegate {
NSArray *allowedExts;
BOOL allowAll;
}

-(id) init {
self = [super init];
if(self != nil) {
	allowAll = YES;
	allowedExts = @[@"png", @"tiff", @"jpg", @"gif", @"jpeg"];
}
return self;
}
-(void) setAllowedExtensions: (NSArray *)exts {
allowedExts = exts;
allowAll = NO;
}


- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
if(allowAll) return YES;

NSString* ext = [url pathExtension];
if ([ext isEqualToString: @""] || [ext isEqualToString: @"/"] || ext == nil || ext == nil || [ext length] < 1) {
	return YES;
}

for(NSString *e in allowedExts) {
	if ([ext caseInsensitiveCompare:e] == NSOrderedSame) {
		return YES;
	}
}
return NO;
}


@end
FilePickerDelegate *impikD = nil;
#endif
#endif
void Dialogs::loadFile(std::string msg, std::function<void(std::string, bool)> completionCallback) const {
	loadFile(msg, {}, completionCallback);
}

void Dialogs::loadFile(std::string msg, const std::vector<std::string> &allowedExtensions, std::function<void(std::string, bool)> completionCallback) const {



#ifdef AUTO_TEST
	return;
#endif
	
	
	
	
#ifdef _WIN32

	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	HWND hwnd = WindowFromDC(wglGetCurrentDC());
	ofn.hwndOwner = hwnd;

  //  wchar_t szFileName[MAX_PATH] = L"";
	char szFileName[MAX_PATH] = "";
	ofn.lpstrFilter = "All\0";
	ofn.lpstrFile = szFileName;

	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = 0;

	if(GetOpenFileName(&ofn)) {
		completionCallback(szFileName, true);
//completionCallback(convertWideToNarrow(szFileName), true);
	} else {
		completionCallback("", false);
	}

#elif defined(__APPLE__)

#	if TARGET_OS_IOS
	
	
	
	
	
	
	if(fpd==nil) {
		fpd = [[FilePickerDelegate alloc] init];
	}
	fpd->completionCallback = completionCallback;

	
	
	
	

	NSMutableArray<NSString*> *docTypes = [[NSMutableArray alloc] init];
	
	// catchall
	if(allowedExtensions.size()==0) {
		[docTypes addObject:(NSString*)kUTTypeData];
	}
	
	for(const auto &ext: allowedExtensions) {
		if(ext=="json") {
			[docTypes addObject:(NSString*)kUTTypeJSON];
		} else if(ext=="bmp") {
			[docTypes addObject:(NSString*)kUTTypeBMP];
		} else if(ext=="gif") {
			[docTypes addObject:(NSString*)kUTTypeGIF];
		} else if(ext=="pdf") {
			[docTypes addObject:(NSString*)kUTTypePDF];
		} else if(ext=="png") {
			[docTypes addObject:(NSString*)kUTTypePNG];
		} else if(ext=="rtf") {
			[docTypes addObject:(NSString*)kUTTypeRTF];
		} else if(ext=="xml") {
			[docTypes addObject:(NSString*)kUTTypeXML];
		} else if(ext=="mp3") {
			[docTypes addObject:(NSString*)kUTTypeMP3];
		} else if(ext=="ttf") {
			[docTypes addObject:(NSString*)kUTTypeFont];
		} else if(ext=="html" || ext=="htm") {
			[docTypes addObject:(NSString*)kUTTypeHTML];
		} else if(ext=="jpg" || ext=="jpeg") {
			[docTypes addObject:(NSString*)kUTTypeJPEG];
		} else if(ext=="mov") {
			[docTypes addObject:(NSString*)kUTTypeMPEG];
		} else if(ext=="tiff") {
			[docTypes addObject:(NSString*)kUTTypeTIFF];
		} else if(ext=="txt") {
			[docTypes addObject:(NSString*)kUTTypeText];
		} else if(ext=="mp4" || ext=="mov") {
			[docTypes addObject:(NSString*)kUTTypeMPEG4];
		} else if(ext=="avi") {
			[docTypes addObject:(NSString*)kUTTypeAVIMovie];
		} else if(ext=="wav") {
			[docTypes addObject:(NSString*)kUTTypeWaveformAudio];
		} else if(ext=="aif" || ext=="aiff") {
			[docTypes addObject:(NSString*)kUTTypeAudioInterchangeFileFormat];
		} else if(ext=="m4a") {
			[docTypes addObject:(NSString*)kUTTypeMPEG4Audio];
		} else {
			Log::e() << "Can't find uttype for extension" << ext;
			mzAssert(false);
		}
	}
	//= @[(NSString*)kUTTypeAudio, (NSString*)kUTTypeVideo, (NSString*)kUTTypeQuickTimeMovie, (NSString*)kUTTypeMPEG4];
	UIDocumentPickerViewController *filePicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:docTypes inMode:UIDocumentPickerModeImport];

	filePicker.delegate = fpd;

//	if (@available(iOS 11.0, *)) {
//		filePicker.allowsMultipleSelection = YES;
//	}

	UIViewController *vc = (__bridge UIViewController*) app.viewController;

	[vc presentViewController:filePicker animated: YES completion:^{
//		if (@available(iOS 11.0, *)) {
//			filePicker.allowsMultipleSelection = YES;
//		}
	}];


	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
#	else
	auto allowedExts = allowedExtensions;
	dispatch_async(dispatch_get_main_queue(), ^{
		// do work here
		NSInteger buttonClicked;
		string filePath = "";
		@autoreleasepool {
			NSOpenPanel * loadDialog = [NSOpenPanel openPanel];
			
			
			if(allowedExts.size()>0) {
				if(impikD==nil) {
					impikD = [[FilePickerDelegate alloc] init];
				}
				vector<NSString*> nsExts;
				for(const auto &ext: allowedExts) {
					nsExts.push_back([NSString stringWithUTF8String:ext.c_str()]);
				}
				NSArray *exts = [NSArray arrayWithObjects:&nsExts[0] count: nsExts.size()];
				[impikD setAllowedExtensions:exts];
				loadDialog.delegate = impikD;
			}
			
			NSOpenGLContext *context = [NSOpenGLContext currentContext];
			[loadDialog setMessage:[NSString stringWithUTF8String:msg.c_str()]];
//			[Dialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileName.c_str()]];
			
			buttonClicked = [loadDialog runModal];
			
			[context makeCurrentContext];
			
			if(buttonClicked == NSModalResponseOK){
				filePath = string([[[loadDialog URL] path] UTF8String]);
			}
		}
		completionCallback(filePath, buttonClicked == NSModalResponseOK);
		
	});
#	endif
#elif !defined(__ANDROID__) && defined(__linux__)
	linuxLoadFileDialog(msg, allowedExtensions, completionCallback);
#endif

}



