#include <string>
#include <functional>
void windowsTextboxDialog(std::string title, std::string msg, std::string text, std::function<void(std::string, bool)> completionCallback);
void windowsConfirmDialog(std::string title, std::string msg,
                   std::function<void()> okPressed,
                   std::function<void()> cancelPressed);

void windowsTwoOptionCancelDialog(std::string title, std::string msg,
                           std::string buttonOneText, std::function<void()> buttonOnePressed,
                           std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                           std::function<void()> cancelPressed);

void windowsThreeOptionCancelDialog(std::string title, std::string msg,
                             std::string buttonOneText, std::function<void()> buttonOnePressed,
                             std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                             std::string buttonThreeText, std::function<void()> buttonThreePressed,
                             std::function<void()> cancelPressed);

// isFile - true choose file
//          false choose dir
void windowsChooseEntryDialog(bool isFile, std::string msg, std::function<void(std::string, bool)> completionCallback);
