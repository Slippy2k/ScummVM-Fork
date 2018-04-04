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

#ifndef XEEN_TOWN_H
#define XEEN_TOWN_H

#include "common/scummsys.h"
#include "common/str-array.h"
#include "xeen/dialogs.h"
#include "xeen/dialogs_error.h"
#include "xeen/party.h"

namespace Xeen {

enum TownAction {
	BANK = 0, BLACKSMITH = 1, GUILD = 2, TAVERN = 3, TEMPLE = 4,
	TRAINING = 5, ARENA = 6, NO_ACTION = 7, REAPER = 8, GOLEM = 9,
	DWARF1 = 10, SPHINX = 11, PYRAMID = 12, DWARF2 = 13
};

class XeenEngine;
class TownMessage;

class TownLocation : public ButtonContainer {
protected:
	TownAction _townActionId;
	Common::Array<SpriteResource> _townSprites;
	SpriteResource _icons1, _icons2;
	int _townMaxId;
	const bool &_isDarkCc;
	int _animFrame;
	Common::String _vocName, _songName;
	Common::Point _townPos;
	int _drawFrameIndex;
	uint _farewellTime;
	int _drawCtr1, _drawCtr2;
	int _animCtr;
protected:
	/**
	 * Draw the window
	 */
	void drawWindow();

	/**
	 * Waits for a brief pause, checking for any key or mouse events
	 */
	int wait();

	/**
	 * Handles animation updates for Sphinx, Golem, Repear, and Dwarf events
	 */
	void animUpdate();

	/**
	 * Generates the display text for the location, for a given character
	 */
	virtual Common::String createLocationText(Character &ch) { return ""; }

	/**
	 * Draw the visual background
	 */
	virtual void drawBackground();

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c) { return c; }

	/**
	 * Handle any farewell
	 */
	virtual void farewell() {}
public:
	TownLocation(TownAction action);
	virtual ~TownLocation();

	/**
	 * Show the town location
	 */
	virtual int show();

	/**
	 * Draws the animated parts
	 */
	void drawAnim(bool flag);
};

class BankLocation : public TownLocation {
private:
	/**
	 * Handles deposits or withdrawls fro the bank
	 */
	void depositWithdrawl(PartyBank whereId);
protected:
	/**
	 * Generates the display text for the location, for a given character
	 */
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Draw the visual background
	 */
	virtual void drawBackground();

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	BankLocation();
	virtual ~BankLocation() {}
};

class BlacksmithLocation : public TownLocation {
protected:
	/**
	* Generates the display text for the location, for a given character
	*/
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Handle any farewell
	 */
	virtual void farewell();

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	BlacksmithLocation();
	virtual ~BlacksmithLocation() {}
};

class GuildLocation : public TownLocation {
protected:
	/**
	 * Generates the display text for the location, for a given character
	 */
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	GuildLocation();
	virtual ~GuildLocation() {}
};

class TavernLocation : public TownLocation {
private:
	int _v21;
	uint _v22;
	int _v23;
	int _v24;
protected:
	/**
	* Generates the display text for the location, for a given character
	*/
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Handle any farewell
	 */
	virtual void farewell();

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	TavernLocation();
	virtual ~TavernLocation() {}
};

class TempleLocation : public TownLocation {
private:
	int _currentCharLevel;
	int _donation;
	int _healCost;
	int _uncurseCost;
	int _dayOfWeek;
	int _v10, _v11, _v12;
	int _v13, _v14;
	bool _flag1;
	int _v5, _v6;
protected:
	/**
	* Generates the display text for the location, for a given character
	*/
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	TempleLocation();
	virtual ~TempleLocation() {}
};

class TrainingLocation : public TownLocation {
private:
	int _charIndex;
	bool _charsTrained[MAX_ACTIVE_PARTY];
	uint _experienceToNextLevel;
	uint _maxLevel;
protected:
	/**
	 * Generates the display text for the location, for a given character
	 */
	virtual Common::String createLocationText(Character &ch);

	/**
	 * Handles options for the particular location
	 */
	virtual Character *doOptions(Character *c);
public:
	TrainingLocation();
	virtual ~TrainingLocation() {}
};

class ArenaLocation : public TownLocation {
public:
	ArenaLocation();
	virtual ~ArenaLocation() {}
};

class ReaperLocation : public TownLocation {
public:
	ReaperLocation();
	virtual ~ReaperLocation() {}
};

class GolemLocation : public TownLocation {
public:
	GolemLocation();
	virtual ~GolemLocation() {}
};

class DwarfLocation : public TownLocation {
private:
	/**
	 * Set the new location
	 */
	bool setNewLocation();
public:
	DwarfLocation(bool isDwarf1);
	virtual ~DwarfLocation() {}

	/**
	 * Show the town location
	 */
	virtual int show();
};

class SphinxLocation : public TownLocation {
public:
	SphinxLocation();
	virtual ~SphinxLocation() {}
};

class PyramidLocation : public TownLocation {
public:
	PyramidLocation();
	virtual ~PyramidLocation() {}

	/**
	 * Show the town location
	 */
	virtual int show();
};

class Town {
private:
	TownLocation *_location;
private:
	int townWait();

	Character *doBankOptions(Character *c);

	Character *doBlacksmithOptions(Character *c);

	Character *doGuildOptions(Character *c);

	Character *doTavernOptions(Character *c);

	Character *doTempleOptions(Character *c);

	Character *doTrainingOptions(Character *c);
public:
	Town();

	/**
	 * Show a given location, and return any result
	 */
	int townAction(TownAction actionId);

	/**
	 * Returns true if a town location (bank, blacksmith, etc.) is currently active
	 */
	bool isActive() const;

	/**
	 * Draws a currently active town location's animation
	 */
	void drawAnim(bool flag);
};

class TownMessage : public TownLocation {
private:
	SpriteResource _iconSprites;

	TownMessage() : TownLocation(NO_ACTION) {}

	bool execute(int portrait, const Common::String &name,
		const Common::String &text, int confirm);

	void loadButtons();
public:
	static bool show(int portrait, const Common::String &name,
		const Common::String &text, int confirm);
};

} // End of namespace Xeen

#endif /* XEEN_SPELLS_H */
