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
	const App &app;
	Dialogs(const App &app) : app(app) {
		
	}
	Dialogs & operator=(const Dialogs&) = delete;
	Dialogs(const Dialogs&) = delete;

	void textbox(std::string title, std::string msg, std::string text, std::function<void(std::string, bool)> completionCallback) const;
	
	
	
	void confirm(std::string title, std::string msg,
	std::function<void()> okPressed,
	std::function<void()> cancelPressed = [](){}) const;
	
	void twoOptionDialog(std::string title, std::string msg,
							   std::string buttonOneText, std::function<void()> buttonOnePressed,
							   std::string buttonTwoText, std::function<void()> buttonTwoPressed) const;
	
	void twoOptionCancelDialog(std::string title, std::string msg,
							   std::string buttonOneText, std::function<void()> buttonOnePressed,
							   std::string buttonTwoText, std::function<void()> buttonTwoPressed,
							   std::function<void()> cancelPressed) const;
	
	void threeOptionDialog(std::string title, std::string msg,
			std::string buttonOneText, std::function<void()> buttonOnePressed,
			std::string buttonTwoText, std::function<void()> buttonTwoPressed,
			  std::string buttonThreeText, std::function<void()> buttonThreePressed) const;

	void threeOptionCancelDialog(std::string title, std::string msg,
			std::string buttonOneText, std::function<void()> buttonOnePressed,
			std::string buttonTwoText, std::function<void()> buttonTwoPressed,
			  std::string buttonThreeText, std::function<void()> buttonThreePressed,
								 std::function<void()> cancelPressed) const;

	
	void alert(std::string title, std::string msg) const;
	
	void chooseImage(std::function<void(bool success, std::string imgPath)> completionCallback) const;
	
	void share(std::string message, std::string path, std::function<void(bool)> completionCallback) const;
	
	void loadFile(std::string msg, std::function<void(std::string, bool)> completionCallback) const;
	void loadFile(std::string msg, const std::vector<std::string> &allowedExtensions, std::function<void(std::string, bool)> completionCallback) const;
	
	void launchUrlInWebView(std::string url, std::function<void()> completionCallback) const;
	void displayHtmlInWebView(const std::string &html, std::function<void()> completionCallback) const;
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
