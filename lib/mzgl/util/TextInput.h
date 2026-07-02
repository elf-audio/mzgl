//
//  TextInput.h
//  mzgl
//
//  Cross-platform text input. A TextInputReceiver is a focusable target (e.g. a
//  search field) that receives typed text from the platform's keyboard — the
//  software keyboard on iOS/Android, the hardware keyboard on mac/desktop.
//
//  Focus/dismiss via Graphics::showKeyboard(receiver) / Graphics::hideKeyboard().
//  Platform key/IME code delivers edits through EventDispatcher::textInput() etc.
//

#pragma once
#include <string>

class TextInputReceiver {
public:
	virtual ~TextInputReceiver() {}

	// Current text — used to seed a native text buffer (e.g. the hidden iOS
	// UITextField) when the keyboard is shown.
	virtual std::string getText() const = 0;

	// The native text buffer is authoritative (iOS/Android IME, autocorrect,
	// dictation): replace the whole string. Called on every editing change.
	virtual void setText(const std::string &utf8) = 0;

	// Character-at-a-time editing, for platforms whose hardware keyboards
	// deliver individual key events rather than owning a text buffer (mac/glfw).
	virtual void insertText(const std::string &utf8) = 0;
	virtual void deleteBackward()					 = 0;

	// Return / Enter / done pressed on the keyboard.
	virtual void onTextDone() {}

	// Hint shown when empty (forwarded to native fields where applicable).
	virtual std::string getPlaceholder() const { return ""; }
};
