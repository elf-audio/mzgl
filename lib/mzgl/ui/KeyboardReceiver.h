
#pragma once

/**
 * If you want to respond to key events, implement this interface and
 * UIResources::requestKeyboardFocus() (and resignKeyboardFocus when done)
 * and return true if event is handled otherwise it will fall back to 
 * global key handling (e.g. space for play etc)
 */
class KeyboardReceiver {
public:
	virtual bool keyDown(int key) = 0;
	virtual bool keyUp(int key)	  = 0;
};