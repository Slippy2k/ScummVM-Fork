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
#include <stdio.h>
#include "common/debug.h"
#include "common/util.h"
#include "common/file.h"
#include "common/macresman.h"
#include "common/md5.h"
#include "common/config-manager.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "gui/EventRecorder.h"
#include "gui/gui-manager.h"
#include "engines/unknown-game-dialog.h"
#include "engines/advancedDetector.h"
#include "engines/obsolete.h"

static GameDescriptor toGameDescriptor(const ADGameDescription &g, const PlainGameDescriptor *sg) {
	const char *title = 0;
	const char *extra;

	if (g.flags & ADGF_USEEXTRAASTITLE) {
		title = g.extra;
		extra = "";
	} else {
		while (sg->gameId) {
			if (!scumm_stricmp(g.gameId, sg->gameId))
				title = sg->description;
			sg++;
		}

		extra = g.extra;
	}

	GameSupportLevel gsl = kStableGame;
	if (g.flags & ADGF_UNSTABLE)
		gsl = kUnstableGame;
	else if (g.flags & ADGF_TESTING)
		gsl = kTestingGame;

	GameDescriptor gd(g.gameId, title, g.language, g.platform, 0, gsl);
	gd.updateDesc(extra);
	return gd;
}

/**
 * Generate a preferred target value as
 *   GAMEID-PLAFORM-LANG
 * or (if ADGF_DEMO has been set)
 *   GAMEID-demo-PLAFORM-LANG
 */
static Common::String generatePreferredTarget(const Common::String &id, const ADGameDescription *desc) {
	Common::String res(id);

	if (desc->flags & ADGF_DEMO) {
		res = res + "-demo";
	}

	if (desc->flags & ADGF_CD) {
		res = res + "-cd";
	}

	if (desc->platform != Common::kPlatformDOS && desc->platform != Common::kPlatformUnknown && !(desc->flags & ADGF_DROPPLATFORM)) {
		res = res + "-" + getPlatformAbbrev(desc->platform);
	}

	if (desc->language != Common::EN_ANY && desc->language != Common::UNK_LANG && !(desc->flags & ADGF_DROPLANGUAGE)) {
		res = res + "-" + getLanguageCode(desc->language);
	}

	return res;
}

static Common::String sanitizeName(const char *name) {
	Common::String res;

	while (*name) {
		if (Common::isAlnum(*name))
			res += tolower(*name);
		name++;
	}

	return res;
}

void AdvancedMetaEngine::updateGameDescriptor(GameDescriptor &desc, const ADGameDescription *realDesc) const {
	if (_singleId != NULL) {
		desc["preferredtarget"] = desc["gameid"];
		desc["gameid"] = _singleId;
	}

	if (!desc.contains("preferredtarget"))
		desc["preferredtarget"] = desc["gameid"];

	if (realDesc->flags & ADGF_AUTOGENTARGET) {
		if (*realDesc->extra)
			desc["preferredtarget"] = sanitizeName(realDesc->extra);
	}

	desc["preferredtarget"] = generatePreferredTarget(desc["preferredtarget"], realDesc);

	if (_flags & kADFlagUseExtraAsHint)
		desc["extra"] = realDesc->extra;

	desc.setGUIOptions(realDesc->guiOptions + _guiOptions);
	desc.appendGUIOptions(getGameGUIOptionsDescriptionLanguage(realDesc->language));

	if (realDesc->flags & ADGF_ADDENGLISH)
		desc.appendGUIOptions(getGameGUIOptionsDescriptionLanguage(Common::EN_ANY));
}

bool cleanupPirated(ADGameDescList &matched) {
	// OKay, now let's sense presence of pirated games
	// if (!matched.empty()) {
		// for (uint j = 0; j < matched.size();) {
			// if (matched[j]->flags & ADGF_PIRATED)
				// matched.remove_at(j);
			// else
				// ++j;
		// }

		// // We ruled out all variants and now have nothing
		// if (matched.empty()) {
			// printf("\n\nIllegitimate Game Copy Detected. We provide no support in such cases\n");
			// return false;
		// }
	// }

	return false;
}


GameList AdvancedMetaEngine::detectGames(const Common::FSList &fslist, bool useUnknownGameDialog) const {
	ADGameDescList matches;
	GameList detectedGames;
	FileMap allFiles;

	if (fslist.empty())
		return detectedGames;

	// Compose a hashmap of all files in fslist.
	composeFileHashMap(allFiles, fslist, (_maxScanDepth == 0 ? 1 : _maxScanDepth));

	// Run the detector on this
       matches = detectGame(fslist.begin()->getParent(), allFiles, Common::UNK_LANG, Common::kPlatformUnknown, "", useUnknownGameDialog);

	if (matches.empty()) {
		// Use fallback detector if there were no matches by other means
		const ADGameDescription *fallbackDesc = fallbackDetect(allFiles, fslist);
		if (fallbackDesc != 0) {
			GameDescriptor desc(toGameDescriptor(*fallbackDesc, _gameIds));
			updateGameDescriptor(desc, fallbackDesc);
			detectedGames.push_back(desc);
		}
	} else {
		// Otherwise use the found matches
		cleanupPirated(matches);
		for (uint i = 0; i < matches.size(); i++) {
			GameDescriptor desc(toGameDescriptor(*matches[i], _gameIds));
			updateGameDescriptor(desc, matches[i]);
			detectedGames.push_back(desc);
		}
	}

	return detectedGames;
}

const ExtraGuiOptions AdvancedMetaEngine::getExtraGuiOptions(const Common::String &target) const {
	if (!_extraGuiOptions)
		return ExtraGuiOptions();

	ExtraGuiOptions options;

	// If there isn't any target specified, return all available GUI options.
	// Only used when an engine starts in order to set option defaults.
	if (target.empty()) {
		for (const ADExtraGuiOptionsMap *entry = _extraGuiOptions; entry->guioFlag; ++entry)
			options.push_back(entry->option);

		return options;
	}

	// Query the GUI options
	const Common::String guiOptionsString = ConfMan.get("guioptions", target);
	const Common::String guiOptions = parseGameGUIOptions(guiOptionsString);

	// Add all the applying extra GUI options.
	for (const ADExtraGuiOptionsMap *entry = _extraGuiOptions; entry->guioFlag; ++entry) {
		if (guiOptions.contains(entry->guioFlag))
			options.push_back(entry->option);
	}

	return options;
}

Common::Error AdvancedMetaEngine::createInstance(OSystem *syst, Engine **engine) const {
	assert(engine);

	const ADGameDescription *agdDesc = 0;
	Common::Language language = Common::UNK_LANG;
	Common::Platform platform = Common::kPlatformUnknown;
	Common::String extra;

	if (ConfMan.hasKey("language"))
		language = Common::parseLanguage(ConfMan.get("language"));
	if (ConfMan.hasKey("platform"))
		platform = Common::parsePlatform(ConfMan.get("platform"));
	if (_flags & kADFlagUseExtraAsHint) {
		if (ConfMan.hasKey("extra"))
			extra = ConfMan.get("extra");
	}

	Common::String gameid = ConfMan.get("gameid");

	Common::String path;
	if (ConfMan.hasKey("path")) {
		path = ConfMan.get("path");
	} else {
		path = ".";

		// This situation may happen only when game was
		// launched from a command line with wrong target and
		// no path was provided.
		//
		// A dummy entry will get created and will keep game path
		// We mark this entry, so it will not be added to the
		// config file.
		//
		// Fixes bug #1544799
		ConfMan.setBool("autoadded", true);

		warning("No path was provided. Assuming the data files are in the current directory");
	}
	Common::FSNode dir(path);
	Common::FSList files;
	if (!dir.isDirectory() || !dir.getChildren(files, Common::FSNode::kListAll, true)) {
		warning("Game data path does not exist or is not a directory (%s)", path.c_str());
		return Common::kNoGameDataFoundError;
	}

	if (files.empty())
		return Common::kNoGameDataFoundError;

	// Compose a hashmap of all files in fslist.
	FileMap allFiles;
	composeFileHashMap(allFiles, files, (_maxScanDepth == 0 ? 1 : _maxScanDepth));

	// Run the detector on this
	ADGameDescList matches = detectGame(files.begin()->getParent(), allFiles, language, platform, extra);

	if (cleanupPirated(matches))
		return Common::kNoGameDataFoundError;

	if (_singleId == NULL) {
		// Find the first match with correct gameid.
		for (uint i = 0; i < matches.size(); i++) {
			if (matches[i]->gameId == gameid) {
				agdDesc = matches[i];
				break;
			}
		}
	} else if (matches.size() > 0) {
		agdDesc = matches[0];
	}

	if (agdDesc == 0) {
		// Use fallback detector if there were no matches by other means
		agdDesc = fallbackDetect(allFiles, files);
		if (agdDesc != 0) {
			// Seems we found a fallback match. But first perform a basic
			// sanity check: the gameid must match.
			if (_singleId == NULL && agdDesc->gameId != gameid)
				agdDesc = 0;
		}
	}

	if (agdDesc == 0)
		return Common::kNoGameDataFoundError;

	// If the GUI options were updated, we catch this here and update them in the users config
	// file transparently.
	Common::String lang = getGameGUIOptionsDescriptionLanguage(agdDesc->language);
	if (agdDesc->flags & ADGF_ADDENGLISH)
		lang += " " + getGameGUIOptionsDescriptionLanguage(Common::EN_ANY);

	Common::updateGameGUIOptions(agdDesc->guiOptions + _guiOptions, lang);

	GameDescriptor gameDescriptor = toGameDescriptor(*agdDesc, _gameIds);

	bool showTestingWarning = false;

#ifdef RELEASE_BUILD
	showTestingWarning = true;
#endif

	if (((gameDescriptor.getSupportLevel() == kUnstableGame
			|| (gameDescriptor.getSupportLevel() == kTestingGame
					&& showTestingWarning)))
			&& !Engine::warnUserAboutUnsupportedGame())
		return Common::kUserCanceled;

	debug(2, "Running %s", gameDescriptor.description().c_str());
	initSubSystems(agdDesc);
	if (!createInstance(syst, engine, agdDesc))
		return Common::kNoGameDataFoundError;
	else
		return Common::kNoError;
}

void AdvancedMetaEngine::reportUnknown(const Common::FSNode &path, const ADFilePropertiesMap &filesProps, const ADGameIdList &matchedGameIds, bool useUnknownGameDialog) const {
	const char *reportCommon = _s("The game in '%s' seems to be an unknown %s engine game "
							   "variant.\n\nPlease report the following data to the ScummVM "
							   "team at %s along with the name of the game you tried to add and "
							   "its version, language, etc.:");
	Common::String report           = Common::String::format(reportCommon, path.getPath().c_str(), getName(), "https://bugs.scummvm.org/");
	Common::String reportTranslated = Common::String::format(_(reportCommon), path.getPath().c_str(), getName(), "https://bugs.scummvm.org/");
	Common::String bugtrackerAffectedEngine = getName();

	if (matchedGameIds.size()) {
		report += "\n\n";
		reportTranslated += "\n\n";
		report += "Matched game IDs:";
		reportTranslated += _("Matched game IDs:");
		report += " ";
		reportTranslated += " ";
		report += " ";

		for (ADGameIdList::const_iterator gameId = matchedGameIds.begin(); gameId != matchedGameIds.end(); ++gameId) {
			if (gameId != matchedGameIds.begin()) {
				reportTranslated += ", ";
			}
			reportTranslated += *gameId;
		}
	}

	reportTranslated += "\n\n";
	
	reportTranslated.wordWrap(65);
	Common::String reportLog = report;
	reportLog.wordWrap(80);

	Common::String unknownFiles;
	
	for (ADFilePropertiesMap::const_iterator file = filesProps.begin(); file != filesProps.end(); ++file)
		unknownFiles += Common::String::format("  {\"%s\", 0, \"%s\", %d},\n", file->_key.c_str(), file->_value.md5.c_str(), file->_value.size);

	report += unknownFiles;
	reportTranslated += unknownFiles;
	reportLog += unknownFiles + "\n";

	// Write the original message about the unknown game to the log file
	g_system->logMessage(LogMessageType::kInfo, reportLog.c_str());

	// Check if the GUI is running, show the UnknownGameDialog and print the translated unknown game information
	if (GUI::GuiManager::hasInstance() && g_gui.isActive() && useUnknownGameDialog == true) {
		UnknownGameDialog dialog(report, reportTranslated, bugtrackerAffectedEngine);
		dialog.runModal();
	}
}

void AdvancedMetaEngine::composeFileHashMap(FileMap &allFiles, const Common::FSList &fslist, int depth, const Common::String &parentName) const {
	if (depth <= 0)
		return;

	if (fslist.empty())
		return;

	for (Common::FSList::const_iterator file = fslist.begin(); file != fslist.end(); ++file) {
		Common::String tstr = (_matchFullPaths && !parentName.empty() ? parentName + "/" : "") + file->getName();

		if (file->isDirectory()) {
			Common::FSList files;

			if (!_directoryGlobs)
				continue;

			bool matched = false;
			for (const char * const *glob = _directoryGlobs; *glob; glob++)
				if (file->getName().matchString(*glob, true)) {
					matched = true;
					break;
				}

			if (!matched)
				continue;

			if (!file->getChildren(files, Common::FSNode::kListAll))
				continue;

			composeFileHashMap(allFiles, files, depth - 1, tstr);
		}

		// Strip any trailing dot
		if (tstr.lastChar() == '.')
			tstr.deleteLastChar();

		allFiles[tstr] = *file;	// Record the presence of this file
	}
}

bool AdvancedMetaEngine::getFileProperties(const Common::FSNode &parent, const FileMap &allFiles, const ADGameDescription &game, const Common::String fname, ADFileProperties &fileProps) const {
	// FIXME/TODO: We don't handle the case that a file is listed as a regular
	// file and as one with resource fork.

	if (game.flags & ADGF_MACRESFORK) {
		Common::MacResManager macResMan;

		if (!macResMan.open(parent, fname))
			return false;

		fileProps.md5 = macResMan.computeResForkMD5AsString(_md5Bytes);
		fileProps.size = macResMan.getResForkDataSize();

		if (fileProps.size != 0)
			return true;
	}

	if (!allFiles.contains(fname))
		return false;

	Common::File testFile;

	if (!testFile.open(allFiles[fname]))
		return false;

	fileProps.size = (int32)testFile.size();
	fileProps.md5 = Common::computeStreamMD5AsString(testFile, _md5Bytes);
	return true;
}

ADGameDescList AdvancedMetaEngine::detectGame(const Common::FSNode &parent, const FileMap &allFiles, Common::Language language, Common::Platform platform, const Common::String &extra, bool useUnknownGameDialog) const {
	ADFilePropertiesMap filesProps;

	const ADGameFileDescription *fileDesc;
	const ADGameDescription *g;
	const byte *descPtr;
	//debug("\n\n");
	debug(3,"Starting detection in dir '%s'", parent.getPath().c_str());

	// Check which files are included in some ADGameDescription *and* are present.
	// Compute MD5s and file sizes for these files.
	for (descPtr = _gameDescriptors; ((const ADGameDescription *)descPtr)->gameId != 0; descPtr += _descItemSize) {
		g = (const ADGameDescription *)descPtr;

		for (fileDesc = g->filesDescriptions; fileDesc->fileName; fileDesc++) {
			Common::String fname = fileDesc->fileName;
			ADFileProperties tmp;

			if (filesProps.contains(fname))
				continue;

			if (getFileProperties(parent, allFiles, *g, fname, tmp)) {
				debug(3,"> '%s': '%s'", fname.c_str(), tmp.md5.c_str());
				filesProps[fname] = tmp;
			}
		}
	}

	ADGameDescList matched;
	ADGameIdList matchedGameIds;
	int maxFilesMatched = 0;
	bool gotAnyMatchesWithAllFiles = false;

	// MD5 based matching
	uint i;
	for (i = 0, descPtr = _gameDescriptors; ((const ADGameDescription *)descPtr)->gameId != 0; descPtr += _descItemSize, ++i) {
		g = (const ADGameDescription *)descPtr;
		bool fileMissing = false;

		// Do not even bother to look at entries which do not have matching
		// language and platform (if specified).
		if ((language != Common::UNK_LANG && g->language != Common::UNK_LANG && g->language != language
			 && !(language == Common::EN_ANY && (g->flags & ADGF_ADDENGLISH))) ||
			(platform != Common::kPlatformUnknown && g->platform != Common::kPlatformUnknown && g->platform != platform)) {
			continue;
		}

		if ((_flags & kADFlagUseExtraAsHint) && !extra.empty() && g->extra != extra)
			continue;

		bool allFilesPresent = true;
		int curFilesMatched = 0;
		bool hashOrSizeMismatch = false;

		// Try to match all files for this game
		for (fileDesc = g->filesDescriptions; fileDesc->fileName; fileDesc++) {
			Common::String tstr = fileDesc->fileName;

			if (!filesProps.contains(tstr)) {
				fileMissing = true;
				allFilesPresent = false;
				break;
			}

			if (hashOrSizeMismatch)
				continue;

			if (fileDesc->md5 != NULL && fileDesc->md5 != filesProps[tstr].md5) {
				debug("MD5 Mismatch. Skipping (%s) (%s) File: %s", fileDesc->md5, filesProps[tstr].md5.c_str(),tstr.c_str());
				fileMissing = true;
				hashOrSizeMismatch = true;
				continue;
			}

			if (fileDesc->fileSize != -1 && fileDesc->fileSize != filesProps[tstr].size) {
				debug("Size Mismatch. Skipping (%d) (%d) File: %s", fileDesc->fileSize, filesProps[tstr].size,tstr.c_str());
				fileMissing = true;
				hashOrSizeMismatch = true;
				continue;
			}

			debug("Matched file: %s \t\t%s", tstr.c_str(), fileDesc->md5);
			curFilesMatched++;
		}

		// We found at least one entry with all required files present.
		// That means that we got new variant of the game.
		//
		// Without this check we would have erroneous checksum display
		// where only located files will be enlisted.
		//
		// Potentially this could rule out variants where some particular file
		// is really missing, but the developers should better know about such
		// cases.
		if (allFilesPresent) {
			gotAnyMatchesWithAllFiles = true;
			if (!matchedGameIds.size() || strcmp(matchedGameIds.back(), g->gameId) != 0)
				matchedGameIds.push_back(g->gameId);
		}

		if (!fileMissing) {
			debug("\n\nFound game: %s (%s %s/%s) (%d)", g->gameId, g->extra,
			 getPlatformDescription(g->platform), getLanguageDescription(g->language), i);

			if (curFilesMatched > maxFilesMatched) {
				debug("\n ... new best match, removing all previous candidates");
				maxFilesMatched = curFilesMatched;

				matched.clear();	// Remove any prior, lower ranked matches.
				matched.push_back(g);
			} else if (curFilesMatched == maxFilesMatched) {
				matched.push_back(g);
			} else {
				debug(" ... skipped");
			}

		} else {
			debug(5, "Skipping game: %s (%s %s/%s) (%d)", g->gameId, g->extra,
			 getPlatformDescription(g->platform), getLanguageDescription(g->language), i);
		}
	}

	// We didn't find a match
	if (matched.empty()) {
		if (!filesProps.empty() && gotAnyMatchesWithAllFiles) {
			reportUnknown(parent, filesProps, matchedGameIds, useUnknownGameDialog);
		}

		// Filename based fallback
	}

	return matched;
}

const ADGameDescription *AdvancedMetaEngine::detectGameFilebased(const FileMap &allFiles, const Common::FSList &fslist, const ADFileBasedFallback *fileBasedFallback, ADFilePropertiesMap *filesProps) const {
	const ADFileBasedFallback *ptr;
	const char* const* filenames;

	int maxNumMatchedFiles = 0;
	const ADGameDescription *matchedDesc = 0;

	for (ptr = fileBasedFallback; ptr->desc; ++ptr) {
		const ADGameDescription *agdesc = ptr->desc;
		int numMatchedFiles = 0;
		bool fileMissing = false;

		for (filenames = ptr->filenames; *filenames; ++filenames) {
			debug(3, "++ %s", *filenames);
			if (!allFiles.contains(*filenames)) {
				fileMissing = true;
				break;
			}

			numMatchedFiles++;
		}

		if (!fileMissing) {
			debug("\n\nMatched: %s", agdesc->gameId);

			if (numMatchedFiles > maxNumMatchedFiles) {
				matchedDesc = agdesc;
				maxNumMatchedFiles = numMatchedFiles;

				debug("and overridden");

				if (filesProps) {
					for (filenames = ptr->filenames; *filenames; ++filenames) {
						ADFileProperties tmp;

						if (getFileProperties(fslist.begin()->getParent(), allFiles, *agdesc, *filenames, tmp))
							(*filesProps)[*filenames] = tmp;
					}
				}

			}
		}
	}

	return matchedDesc;
}

GameList AdvancedMetaEngine::getSupportedGames() const {
	if (_singleId != NULL) {
		GameList gl;

		const PlainGameDescriptor *g = _gameIds;
		while (g->gameId) {
			if (0 == scumm_stricmp(_singleId, g->gameId)) {
				gl.push_back(GameDescriptor(g->gameId, g->description));

				return gl;
			}
			g++;
		}
		error("Engine %s doesn't have its singleid specified in ids list", _singleId);
	}

	return GameList(_gameIds);
}

GameDescriptor AdvancedMetaEngine::findGame(const char *gameId) const {
	// First search the list of supported gameids for a match.
	const PlainGameDescriptor *g = findPlainGameDescriptor(gameId, _gameIds);
	if (g)
		return GameDescriptor(*g);

	// No match found
	return GameDescriptor();
}

AdvancedMetaEngine::AdvancedMetaEngine(const void *descs, uint descItemSize, const PlainGameDescriptor *gameIds, const ADExtraGuiOptionsMap *extraGuiOptions)
	: _gameDescriptors((const byte *)descs), _descItemSize(descItemSize), _gameIds(gameIds),
	  _extraGuiOptions(extraGuiOptions) {

	_md5Bytes = 5000;
	_singleId = NULL;
	_flags = 0;
	_guiOptions = GUIO_NONE;
	_maxScanDepth = 1;
	_directoryGlobs = NULL;
	_matchFullPaths = false;
}

void AdvancedMetaEngine::initSubSystems(const ADGameDescription *gameDesc) const {
#ifdef ENABLE_EVENTRECORDER
	if (gameDesc) {
		g_eventRec.processGameDescription(gameDesc);
	}
#endif
}
