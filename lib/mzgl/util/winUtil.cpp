#include "winUtil.h"


// This sample will work either with or without UNICODE, it looks like
// it's recommended now to use UNICODE for all new code, but I left
// the ANSI option in there just to get the absolute maximum amount
// of compatibility.
//
// Note that UNICODE and _UNICODE go together, unfortunately part
// of the Windows API uses _UNICODE, and part of it uses UNICODE.
//
// tchar.h, for example, makes heavy use of _UNICODE, and windows.h
// makes heavy use of UNICODE.



#include <locale>
#include <codecvt>

std::string w2n(const std::wstring& w)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string narrow = converter.to_bytes(w);
    return narrow;
}

std::wstring n2w(const std::string& n)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(n);
    return wide;
}



#define UNICODE
#define _UNICODE
//#undef UNICODE
//#undef _UNICODE




#include <windows.h>
#include <tchar.h>

#include "log.h"
#include "Rectf.h"

const WORD ID_btnOk = 1234;
const WORD ID_btnCancel = 1235;

// Typically these would be #defines, but there
// is no reason to not make them constants
const WORD ID_btnHELLO = 1;
const WORD ID_btnQUIT = 2;
const WORD ID_CheckBox = 3;
const WORD ID_txtEdit = 4;
const WORD ID_btnShow = 5;
const WORD ID_btn1 = 6;
const WORD ID_btn2 = 7;
const WORD ID_btn3 = 8;
//                                    x,      y,      width,  height
const Rectf dialogWindow = {150, 150, 500, 220};

// const Rectf btnHello     =   { 20,     50,     80,     25 };
// const Rectf btnQuit      =   { 120,    50,     80,     25 };
// const Rectf chkCheck     =   { 20,     90,     185,    35 };

// const Rectf txtEdit      =   { 20,     150,    150,    20 };
// const Rectf btnShow      =   { 180,    150,    80,     25 };

HWND txtEditHandle = NULL;

std::function<void(std::string, bool)> textboxCallback;
std::string textboxDefaultText = "";
// hwnd:    All window processes are passed the handle of the window
//         that they belong to in hwnd.
// msg:     Current message (e.g., WM_*) from the OS.
// wParam:  First message parameter, note that these are more or less
//          integers, but they are really just "data chunks" that
//          you are expected to memcpy as raw data to float, etc.
// lParam:  Second message parameter, same deal as above.
LRESULT CALLBACK textboxWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam) {
    RECT R;
    //  GetWindowRect(hwnd, &R);
    //  Log::d() << "Reported size: " << R.left << R.top << (R.right - R.left) << (R.bottom - R.top);
    int padding = 10;
    Rectf txtEdit;

    txtEdit.width = dialogWindow.width - padding*2 - 25;
    txtEdit.height = (dialogWindow.height - padding * 3) / 2 - 25;
    txtEdit.x = padding;
    txtEdit.y = padding;

    Rectf btnCancel(padding, txtEdit.bottom() + padding, (txtEdit.width - padding)/2, txtEdit.height);
    Rectf btnOk(btnCancel.right() + padding, btnCancel.y, btnCancel.width, btnCancel.height);

    switch (msg) {

        case WM_CREATE:

            txtEditHandle = CreateWindow(TEXT("Edit"),n2w(textboxDefaultText).c_str(),
                                         WS_CHILD | WS_VISIBLE | WS_BORDER, txtEdit.x,
                                         txtEdit.y, txtEdit.width, txtEdit.height, hwnd,
                                         (HMENU)ID_txtEdit, NULL, NULL);

            CreateWindow(TEXT("Button"), TEXT("Cancel"), WS_VISIBLE | WS_CHILD,
                         btnCancel.x, btnCancel.y, btnCancel.width, btnCancel.height,
                         hwnd, (HMENU)ID_btnCancel, NULL, NULL);

            CreateWindow(TEXT("Button"), TEXT("OK"), WS_VISIBLE | WS_CHILD, btnOk.x,
                         btnOk.y, btnOk.width, btnOk.height, hwnd, (HMENU)ID_btnOk,
                         NULL, NULL);

            break;

            // For more information about WM_COMMAND, see
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591(v=vs.85).aspx
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_btnOk) {
                Log::d() << "OK pressed";
                int textLength_WithNUL = GetWindowTextLength(txtEditHandle) + 1;
                // WARNING: If you are compiling this for C, please remember to remove the
                // (TCHAR*) cast.
                TCHAR *textBoxText = (TCHAR *)malloc(sizeof(TCHAR) * textLength_WithNUL);

                GetWindowText(txtEditHandle, textBoxText, textLength_WithNUL);

                // MessageBox(hwnd, textBoxText, TEXT("Here's what you typed"), MB_OK);
                char output[512];
                sprintf(output, "%ws", textBoxText);
                textboxCallback(std::string(output), true);
                Log::d() << "Completed textbox with " << output;
                free(textBoxText);
                DestroyWindow(hwnd);

            } else if (LOWORD(wParam) == ID_btnCancel) {
                Log::d() << "Cancel pressed";
                textboxCallback("", false);
                DestroyWindow(hwnd);
            }
            break;


    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}



std::function<void()> buttonOneCallback;
std::function<void()> buttonTwoCallback;
std::function<void()> buttonThreeCallback;
std::string buttonOneName, buttonTwoName, buttonThreeName;
std::function<void()> cancelCallback;
std::string dialogTitle;
std::string dialogMsg;
int numOptions = 2;


LRESULT CALLBACK optionCancelDialogProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam) {
    int padding = 10;
    Rectf msgArea;

    msgArea.width = dialogWindow.width - padding*2 - 25;
    msgArea.height = (dialogWindow.height - padding * 3) / 2 - 25;
    msgArea.x = padding;
    msgArea.y = padding;

    Rectf btnCancel(padding, msgArea.bottom() + padding, (msgArea.width - padding)/2, msgArea.height);
    Rectf btnOk(btnCancel.right() + padding, btnCancel.y, btnCancel.width, btnCancel.height);

    switch (msg) {

        case WM_CREATE: {

            CreateWindow(TEXT("Static"), n2w(dialogMsg).c_str(),
                         WS_CHILD | WS_VISIBLE | WS_BORDER, msgArea.x,
                         msgArea.y, msgArea.width, msgArea.height, hwnd,
                         (HMENU) ID_txtEdit, NULL, NULL);
            float buttonX = msgArea.x;
            float buttonHeight = msgArea.height;
            const float buttonY = msgArea.bottom() + padding;
            if (numOptions == 2) {

                float buttonWidth = msgArea.width / 3 - padding * 2;
                CreateWindow(TEXT("Button"), n2w(buttonOneName).c_str(), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btn1, NULL, NULL);

                buttonX += buttonWidth + padding;
                CreateWindow(TEXT("Button"), n2w(buttonTwoName).c_str(), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btn2, NULL, NULL);
                buttonX += buttonWidth + padding;
                CreateWindow(TEXT("Button"), TEXT("Cancel"), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btnCancel, NULL, NULL);


            } else if (numOptions == 3) {
                float buttonWidth = msgArea.width / 4 - padding * 3;
                CreateWindow(TEXT("Button"), n2w(buttonOneName).c_str(), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btn1, NULL, NULL);


                buttonX += buttonWidth + padding;
                CreateWindow(TEXT("Button"), n2w(buttonTwoName).c_str(), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btn2, NULL, NULL);

                buttonX += buttonWidth + padding;

                CreateWindow(TEXT("Button"), n2w(buttonThreeName).c_str(), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btn3, NULL, NULL);

                buttonX += buttonWidth + padding;

                CreateWindow(TEXT("Button"), TEXT("Cancel"), WS_VISIBLE | WS_CHILD,
                             buttonX, buttonY, buttonWidth, buttonHeight,
                             hwnd, (HMENU) ID_btnCancel, NULL, NULL);
            }
//            CreateWindow(TEXT("Button"), TEXT("Cancel"), WS_VISIBLE | WS_CHILD,
//                         btnCancel.x, btnCancel.y, btnCancel.width, btnCancel.height,
//                         hwnd, (HMENU)ID_btnCancel, NULL, NULL);
//
//            CreateWindow(TEXT("Button"), TEXT("OK"), WS_VISIBLE | WS_CHILD, btnOk.x,
//                         btnOk.y, btnOk.width, btnOk.height, hwnd, (HMENU)ID_btnOk,
//                         NULL, NULL);
        }
            break;

            // For more information about WM_COMMAND, see
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591(v=vs.85).aspx



        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_btn1) {

                buttonOneCallback();
                DestroyWindow(hwnd);

            } else if (LOWORD(wParam) == ID_btn2) {

                buttonTwoCallback();
                DestroyWindow(hwnd);

            } else if (LOWORD(wParam) == ID_btn3) {

                buttonThreeCallback();
                DestroyWindow(hwnd);

            } else if (LOWORD(wParam) == ID_btnCancel) {
                cancelCallback();
                DestroyWindow(hwnd);
            }
        }
            break;


    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// https://stackoverflow.com/questions/61634/windows-api-dialogs-without-using-resource-files


void windowsTextboxDialog(std::string title, std::string msg, std::string text,
                          std::function<void(std::string, bool)> completionCallback)
{
  textboxCallback = completionCallback;
  textboxDefaultText = text;
  auto hInstance = GetModuleHandle(NULL);

  // MSG  msg;
  WNDCLASS mainWindowClass = {0};

  // You can set the main window name to anything, but
  // typically you should prefix custom window classes
  // with something that makes it unique.
  mainWindowClass.lpszClassName = TEXT("mzgl.textboxdialog");

  mainWindowClass.hInstance = hInstance;
  mainWindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  mainWindowClass.lpfnWndProc = textboxWndProc;
  mainWindowClass.hCursor = LoadCursor(0, IDC_ARROW);
  RegisterClass(&mainWindowClass);
  CreateWindow(mainWindowClass.lpszClassName, n2w(title).c_str(), //TEXT("-"),
               WS_OVERLAPPEDWINDOW | WS_VISIBLE, dialogWindow.x, dialogWindow.y,
               dialogWindow.width, dialogWindow.height, NULL, 0, hInstance, NULL);
}



void windowsConfirmDialog(std::string title, std::string msg,
                          std::function<void()> okPressed,
                          std::function<void()> cancelPressed) {
    auto response = MessageBoxW(NULL, n2w(msg).c_str(), n2w(title).c_str(), MB_OKCANCEL);
    if(response==IDOK) {
        okPressed();
    } else {
        cancelPressed();
    }


}



void optionCancelDialog() {

    auto hInstance = GetModuleHandle(NULL);

    // MSG  msg;
    WNDCLASS mainWindowClass = {0};

    // You can set the main window name to anything, but
    // typically you should prefix custom window classes
    // with something that makes it unique.
    mainWindowClass.lpszClassName = TEXT("mzgl.optioncanceldialog");

    mainWindowClass.hInstance = hInstance;
    mainWindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    mainWindowClass.lpfnWndProc = optionCancelDialogProc;
    mainWindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClass(&mainWindowClass);
    CreateWindow(mainWindowClass.lpszClassName, n2w(dialogTitle).c_str(), //TEXT("-"),
                 WS_OVERLAPPEDWINDOW | WS_VISIBLE, dialogWindow.x, dialogWindow.y,
                 dialogWindow.width, dialogWindow.height, NULL, 0, hInstance, NULL);
}


void windowsTwoOptionCancelDialog(std::string title, std::string msg,
                                  std::string buttonOneText, std::function<void()> buttonOnePressed,
                                  std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                                  std::function<void()> cancelPressed) {
    buttonOneCallback = buttonOnePressed;
    buttonTwoCallback = buttonTwoPressed;
    cancelCallback = cancelPressed;
    dialogTitle = title;
    dialogMsg = msg;
    numOptions = 2;
    buttonOneName = buttonOneText;
    buttonTwoName = buttonTwoText;
    optionCancelDialog();

}

void windowsThreeOptionCancelDialog(std::string title, std::string msg,
                                    std::string buttonOneText, std::function<void()> buttonOnePressed,
                                    std::string buttonTwoText, std::function<void()> buttonTwoPressed,
                                    std::string buttonThreeText, std::function<void()> buttonThreePressed,
                                    std::function<void()> cancelPressed) {
    buttonOneCallback = buttonOnePressed;
    buttonTwoCallback = buttonTwoPressed;
    buttonThreeCallback = buttonThreePressed;
    cancelCallback = cancelPressed;
    dialogTitle = title;
    dialogMsg = msg;
    numOptions = 3;
    buttonOneName = buttonOneText;
    buttonTwoName = buttonTwoText;
    buttonThreeName = buttonThreeText;
    optionCancelDialog();
}