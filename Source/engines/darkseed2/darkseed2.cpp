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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

// Base stuff
#include "common/endian.h"
#include "common/md5.h"
#include "common/random.h"
#include "base/plugins.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/macresman.h"
#include "common/textconsole.h"
#include "common/error.h"

#include "engines/util.h"

// Audio
#include "audio/mixer.h"
#include "audio/mididrv.h"

// Save/Load
#include "gui/saveload.h"
#include "darkseed2/saveload.h"

// Dark Seed II subsystems
#include "darkseed2/darkseed2.h"
#include "darkseed2/options.h"
#include "darkseed2/cursors.h"
#include "darkseed2/resources.h"
#include "darkseed2/font.h"
#include "darkseed2/script.h"
#include "darkseed2/imageconverter.h"
#include "darkseed2/graphics.h"
#include "darkseed2/room.h"
#include "darkseed2/conversationbox.h"
#include "darkseed2/sound.h"
#include "darkseed2/music.h"
#include "darkseed2/variables.h"
#include "darkseed2/talk.h"
#include "darkseed2/mike.h"
#include "darkseed2/movie.h"
#include "darkseed2/roomconfig.h"
#include "darkseed2/inter.h"
#include "darkseed2/events.h"

namespace DarkSeed2 {

// Files
static const char *kVariableIndex = "GAMEVAR";

DarkSeed2Engine::DarkSeed2Engine(OSystem *syst, const ADGameDescription *gameDesc) :
	Engine(syst), _gameDescription(gameDesc) {

	DebugMan.addDebugChannel(kDebugResources   , "Resources"   , "Resource handling debug level");
	DebugMan.addDebugChannel(kDebugGraphics    , "Graphics"    , "Graphics debug level");
	DebugMan.addDebugChannel(kDebugMusic       , "Music"       , "Music debug level");
	DebugMan.addDebugChannel(kDebugSound       , "Sound"       , "Sound debug level");
	DebugMan.addDebugChannel(kDebugTalk        , "Talk"        , "Talk debug level");
	DebugMan.addDebugChannel(kDebugMovie       , "Movie"       , "Movie debug level");
	DebugMan.addDebugChannel(kDebugScript      , "Script"      , "Script debug level");
	DebugMan.addDebugChannel(kDebugRooms       , "Rooms"       , "Rooms debug level");
	DebugMan.addDebugChannel(kDebugObjects     , "Objects"     , "Objects debug level");
	DebugMan.addDebugChannel(kDebugConversation, "Conversation", "Conversation debug level");
	DebugMan.addDebugChannel(kDebugOpcodes     , "Opcodes"     , "Script functions debug level");
	DebugMan.addDebugChannel(kDebugRoomConf    , "RoomConf"    , "Room config debug level");
	DebugMan.addDebugChannel(kDebugGameflow    , "Gameflow"    , "Gameflow debug level");

	// Setup mixer
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));

	_options        = 0;
	_cursors        = 0;
	_resources      = 0;
	_fontMan        = 0;
	_sound          = 0;
	_music          = 0;
	_variables      = 0;
	_scriptRegister = 0;
	_graphics       = 0;
	_talkMan        = 0;
	_mike           = 0;
	_movie          = 0;
	_roomConfMan    = 0;
	_inter          = 0;
	_events         = 0;
	_macExeResFork  = 0;
	_midiDriver     = 0;

	_rnd = new Common::RandomSource("darkseed2");

	_engineStartTime = 0;
	_playTime        = 0;

	const Common::FSNode gameDataDir(ConfMan.get("path"));
	SearchMan.addSubDirectoryMatching(gameDataDir, "sndtrack"); // Windows MIDI
}

DarkSeed2Engine::~DarkSeed2Engine() {
	if (_music)
		_music->stop();
	if (_sound)
		_sound->stopAll();

	_mixer->stopAll();

	delete _events;
	delete _inter;
	delete _movie;
	delete _mike;
	delete _talkMan;
	delete _graphics;
	delete _roomConfMan;

	delete _variables;
	delete _scriptRegister;
	delete _music;
	delete _sound;
	delete _fontMan;
	delete _resources;
	delete _cursors;
	delete _options;

	delete _midiDriver;

	delete _rnd;

	delete _macExeResFork;
}

Common::Error DarkSeed2Engine::run() {
	int32 width, height;
	if (!getScreenResolution(width, height))
		return Common::kUnknownError;

	if (!initGraphics(width, height))
		return Common::kUnknownError;

	if (!init(width, height))
		return Common::kUnknownError;

	if (!initGraphicsSystem())
		return Common::kUnknownError;

	debug(-1, "Done initializing.");

	_engineStartTime = g_system->getMillis();

	while (!shouldQuit()) {
		_events->setLoading(false);
		if (!_events->run()) {
			return Common::kUnknownError;
		}
	}

	return Common::kNoError;
}

void DarkSeed2Engine::pauseEngineIntern(bool pause) {
	_mixer->pauseAll(pause);
}

void DarkSeed2Engine::syncSoundSettings() {
	Engine::syncSoundSettings();

	// Use our music settings for plain audio
	_mixer->setVolumeForSoundType(Audio::Mixer::kPlainSoundType, ConfMan.getInt("music_volume"));

	_options->syncSettings();

	_talkMan->syncSettings(*_options);
}

bool DarkSeed2Engine::getScreenResolution(int32 &width, int32 &height) const {
	switch (getPlatform()) {
	case Common::kPlatformWindows:
	case Common::kPlatformMacintosh:
		width  = 640;
		height = 480;
		return true;
	case Common::kPlatformSaturn:
		width  = 320;
		height = 240;
		return true;
	case Common::kPlatformPSX:			
		warning("DarkSeed2Engine::getScreenResolution(): PSX Version Code, not supportet");
		break;	
	default:
		warning("DarkSeed2Engine::getScreenResolution(): Unknown game version");
		break;
	}

	return false;
}

bool DarkSeed2Engine::init(int32 width, int32 height) {
	uint32 midiDriver = MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);
	bool native_mt32 = ((MidiDriver::getMusicType(midiDriver) == MT_MT32) || ConfMan.getBool("native_mt32"));

	_midiDriver = MidiDriver::createMidi(midiDriver);

	if (native_mt32)
		_midiDriver->property(MidiDriver::PROP_CHANNEL_MASK, 0x03FE);

	debug(-1, "Creating subclasses...");

	if (getPlatform() == Common::kPlatformMacintosh) {
		// Open up the Mac resource fork for the executable
		_macExeResFork = new Common::MacResManager();
		if (!_macExeResFork->open("Dark Seed II/Dark Seed II")) {
			warning("Could not open 'Dark Seed II'");
			return false;
		}
	}

	_options         = new Options();
	_variables       = new Variables(*_rnd);
	_scriptRegister  = new ScriptRegister();
	_resources       = new Resources(getPlatform(), getLanguage(), isDemo());
	_fontMan         = new FontManager(*_resources);
	_sound           = new Sound(getPlatform(), *_mixer, *_variables);
	_music           = new Music(getPlatform(), *_mixer, *_midiDriver);

	// The cursors need to be created after Resources but before Graphics
	switch (getPlatform()) {
	case Common::kPlatformWindows:
		if (isDemo())
			_cursors = new CursorsWindows("ds2_demo.exe");
		else
			_cursors = new CursorsWindows("dark0001.exe");
		break;
	case Common::kPlatformSaturn:
		_cursors = new CursorsSaturn(*_resources);
		break;
	case Common::kPlatformMacintosh:
		_cursors = new CursorsMac(*_macExeResFork);
		break;
	default:
		warning("Unknown platform for cursors");
		return false;
	}

	_graphics        = new Graphics(width, height, *_resources, *_variables, *_cursors, *_fontMan);
	_talkMan         = new TalkManager(*_sound, *_graphics, *_fontMan, getPlatform());
	_mike            = new Mike(*_resources, *_variables, *_graphics);
	_movie           = new Movie(*_mixer, *_graphics, *_cursors, *_sound, getPlatform());
	_roomConfMan     = new RoomConfigManager(*this);
	_inter           = new ScriptInterpreter(*this);
	_events          = new Events(*this);

	syncSoundSettings();

	debug(-1, "Indexing resources...");

	if (!_resources->index()) {
		warning("DarkSeed2Engine::init(): Couldn't index resources");
		return false;
	}

	if (!_cursors->load()) {
		warning("DarkSeed2Engine::init(): Couldn't load cursors");
		return false;
	}

	if (!_fontMan->init(getPlatform(), getLanguage())) {
		warning("DarkSeed2Engine::init(): Couldn't initialize the font manager");
		return false;
	}

	if (!_events->init()) {
		warning("DarkSeed2Engine::init(): Couldn't initialize the event handler");
		return false;
	}

	debug(-1, "Initializing game variables...");

	if (getPlatform() == Common::kPlatformMacintosh) {
		Common::SeekableReadStream *stream = _macExeResFork->getResource(kVariableIndex);
		if (!_variables->loadFromIDX(*stream)) {
			delete stream;
			warning("DarkSeed2Engine::init(): Couldn't load initial variables values");
			return false;
		}
		delete stream;
	} else if (!_variables->loadFromIDX(*_resources, kVariableIndex)) {
		warning("DarkSeed2Engine::init(): Couldn't load initial variables values");
		return false;
	}

	bool needPalette = getPlatform() != Common::kPlatformSaturn;
	if (!_mike->init(needPalette)) {
		warning("DarkSeed2Engine::init(): Couldn't initialize Mike");
		return false;
	}

	return true;
}

bool DarkSeed2Engine::initGraphics(int32 width, int32 height) {
	debug(-1, "Setting up graphics...");


	::Graphics::PixelFormat _pixelformat;

	if (getPlatform() == Common::kPlatformWindows) {
		_pixelformat = ::Graphics::PixelFormat(4, 8, 8, 8, 0, 16, 8, 0, 0);
		::initGraphics(640, 480, &_pixelformat);
	}
	else {
		_pixelformat = ::Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
		::initGraphics(320, 240, &_pixelformat);
	}
	
	ImgConv.setPixelFormat(g_system->getScreenFormat());

	return true;
}

bool DarkSeed2Engine::initGraphicsSystem() {
	debug(-1, "Setting up the graphics system...");

	if (!_graphics->init(*_talkMan, *_scriptRegister, *_roomConfMan, *_movie))
		return false;

	return true;
}

bool DarkSeed2Engine::doLoadDialog() {
	const Plugin *plugin = nullptr;
	EngineMan.findGame(getGameId(), &plugin);
	assert(plugin);

	GUI::SaveLoadChooser *dialog = new GUI::SaveLoadChooser("Load game:", "Load", false);

	int slot = dialog->runModalWithPluginAndTarget(plugin, ConfMan.getActiveDomainName());

	bool result = ((slot >= 0) && (loadGameState(slot).getCode() == Common::kNoError));

	delete dialog;
	return result;
}

void DarkSeed2Engine::clearAll() {
	_movie->stop();
	_music->stop();
	_talkMan->endTalk();
	_sound->stopAll();
	_mike->setWalkMap();

	_graphics->unregisterBackground();
	_inter->clear();

	_graphics->getRoom().clear();
	_graphics->getConversationBox().stop();

	_scriptRegister->clear();
}

bool DarkSeed2Engine::canLoadGameStateCurrently() {
	// We can always load
	return true;
}

bool DarkSeed2Engine::canSaveGameStateCurrently() {
	// We can always save
	return true;
}

Common::Error DarkSeed2Engine::loadGameState(int slot) {
	SaveMetaInfo meta;

	Common::InSaveFile *file = SaveLoad::openForLoading(SaveLoad::createFileName(_targetName, slot));
	if (!file)
		return Common::kUnknownError;

	if (!SaveLoad::skipThumbnail(*file))
		return Common::kUnknownError;

	Common::Serializer serializer(file, 0);

	if (!saveLoad(serializer, meta))
		return Common::kUnknownError;

	delete file;

	_playTime = meta.getPlayTime();

	_events->setLoading(true);

	_graphics->retrace();
	g_system->updateScreen();

	return Common::kNoError;
}

Common::Error DarkSeed2Engine::saveGameState(int slot, const Common::String &desc) {
	_graphics->retrace();

	SaveMetaInfo meta;

	meta.description = desc;
	meta.fillWithCurrentTime(_engineStartTime, _playTime);

	Common::OutSaveFile *file = SaveLoad::openForSaving(SaveLoad::createFileName(_targetName, slot));
	if (!file)
		return Common::kUnknownError;

	if (!SaveLoad::saveThumbnail(*file))
		return Common::kUnknownError;

	Common::Serializer serializer(0, file);

	if (!saveLoad(serializer, meta))
		return Common::kUnknownError;

	bool flushed = file->flush();
	if (!flushed || file->err())
		return Common::kUnknownError;

	delete file;

	return Common::kNoError;
}

bool DarkSeed2Engine::saveLoad(Common::Serializer &serializer, SaveMetaInfo &meta) {
	if (!SaveLoad::syncMetaInfo(serializer, meta))
		return false;

	if (serializer.isLoading())
		clearAll();

	if (!_variables->doSaveLoad(serializer, *_resources))
		return false;

	if (!_music->doSaveLoad(serializer, *_resources))
		return false;

	if (!_scriptRegister->doSaveLoad(serializer, *_resources))
		return false;

	if (!_graphics->doSaveLoad(serializer, *_resources))
		return false;

	if (!_roomConfMan->doSaveLoad(serializer, *_resources))
		return false;

	if (!_movie->doSaveLoad(serializer, *_resources))
		return false;

	if (!_inter->doSaveLoad(serializer, *_resources))
		return false;

	if (!_mike->doSaveLoad(serializer, *_resources))
		return false;

	if (!_events->doSaveLoad(serializer, *_resources))
		return false;

	if (!_cursors->doSaveLoad(serializer, *_resources))
		return false;

	return true;
}

} // End of namespace DarkSeed2
