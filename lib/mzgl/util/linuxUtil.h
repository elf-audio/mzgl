/**
 * linuxUtil.h
 * 17/03/21
 * Marek Bereza
 */



#include <string>
#include <functional>
#include <vector>


void linuxSaveFileDialog(std::string msg, std::string defaultFileName, std::function<void(std::string, bool)> completionCallback);


void linuxLoadFileDialog(std::string msg, const std::vector<std::string> &allowedExtensions, std::function<void(std::string, bool)> completionCallback);

void linuxTextboxDialog(std::string title, std::string msg, std::string text,
                          std::function<void(std::string, bool)> completionCallback);


                          
void linuxAlertDialog(std::string title, std::string msg);


void linuxConfirmDialog(std::string title, std::string msg,
                          std::function<void()> okPressed,
                          std::function<void()> cancelPressed);



void linuxTwoOptionCancelDialog(std::string title, std::string msg,
                                  std::string buttonOneText, std::function<void()> buttonOnePressed,
                                  std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                                  std::function<void()> cancelPressed);

void linuxTwoOptionDialog(std::string title, std::string msg,
                                  std::string buttonOneText, std::function<void()> buttonOnePressed,
                                  std::string buttonTwoText, std::function<void()> buttonTwoPressed);


void linuxThreeOptionCancelDialog(std::string title, std::string msg,
                                    std::string buttonOneText, std::function<void()> buttonOnePressed,
                                    std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                                    std::string buttonThreeText, std::function<void()> buttonThreePressed,
                                    std::function<void()> cancelPressed);

