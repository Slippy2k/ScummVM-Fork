/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "xeen/dialogs_input.h"
#include "xeen/scripts.h"
#include "xeen/xeen.h"

namespace Xeen {

int Input::show(XeenEngine *vm, Window *window, Common::String &line,
		uint maxLen, int maxWidth, bool isNumeric) {
	Input *dlg = new Input(vm, window);
	int result = dlg->getString(line, maxLen, maxWidth, isNumeric);
	delete dlg;

	return result;
}

int Input::getString(Common::String &line, uint maxLen, int maxWidth, bool isNumeric) {
	_vm->_noDirectionSense = true;
	Common::String msg = Common::String::format("\x3""l\t000\x4%03d\x3""c", maxWidth);
	_window->writeString(msg);
	_window->update();

	while (!_vm->shouldQuit()) {
		Common::KeyCode keyCode = waitForKey(msg);

		bool refresh = false;
		if ((keyCode == Common::KEYCODE_BACKSPACE || keyCode == Common::KEYCODE_DELETE)
				&& line.size() > 0) {
			line.deleteLastChar();
			refresh = true;
		} else if (line.size() < maxLen && (line.size() > 0 || keyCode != Common::KEYCODE_SPACE)
				&& ((isNumeric && keyCode >= Common::KEYCODE_0 && keyCode < Common::KEYCODE_9) ||
				   (!isNumeric && keyCode >= Common::KEYCODE_SPACE && keyCode < Common::KEYCODE_DELETE))) {
			line += (char)keyCode;
			refresh = true;
		} else if (keyCode == Common::KEYCODE_RETURN || keyCode == Common::KEYCODE_KP_ENTER) {
			break;
		} else if (keyCode == Common::KEYCODE_ESCAPE) {
			line = "";
			break;
		}

		if (refresh) {
			msg = Common::String::format("\x3""l\t000\x4%03d\x3""c%s", maxWidth, line.c_str());
			_window->writeString(msg);
			_window->update();
		}
	}

	_vm->_noDirectionSense = false;
	return line.size();
}

Common::KeyCode Input::waitForKey(const Common::String &msg) {
	EventsManager &events = *_vm->_events;
	Interface &intf = *_vm->_interface;
	Windows &windows = *_vm->_windows;

	bool oldUpDoorText = intf._upDoorText;
	byte oldTillMove = intf._tillMove;
	intf._upDoorText = false;
	intf._tillMove = 0;

	bool flag = !_vm->_startupWindowActive && !windows[25]._enabled
		&& _vm->_mode != MODE_FF && _vm->_mode != MODE_17;

	Common::KeyCode ch = Common::KEYCODE_INVALID;
	while (!_vm->shouldQuit()) {
		events.updateGameCounter();

		if (flag)
			intf.draw3d(false);
		_window->writeString(msg);
		animateCursor();
		_window->update();

		if (flag)
			windows[3].update();

		events.wait(1);

		if (events.isKeyPending()) {
			Common::KeyState keyState;
			events.getKey(keyState);
			ch = keyState.keycode;
			break;
		}
	}

	_window->writeString("");
	_window->update();

	intf._tillMove = oldTillMove;
	intf._upDoorText = oldUpDoorText;

	return ch;
}

void Input::animateCursor() {
	// Iterate through each frame
	_cursorAnimIndex = _cursorAnimIndex ? _cursorAnimIndex - 1 : 5;
	static const int CURSOR_ANIMATION_IDS[] = { 32, 124, 126, 127, 126, 124 };

	// Form a string for the cursor and write it out
	Common::String cursorStr = Common::String::format("%c",
		CURSOR_ANIMATION_IDS[_cursorAnimIndex]);

	Common::Point writePos = _window->_writePos;
	_window->writeString(cursorStr);
	_window->_writePos = writePos;
}

/*------------------------------------------------------------------------*/

StringInput::StringInput(XeenEngine *vm): Input(vm, &(*vm->_windows)[6]) {
}

int StringInput::show(XeenEngine *vm, bool type, const Common::String &msg1,
		const Common::String &msg2, int opcode) {
	StringInput *dlg = new StringInput(vm);
	int result = dlg->execute(type, msg1, msg2, opcode);
	delete dlg;

	return result;
}

int StringInput::execute(bool type, const Common::String &expected,
		const Common::String &title, int opcode) {
	Interface &intf = *_vm->_interface;
	Scripts &scripts = *_vm->_scripts;
	Windows &windows = *_vm->_windows;
	Window &w = windows[6];
	Sound &sound = *_vm->_sound;
	int result = 0;

	w.open();
	w.writeString(Common::String::format("\r\x03""c%s\v024\t000", title.c_str()));
	w.update();

	Common::String line;
	if (getString(line, 30, 200, false)) {
		if (type) {
			if (line == intf._interfaceText) {
				result = true;
			} else if (line == expected) {
				result = (opcode == 55) ? -1 : 1;
			}
		} else {
			// Load in the mirror list
			File f(Common::String::format("%smirr.txt",
				_vm->_files->_isDarkCc ? "dark" : "xeen"));
			MirrorEntry me;
			scripts._mirror.clear();
			while (me.synchronize(f))
				scripts._mirror.push_back(me);

			for (uint idx = 0; idx < scripts._mirror.size(); ++idx) {
				if (line == scripts._mirror[idx]._name) {
					result = idx + 1;
					sound.playFX(_vm->_files->_isDarkCc ? 35 : 61);
					break;
				}
			}
		}
	}

	w.close();
	return result;
}

/*------------------------------------------------------------------------*/

NumericInput::NumericInput(XeenEngine *vm, int window) : Input(vm, &(*vm->_windows)[window]) {
}

int NumericInput::show(XeenEngine *vm, int window, int maxLength, int maxWidth) {
	NumericInput *dlg = new NumericInput(vm, window);
	int result = dlg->execute(maxLength, maxWidth);
	delete dlg;

	return result;
}

int NumericInput::execute(int maxLength, int maxWidth) {
	Common::String line;

	if (getString(line, maxLength, maxWidth, true))
		return atoi(line.c_str());
	else
		return 0;
}

/*------------------------------------------------------------------------*/

int Choose123::show(XeenEngine *vm, int numOptions) {
	assert(numOptions <= 3);
	Choose123 *dlg = new Choose123(vm);
	int result = dlg->execute(numOptions);
	delete dlg;

	return result;
}

int Choose123::execute(int numOptions) {
	EventsManager &events = *_vm->_events;
	Interface &intf = *_vm->_interface;
	Town &town = *_vm->_town;
	Windows &windows = *_vm->_windows;

	Mode oldMode = _vm->_mode;
	_vm->_mode = MODE_DIALOG_123;

	loadButtons(numOptions);
	_iconSprites.draw(0, 7, Common::Point(232, 74));
	drawButtons(&windows[0]);
	windows[34].update();

	int result = -1;
	while (result == -1) {
		do {
			events.updateGameCounter();
			int delay;
			if (town.isActive()) {
				town.drawAnim(true);
				delay = 3;
			} else {
				intf.draw3d(true);
				delay = 1;
			}

			events.wait(delay);
			if (_vm->shouldQuit())
				return 0;
		} while (!_buttonValue);

		switch (_buttonValue) {
		case Common::KEYCODE_ESCAPE:
			result = 0;
			break;
		case Common::KEYCODE_1:
		case Common::KEYCODE_2:
		case Common::KEYCODE_3: {
			int v = _buttonValue - Common::KEYCODE_1 + 1;
			if (v <= numOptions)
				result = v;
			break;
		}
		default:
			break;
		}
	}

	_vm->_mode = oldMode;
	intf.mainIconsPrint();

	return result;
}

void Choose123::loadButtons(int numOptions) {
	_iconSprites.load("choose.icn");

	if (numOptions >= 1)
		addButton(Common::Rect(235, 75, 259, 95), Common::KEYCODE_1, &_iconSprites);
	if (numOptions >= 2)
		addButton(Common::Rect(260, 75, 284, 95), Common::KEYCODE_2, &_iconSprites);
	if (numOptions >= 3)
		addButton(Common::Rect(286, 75, 311, 95), Common::KEYCODE_3, &_iconSprites);
}

/*------------------------------------------------------------------------*/

int HowMuch::show(XeenEngine *vm) {
	HowMuch *dlg = new HowMuch(vm);
	int result = dlg->execute();
	delete dlg;

	return result;
}

int HowMuch::execute() {
	Windows &windows = *_vm->_windows;
	Window &w = windows[6];
	Common::String num;

	w.open();
	w.writeString(Res.HOW_MUCH);
	w.update();
	int lineSize = Input::show(_vm, &w, num, 8, 70, true);
	w.close();

	if (!lineSize)
		return -1;
	return atoi(num.c_str());
}

} // End of namespace Xeen
