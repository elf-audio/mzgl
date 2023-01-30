//
//  Dialogs.h
//  mzgl
//
//  Created by Marek Bereza on 12/07/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
class App;
#include <string>
#include <functional>
#include <vector>

class Dialogs {
public:
	App &app;

	/**
	 * Constructor must take the calling app because
	 * some dialog methods need references to the apps window
	 * which can be stored in the App class.
	 */
	Dialogs(App &app) : app(app) {

	}

	Dialogs & operator=(const Dialogs&) = delete;
	Dialogs(const Dialogs&) = delete;








	/**
	 * launch an alert dialog - e.g. just a message with an ok button.
	 *
	 * @param title - can be ignored if the native window system doesn't
	 *			      easily support titles on dialog windows.
	 *
	 * @param msg - the message to be displayed
	 */
	void alert(std::string title, std::string msg) const;


	/**
	 * launch a dialog with a message, an ok and a cancel button.
	 *
	 * @param title - can be ignored if the native window system doesn't
	 *			      easily support titles on dialog windows.
	 *
	 * @param msg - the message to be displayed.
	 *
	 * @param okPressed - the callback if the user pressed "ok"
	 *
	 * @param cancelPressed - the callback if the user pressed "cancel"
	 */
	void confirm(std::string title, std::string msg,
								std::function<void()> okPressed,
								std::function<void()> cancelPressed = [](){}) const;

	/**
	 * launch a dialog with a message, a textbox and an ok and cancel button.
	 *
	 * @param title - can be ignored if the native window system doesn't
	 *			      easily support titles on dialog windows.
	 *
	 * @param msg - the message to be displayed.
	 *
	 * @param text - the placeholder text that should be prepopulated into the textbox
	 *
	 * @param completionCallback - the callback that should come back on the main thread to signal that
	 *                             the user has pressed "ok" or "cancel" (the boolean argument)
	 *                             and the string typed (the string argument)
	 */
	void textbox(std::string title, std::string msg, std::string text,
											std::function<void(std::string, bool)> completionCallback) const;





	/**
	 * launch a dialog with a message and 2 buttons.
	 */
	void twoOptionDialog(std::string title, std::string msg,
							   std::string buttonOneText, std::function<void()> buttonOnePressed,
							   std::string buttonTwoText, std::function<void()> buttonTwoPressed) const;


	/**
	 * launch a dialog with a message and 2 buttons and a cancel button
	 */
	void twoOptionCancelDialog(std::string title, std::string msg,
							   std::string buttonOneText, std::function<void()> buttonOnePressed,
							   std::string buttonTwoText, std::function<void()> buttonTwoPressed,
							   std::function<void()> cancelPressed) const;


	/**
	 * launch a dialog with a message and 3 buttons
	 */
	void threeOptionDialog(std::string title, std::string msg,
			std::string buttonOneText, std::function<void()> buttonOnePressed,
			std::string buttonTwoText, std::function<void()> buttonTwoPressed,
			  std::string buttonThreeText, std::function<void()> buttonThreePressed) const;

	/**
	 * launch a dialog with a message and 3 buttons and a cancel button
	 */
	void threeOptionCancelDialog(std::string title, std::string msg,
			std::string buttonOneText, std::function<void()> buttonOnePressed,
			std::string buttonTwoText, std::function<void()> buttonTwoPressed,
			  std::string buttonThreeText, std::function<void()> buttonThreePressed,
								 std::function<void()> cancelPressed) const;


	/**
	 * launch a dialog with a message and 3 buttons and a cancel button
	 */
	void chooseImage(std::function<void(bool success, std::string imgPath)> completionCallback) const;

	/**
	 * If the OS has a standard sharing dialog, this method launches one of those.
	 */
	void share(std::string message, std::string path, std::function<void(bool)> completionCallback) const;

	/**
	 * Launch file opening dialog
	 */
	void loadFile(std::string msg, std::function<void(std::string, bool)> completionCallback) const;

	/**
	 * Launch file opening dialog with allowed file extensions filter.
	 */
	void loadFile(std::string msg, const std::vector<std::string> &allowedExtensions, std::function<void(std::string, bool)> completionCallback) const;

	/**
	 * launches a web view pointing to a web url - only really needed on iOS (for ableton lite download)
	 */
	void launchUrlInWebView(std::string url, std::function<void()> completionCallback) const;

	/**
	 * launch a webview that displays the html contained in the string. Optionally calls completionCallback if webview is dismissed
	 */
	void displayHtmlInWebView(const std::string &html, std::function<void()> completionCallback) const;

	void chooseFolder(std::string msg, std::function<void(std::string, bool)> completionCallback) const;

};


#ifdef UNIT_TEST

namespace unit_test {
    bool isDialogOpen();
    void dismissAlert();
    void pressOk();
    void pressCancel();
    void pressButtonOne();
    void pressButtonTwo();
    void pressButtonThree();
}

#endif
