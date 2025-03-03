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

#include "common/scummsys.h"

#if defined(SDL_BACKEND)

#include "backends/graphics/surfacesdl/surfacesdl-graphics.h"
#include "backends/events/sdl/sdl-events.h"
#include "backends/platform/sdl/sdl.h"
#include "common/config-manager.h"
#include "common/mutex.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/util.h"
#include "common/frac.h"
#ifdef USE_RGB_COLOR
#include "common/list.h"
#endif
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/scaler.h"
#include "graphics/scaler/aspect.h"
#include "graphics/surface.h"
#include "gui/EventRecorder.h"
#ifdef USE_PNG
#include "common/file.h"
#include "image/png.h"
#endif

static const OSystem::GraphicsMode s_supportedShaders[] = {
	{"NONE", "Normal (no shader)", 0},
	{0, 0, 0}
};

static const OSystem::GraphicsMode s_supportedGraphicsModes[] = {
	{"1x", _s("Normal (no scaling)"), GFX_NORMAL},
#ifdef USE_SCALERS
	{"2x", "2x", GFX_DOUBLESIZE},
	{"3x", "3x", GFX_TRIPLESIZE},
	{"3x", "3x", GFX_TRIPLESIZE},
// --------------------------------------- New Scaler Begin		
 	{"4x", "4x", GFX_QUADSIZE},
	{"5x", "5x", GFX_PENTASIZE},
	{"6x", "6x", GFX_HEXASIZE},	
// --------------------------------------- New Scaler end
	{ "tv2x", "TV2x", GFX_TV2X },
// --------------------------------------- New Scaler Begin	
	{ "scan2x20", "Scanlines2x  20%", GFX_SCAN2X020 },
	{ "scan2x40", "Scanlines2x  40%", GFX_SCAN2X040 },
	{ "scan2x60", "Scanlines2x  60%", GFX_SCAN2X060 },
	{ "scan2x80", "Scanlines2x  80%", GFX_SCAN2X080 },
	{ "scan2x100","Scanlines2x 100%", GFX_SCAN2X100 },
	{ "tv3x", "TV3x", GFX_TV3X },
	{ "scan3x20", "Scanlines3x  20%", GFX_SCAN3X020 },
	{ "scan3x40", "Scanlines3x  40%", GFX_SCAN3X040 },
	{ "scan3x60", "Scanlines3x  60%", GFX_SCAN3X060 },
	{ "scan3x80", "Scanlines3x  80%", GFX_SCAN3X080 },
	{ "scan3x100","Scanlines3x 100%", GFX_SCAN3X100 },
	{ "tv4x", "TV4x", GFX_TV4X },
	{ "scan4x20", "Scanlines4x  20%", GFX_SCAN4X020 },
	{ "scan4x40", "Scanlines4x  40%", GFX_SCAN4X040 },
	{ "scan4x60", "Scanlines4x  60%", GFX_SCAN4X060 },
	{ "scan4x80", "Scanlines4x  80%", GFX_SCAN4X080 },
	{ "scan4x100","Scanlines4x 100%", GFX_SCAN4X100 },	
	{ "tv5x", "TV5x", GFX_TV5X },
	{ "scan5x20", "Scanlines5x  20%", GFX_SCAN5X020 },
	{ "scan5x40", "Scanlines5x  40%", GFX_SCAN5X040 },
	{ "scan5x60", "Scanlines5x  60%", GFX_SCAN5X060 },
	{ "scan5x80", "Scanlines5x  80%", GFX_SCAN5X080 },
	{ "scan5x100","Scanlines5x 100%", GFX_SCAN5X100 },	
	{ "tv6x", "TV6x", GFX_TV6X },
	{ "scan6x20", "Scanlines6x  20%", GFX_SCAN6X020 },
	{ "scan6x40", "Scanlines6x  40%", GFX_SCAN6X040 },
	{ "scan6x60", "Scanlines6x  60%", GFX_SCAN6X060 },
	{ "scan6x80", "Scanlines6x  80%", GFX_SCAN6X080 },
	{ "scan6x100","Scanlines6x 100%", GFX_SCAN6X100 },		
// --------------------------------------- New Scaler ENd	
	{ "dotmatrix", "DotMatrix", GFX_DOTMATRIX },
// --------------------------------------- New Scaler Begin		
	{ "rgb2x","RGB2x", GFX_RGB2X },
	{ "rgb3x","RGB3x", GFX_RGB3X },	
	{ "rgb4x","RGB4x", GFX_RGB4X },		
// --------------------------------------- New Scaler End	
	{"2xsai", "2xSAI", GFX_2XSAI},
	{"super2xsai", "Super2xSAI", GFX_SUPER2XSAI},
	{"supereagle", "SuperEagle", GFX_SUPEREAGLE},
	{"advmame2x", "AdvMAME2x", GFX_ADVMAME2X},
	{"advmame3x", "AdvMAME3x", GFX_ADVMAME3X},
// --------------------------------------- New Scaler Begin	
	{"advmame4x", "AdvMAME4x", GFX_ADVMAME4X},
	{"advmame6x", "AdvMAME6x", GFX_ADVMAME6X},	
// --------------------------------------- New Scaler ENd	
#ifdef USE_HQ_SCALERS
	{"hq2x", "HQ2x", GFX_HQ2X},
	{"hq3x", "HQ3x", GFX_HQ3X},
#endif
#endif
	{0, 0, 0}
};

DECLARE_TRANSLATION_ADDITIONAL_CONTEXT("Normal (no scaling)", "lowres")

// Table of the cursor scalers [scaleFactor - 1]
static ScalerProc *scalersMagn[6] = {
#ifdef USE_SCALERS
	Normal1x, AdvMame2x, AdvMame3x, AdvMame4x, Normal5x, Normal6x
#else // remove dependencies on other scalers
	Normal1x, Normal1x, Normal1x, Normal1x, Normal1x, Normal1x
#endif
};

static const int s_gfxModeSwitchTable[][7] = {
// --------------------------------------- New Scaler Begin		
		{ GFX_NORMAL, GFX_DOUBLESIZE, GFX_TRIPLESIZE, GFX_QUADSIZE, GFX_PENTASIZE, GFX_HEXASIZE, -1 },
		{ GFX_NORMAL, GFX_TV2X,  GFX_TV3X,  GFX_TV4X,  GFX_TV5X,  GFX_TV6X,-1, },		
		{ GFX_NORMAL, GFX_SCAN2X020, GFX_SCAN3X020,  GFX_SCAN4X020,  GFX_SCAN5X020,  GFX_SCAN6X020,-1 },	
		{ GFX_NORMAL, GFX_SCAN2X040, GFX_SCAN3X040,  GFX_SCAN4X020,  GFX_SCAN5X040,  GFX_SCAN6X040,-1 },
		{ GFX_NORMAL, GFX_SCAN2X060, GFX_SCAN3X060,  GFX_SCAN4X060,  GFX_SCAN5X060,  GFX_SCAN6X060,-1 },			
		{ GFX_NORMAL, GFX_SCAN2X080, GFX_SCAN3X080,  GFX_SCAN4X080,  GFX_SCAN5X080,  GFX_SCAN6X080,-1 },
		{ GFX_NORMAL, GFX_SCAN2X100, GFX_SCAN3X100,  GFX_SCAN4X100,  GFX_SCAN5X100,  GFX_SCAN6X100,-1 },	
		{ GFX_NORMAL, GFX_RGB2X, GFX_RGB3X, GFX_RGB4X, -1,-1,-1},		
		{ GFX_NORMAL, GFX_DOTMATRIX, -1, -1,-1,-1,-1 },							
		{ GFX_NORMAL, GFX_ADVMAME2X, GFX_ADVMAME3X, GFX_ADVMAME4X, GFX_PENTASIZE, GFX_ADVMAME6X, -1 },
		{ GFX_NORMAL, GFX_HQ2X, GFX_HQ3X, -1,-1,-1,-1},
		{ GFX_NORMAL, GFX_2XSAI, -1, -1,-1,-1,-1 },
		{ GFX_NORMAL, GFX_SUPER2XSAI, -1, -1,-1,-1,-1 },
		{ GFX_NORMAL, GFX_SUPEREAGLE, -1, -1,-1,-1,-1 }
};
// --------------------------------------- New Scaler End

#ifdef USE_SCALERS
static int cursorStretch200To240(uint8 *buf, uint32 pitch, int width, int height, int srcX, int srcY, int origSrcY);
#endif

AspectRatio::AspectRatio(int w, int h) {
	// TODO : Validation and so on...
	// Currently, we just ensure the program don't instantiate non-supported aspect ratios
	_kw = w;
	_kh = h;
}

#if !defined(_WIN32_WCE) && !defined(__SYMBIAN32__) && defined(USE_SCALERS)
static AspectRatio getDesiredAspectRatio() {
	const size_t AR_COUNT = 4;
	const char *desiredAspectRatioAsStrings[AR_COUNT] = {	"auto",				"4/3",				"16/9",				"16/10" };
	const AspectRatio desiredAspectRatios[AR_COUNT] = {		AspectRatio(0, 0),	AspectRatio(4,3),	AspectRatio(16,9),	AspectRatio(16,10) };

	//TODO : We could parse an arbitrary string, if we code enough proper validation
	Common::String desiredAspectRatio = ConfMan.get("desired_screen_aspect_ratio");

	for (size_t i = 0; i < AR_COUNT; i++) {
		assert(desiredAspectRatioAsStrings[i] != NULL);

		if (!scumm_stricmp(desiredAspectRatio.c_str(), desiredAspectRatioAsStrings[i])) {
			return desiredAspectRatios[i];
		}
	}
	// TODO : Report a warning
	return AspectRatio(0, 0);
}
#endif

SurfaceSdlGraphicsManager::SurfaceSdlGraphicsManager(SdlEventSource *sdlEventSource, SdlWindow *window)
	:
	SdlGraphicsManager(sdlEventSource, window),
#ifdef USE_OSD
	_osdMessageSurface(nullptr), _osdMessageAlpha(SDL_ALPHA_TRANSPARENT), _osdMessageFadeStartTime(0),
	_osdIconSurface(nullptr),
#endif
#if SDL_VERSION_ATLEAST(2, 0, 0)
	_renderer(nullptr), _screenTexture(nullptr),
#endif
#if defined(WIN32) && !SDL_VERSION_ATLEAST(2, 0, 0)
	_originalBitsPerPixel(0),
#endif
	_screen(0), _tmpscreen(0),
	_screenFormat(Graphics::PixelFormat::createFormatCLUT8()),
	_cursorFormat(Graphics::PixelFormat::createFormatCLUT8()), 
	_overlayscreen(0), _tmpscreen2(0),
	_scalerProc(0), _screenChangeCount(0),
	_mouseData(nullptr), _mouseSurface(nullptr),
	_mouseOrigSurface(nullptr), _cursorDontScale(false), _cursorPaletteDisabled(true),
	_currentShakePos(0), _newShakePos(0),
	_paletteDirtyStart(0), _paletteDirtyEnd(0),
	_screenIsLocked(false),
	_graphicsMutex(0),
	_displayDisabled(false),
#ifdef USE_SDL_DEBUG_FOCUSRECT
	_enableFocusRectDebugCode(false), _enableFocusRect(false), _focusRect(),
#endif
	_transactionMode(kTransactionNone) {

	// allocate palette storage
	_currentPalette = (SDL_Color *)calloc(sizeof(SDL_Color), 256);
	_cursorPalette = (SDL_Color *)calloc(sizeof(SDL_Color), 256);

	_mouseBackup.x = _mouseBackup.y = _mouseBackup.w = _mouseBackup.h = 0;

	memset(&_mouseCurState, 0, sizeof(_mouseCurState));

	_graphicsMutex = g_system->createMutex();

#ifdef USE_SDL_DEBUG_FOCUSRECT
	if (ConfMan.hasKey("use_sdl_debug_focusrect"))
		_enableFocusRectDebugCode = ConfMan.getBool("use_sdl_debug_focusrect");
#endif

	memset(&_oldVideoMode, 0, sizeof(_oldVideoMode));
	memset(&_videoMode, 0, sizeof(_videoMode));
	memset(&_transactionDetails, 0, sizeof(_transactionDetails));

#if !defined(_WIN32_WCE) && !defined(__SYMBIAN32__) && defined(USE_SCALERS)
	_videoMode.mode = GFX_DOUBLESIZE;
	_videoMode.scaleFactor = 2;
	_videoMode.aspectRatioCorrection = ConfMan.getBool("aspect_ratio");
	_videoMode.desiredAspectRatio = getDesiredAspectRatio();
	_scalerProc = Normal2x;
#else // for small screen platforms
	_videoMode.mode = GFX_NORMAL;
	_videoMode.scaleFactor = 1;
	_videoMode.aspectRatioCorrection = false;
	_scalerProc = Normal1x;
#endif
	_scalerType = 0;

#if !defined(_WIN32_WCE) && !defined(__SYMBIAN32__)
	_videoMode.fullscreen = ConfMan.getBool("fullscreen");
#else
	_videoMode.fullscreen = true;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
	_videoMode.filtering = ConfMan.getBool("filtering");
#endif

	// the default backend has no shaders
	// shader number 0 is the entry NONE (no shader)
	// for an example on shader support,
	// consult the psp2sdl backend which inherits from this class
	_currentShader = 0;
	_numShaders = 1;
}

SurfaceSdlGraphicsManager::~SurfaceSdlGraphicsManager() {
	unloadGFXMode();
	if (_mouseOrigSurface) {







		SDL_FreeSurface(_mouseOrigSurface);
		if (_mouseOrigSurface == _mouseSurface) {
			_mouseSurface = nullptr;
		}
	}
	if (_mouseSurface) {
		SDL_FreeSurface(_mouseSurface);
	}
	g_system->deleteMutex(_graphicsMutex);

	free(_currentPalette);
	free(_cursorPalette);
	delete[] _mouseData;
}

void SurfaceSdlGraphicsManager::activateManager() {
	SdlGraphicsManager::activateManager();

	// Register the graphics manager as a event observer
	g_system->getEventManager()->getEventDispatcher()->registerObserver(this, 10, false);
}

void SurfaceSdlGraphicsManager::deactivateManager() {
	// Unregister the event observer
	if (g_system->getEventManager()->getEventDispatcher()) {
		g_system->getEventManager()->getEventDispatcher()->unregisterObserver(this);
	}

	SdlGraphicsManager::deactivateManager();
}

bool SurfaceSdlGraphicsManager::hasFeature(OSystem::Feature f) const {
	return
		(f == OSystem::kFeatureFullscreenMode) ||
		(f == OSystem::kFeatureAspectRatioCorrection) ||
#if SDL_VERSION_ATLEAST(2, 0, 0)
		(f == OSystem::kFeatureFilteringMode) ||
#endif
		(f == OSystem::kFeatureCursorPalette) ||
		(f == OSystem::kFeatureIconifyWindow);
}

void SurfaceSdlGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		setFullscreenMode(enable);
		break;
	case OSystem::kFeatureAspectRatioCorrection:
		setAspectRatioCorrection(enable);
		break;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	case OSystem::kFeatureFilteringMode:
		setFilteringMode(enable);
		break;
#endif
	case OSystem::kFeatureCursorPalette:
		_cursorPaletteDisabled = !enable;
		blitCursor();
		break;
	case OSystem::kFeatureIconifyWindow:
		if (enable)
			_window->iconifyWindow();
		break;
	default:
		break;
	}
}

bool SurfaceSdlGraphicsManager::getFeatureState(OSystem::Feature f) const {
	// We need to allow this to be called from within a transaction, since we
	// currently use it to retreive the graphics state, when switching from
	// SDL->OpenGL mode for example.
	//assert(_transactionMode == kTransactionNone);

	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		return _videoMode.fullscreen;
	case OSystem::kFeatureAspectRatioCorrection:
		return _videoMode.aspectRatioCorrection;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	case OSystem::kFeatureFilteringMode:
		return _videoMode.filtering;
#endif
	case OSystem::kFeatureCursorPalette:
		return !_cursorPaletteDisabled;
	default:
		return false;
	}
}

const OSystem::GraphicsMode *SurfaceSdlGraphicsManager::getSupportedGraphicsModes() const {
	return s_supportedGraphicsModes;
}

int SurfaceSdlGraphicsManager::getDefaultGraphicsMode() const {
#ifdef USE_SCALERS
	return GFX_DOUBLESIZE;
#else
	return GFX_NORMAL;
#endif
}

void SurfaceSdlGraphicsManager::resetGraphicsScale() {
	setGraphicsMode(s_gfxModeSwitchTable[_scalerType][0]);
}

void SurfaceSdlGraphicsManager::beginGFXTransaction() {
	assert(_transactionMode == kTransactionNone);

	_transactionMode = kTransactionActive;

	_transactionDetails.sizeChanged = false;

	_transactionDetails.needHotswap = false;
	_transactionDetails.needUpdatescreen = false;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	_transactionDetails.needTextureUpdate = false;
#endif
#ifdef USE_RGB_COLOR
	_transactionDetails.formatChanged = false;
#endif

	_oldVideoMode = _videoMode;
}

OSystem::TransactionError SurfaceSdlGraphicsManager::endGFXTransaction() {
	int errors = OSystem::kTransactionSuccess;

	assert(_transactionMode != kTransactionNone);

	if (_transactionMode == kTransactionRollback) {
		if (_videoMode.fullscreen != _oldVideoMode.fullscreen) {
			errors |= OSystem::kTransactionFullscreenFailed;

			_videoMode.fullscreen = _oldVideoMode.fullscreen;
		} else if (_videoMode.aspectRatioCorrection != _oldVideoMode.aspectRatioCorrection) {
			errors |= OSystem::kTransactionAspectRatioFailed;

			_videoMode.aspectRatioCorrection = _oldVideoMode.aspectRatioCorrection;
		} else if (_videoMode.mode != _oldVideoMode.mode) {
			errors |= OSystem::kTransactionModeSwitchFailed;

			_videoMode.mode = _oldVideoMode.mode;
			_videoMode.scaleFactor = _oldVideoMode.scaleFactor;
#if SDL_VERSION_ATLEAST(2, 0, 0)
		} else if (_videoMode.filtering != _oldVideoMode.filtering) {
			errors |= OSystem::kTransactionFilteringFailed;

			_videoMode.filtering = _oldVideoMode.filtering;
#endif
#ifdef USE_RGB_COLOR
		} else if (_videoMode.format != _oldVideoMode.format) {
			errors |= OSystem::kTransactionFormatNotSupported;

			_videoMode.format = _oldVideoMode.format;
			_screenFormat = _videoMode.format;
#endif
		} else if (_videoMode.screenWidth != _oldVideoMode.screenWidth || _videoMode.screenHeight != _oldVideoMode.screenHeight) {
			errors |= OSystem::kTransactionSizeChangeFailed;

			_videoMode.screenWidth = _oldVideoMode.screenWidth;
			_videoMode.screenHeight = _oldVideoMode.screenHeight;
			_videoMode.overlayWidth = _oldVideoMode.overlayWidth;
			_videoMode.overlayHeight = _oldVideoMode.overlayHeight;
		}

		if (_videoMode.fullscreen == _oldVideoMode.fullscreen &&
			_videoMode.aspectRatioCorrection == _oldVideoMode.aspectRatioCorrection &&
			_videoMode.mode == _oldVideoMode.mode &&
#if SDL_VERSION_ATLEAST(2, 0, 0)
			_videoMode.filtering == _oldVideoMode.filtering &&
#endif
			_videoMode.screenWidth == _oldVideoMode.screenWidth &&
			_videoMode.screenHeight == _oldVideoMode.screenHeight) {

			// Our new video mode would now be exactly the same as the
			// old one. Since we still can not assume SDL_SetVideoMode
			// to be working fine, we need to invalidate the old video
			// mode, so loadGFXMode would error out properly.
			_oldVideoMode.setup = false;
		}
	}

#ifdef USE_RGB_COLOR
	if (_transactionDetails.sizeChanged || _transactionDetails.formatChanged) {
#else
	if (_transactionDetails.sizeChanged) {
#endif
		unloadGFXMode();
		if (!loadGFXMode()) {
			if (_oldVideoMode.setup) {
				_transactionMode = kTransactionRollback;
				errors |= endGFXTransaction();
			}
		} else {
			setGraphicsModeIntern();
			clearOverlay();

			_videoMode.setup = true;
			// OSystem_SDL::pollEvent used to update the screen change count,
			// but actually it gives problems when a video mode was changed
			// but OSystem_SDL::pollEvent was not called. This for example
			// caused a crash under certain circumstances when doing an RTL.
			// To fix this issue we update the screen change count right here.
			_screenChangeCount++;
		}
	} else if (_transactionDetails.needHotswap) {
		setGraphicsModeIntern();
		if (!hotswapGFXMode()) {
			if (_oldVideoMode.setup) {
				_transactionMode = kTransactionRollback;
				errors |= endGFXTransaction();
			}
		} else {
			_videoMode.setup = true;
			// OSystem_SDL::pollEvent used to update the screen change count,
			// but actually it gives problems when a video mode was changed
			// but OSystem_SDL::pollEvent was not called. This for example
			// caused a crash under certain circumstances when doing an RTL.
			// To fix this issue we update the screen change count right here.
			_screenChangeCount++;

			if (_transactionDetails.needUpdatescreen)
				internUpdateScreen();
		}
#if SDL_VERSION_ATLEAST(2, 0, 0)
	} else if (_transactionDetails.needTextureUpdate) {
		setGraphicsModeIntern();
		recreateScreenTexture();
		internUpdateScreen();
#endif
	} else if (_transactionDetails.needUpdatescreen) {
		setGraphicsModeIntern();
		internUpdateScreen();
	}

	_transactionMode = kTransactionNone;
	return (OSystem::TransactionError)errors;
}

#ifdef USE_RGB_COLOR
Common::List<Graphics::PixelFormat> SurfaceSdlGraphicsManager::getSupportedFormats() const {
	assert(!_supportedFormats.empty());
	return _supportedFormats;
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
static void maskToBitCount(Uint32 mask, uint8 &numBits, uint8 &shift) {
	numBits = 0;
	shift = 32;
	for (int i = 0; i < 32; ++i) {
		if (mask & 1) {
			if (i < shift) {
				shift = i;
			}
			++numBits;
		}

		mask >>= 1;
	}
}
#endif

void SurfaceSdlGraphicsManager::detectSupportedFormats() {
	_supportedFormats.clear();

	Graphics::PixelFormat format = Graphics::PixelFormat::createFormatCLUT8();

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{
		SDL_Window *window = _window->getSDLWindow();
		if (window == nullptr) {
			error("Could not find ScummVM window for retrieving default display mode");
		}

		const int displayIndex = SDL_GetWindowDisplayIndex(window);
		if (displayIndex < 0) {
			error("Could not find ScummVM window display index");
		}

		SDL_DisplayMode defaultMode;
		if (SDL_GetDesktopDisplayMode(displayIndex, &defaultMode) != 0) {
			error("Could not get default system display mode");
		}

		int bpp;
		Uint32 rMask, gMask, bMask, aMask;
		if (SDL_PixelFormatEnumToMasks(defaultMode.format, &bpp, &rMask, &gMask, &bMask, &aMask) != SDL_TRUE) {
			error("Could not convert system pixel format %s to masks", SDL_GetPixelFormatName(defaultMode.format));
		}

		const uint8 bytesPerPixel = SDL_BYTESPERPIXEL(defaultMode.format);
		uint8 rBits, rShift, gBits, gShift, bBits, bShift, aBits, aShift;
		maskToBitCount(rMask, rBits, rShift);
		maskToBitCount(gMask, gBits, gShift);
		maskToBitCount(bMask, bBits, bShift);
		maskToBitCount(aMask, aBits, aShift);

		format = Graphics::PixelFormat(bytesPerPixel, rBits, gBits, bBits, aBits, rShift, gShift, bShift, aShift);

		_supportedFormats.push_back(format);
	}
#endif

	// Some tables with standard formats that we always list
	// as "supported". If frontend code tries to use one of
	// these, we will perform the necessary format
	// conversion in the background. Of course this incurs a
	// performance hit, but on desktop ports this should not
	// matter. We still push the currently active format to
	// the front, so if frontend code just uses the first
	// available format, it will get one that is "cheap" to
	// use.
	const Graphics::PixelFormat RGBList[] = {

		// RGBA8888, ARGB8888, RGB888
		Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0),
		Graphics::PixelFormat(4, 8, 8, 8, 8, 16, 8, 0, 24),
		Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0),

		// RGB565, XRGB1555, RGB555, RGBA4444, ARGB4444
		Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0),
		Graphics::PixelFormat(2, 5, 5, 5, 1, 10, 5, 0, 15),
		Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0),
		Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0),
		Graphics::PixelFormat(2, 4, 4, 4, 4, 8, 4, 0, 12)
	};
	const Graphics::PixelFormat BGRList[] = {

		// ABGR8888, BGRA8888, BGR888
		Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24),
		Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0),
		Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0),

		// BGR565, XBGR1555, BGR555, ABGR4444, BGRA4444
		Graphics::PixelFormat(2, 5, 6, 5, 0, 0, 5, 11, 0),
		Graphics::PixelFormat(2, 5, 5, 5, 1, 0, 5, 10, 15),
		Graphics::PixelFormat(2, 5, 5, 5, 0, 0, 5, 10, 0),
		Graphics::PixelFormat(2, 4, 4, 4, 4, 0, 4, 8, 12),
		Graphics::PixelFormat(2, 4, 4, 4, 4, 4, 8, 12, 0)
	};

	if (_hwScreen) {
		// Get our currently set hardware format
		Graphics::PixelFormat hwFormat(_hwScreen->format->BytesPerPixel,
			8 - _hwScreen->format->Rloss, 8 - _hwScreen->format->Gloss,
			8 - _hwScreen->format->Bloss, 8 - _hwScreen->format->Aloss,
			_hwScreen->format->Rshift, _hwScreen->format->Gshift,
			_hwScreen->format->Bshift, _hwScreen->format->Ashift);

		// Workaround to SDL not providing an accurate Aloss value on Mac OS X.
		if (_hwScreen->format->Amask == 0)
			hwFormat.aLoss = 8;

		_supportedFormats.push_back(hwFormat);

#if !SDL_VERSION_ATLEAST(2, 0, 0)
		format = hwFormat;
#endif
	}

	// TODO: prioritize matching alpha masks
	int i;

	// Push some RGB formats
	for (i = 0; i < ARRAYSIZE(RGBList); i++) {
		if (_hwScreen && (RGBList[i].bytesPerPixel > format.bytesPerPixel))
			continue;
		if (RGBList[i] != format)
			_supportedFormats.push_back(RGBList[i]);
	}

	// Push some BGR formats
	for (i = 0; i < ARRAYSIZE(BGRList); i++) {
		if (_hwScreen && (BGRList[i].bytesPerPixel > format.bytesPerPixel))
			continue;
		if (BGRList[i] != format)
			_supportedFormats.push_back(BGRList[i]);
	}

	// Finally, we always supposed 8 bit palette graphics
	_supportedFormats.push_back(Graphics::PixelFormat::createFormatCLUT8());
}
#endif

int SurfaceSdlGraphicsManager::getGraphicsModeScale(int mode) const {
	int scale;

	switch (mode) {
	case GFX_NORMAL:
		scale = 1;
		break;
#ifdef USE_SCALERS
	case GFX_DOUBLESIZE:
	case GFX_2XSAI:
	case GFX_SUPER2XSAI:
	case GFX_SUPEREAGLE:
	case GFX_ADVMAME2X:
	case GFX_TV2X:	
	case GFX_DOTMATRIX:	
#ifdef USE_HQ_SCALERS
	case GFX_HQ2X:

#endif	
	case GFX_SCAN2X020:
	case GFX_SCAN2X040:	
	case GFX_SCAN2X060:	
	case GFX_SCAN2X080:	
	case GFX_SCAN2X100:	
	case GFX_RGB2X:	
		scale = 2;
		break;
			
	case GFX_ADVMAME3X:		
	case GFX_TRIPLESIZE:
	case GFX_TV3X:	
#ifdef USE_HQ_SCALERS
	case GFX_HQ3X:
#endif		
	case GFX_SCAN3X020:
	case GFX_SCAN3X040:
	case GFX_SCAN3X060:
	case GFX_SCAN3X080:
	case GFX_SCAN3X100:	
	case GFX_RGB3X:	
		scale = 3;
		break;
// --------------------------------------- New Scaler Begin
	case GFX_ADVMAME4X:	
	case GFX_QUADSIZE:
	case GFX_TV4X:	
	case GFX_SCAN4X020:
	case GFX_SCAN4X040:
	case GFX_SCAN4X060:
	case GFX_SCAN4X080:
	case GFX_SCAN4X100:	
	case GFX_RGB4X:	
		scale = 4;
		break;
	case GFX_PENTASIZE:
	case GFX_TV5X:	
	case GFX_SCAN5X020:
	case GFX_SCAN5X040:
	case GFX_SCAN5X060:
	case GFX_SCAN5X080:
	case GFX_SCAN5X100:	
		scale = 5;
		break;
	case GFX_ADVMAME6X:		
	case GFX_HEXASIZE:
	case GFX_TV6X:	
	case GFX_SCAN6X020:
	case GFX_SCAN6X040:
	case GFX_SCAN6X060:
	case GFX_SCAN6X080:
	case GFX_SCAN6X100:	
		scale = 6;
		break;
#endif		

	default:
		warning("unknown gfx mode %d. Set Default", mode);
		scale = -1;
	}

	return scale;
}

bool SurfaceSdlGraphicsManager::setGraphicsMode(int mode) {
	Common::StackLock lock(_graphicsMutex);

	assert(_transactionMode == kTransactionActive);

	if (_oldVideoMode.setup && _oldVideoMode.mode == mode)
		return true;

	int newScaleFactor = getGraphicsModeScale(mode);

	if (newScaleFactor == -1) {
		warning("unknown gfx mode %d", mode);
		return false;
	}

	if (_oldVideoMode.setup && _oldVideoMode.scaleFactor != newScaleFactor)
		_transactionDetails.needHotswap = true;

	_transactionDetails.needUpdatescreen = true;

	_videoMode.mode = mode;
	_videoMode.scaleFactor = newScaleFactor;

	return true;
}

void SurfaceSdlGraphicsManager::setGraphicsModeIntern() {
	Common::StackLock lock(_graphicsMutex);
	ScalerProc *newScalerProc = 0;

	updateShader();

	switch (_videoMode.mode) {
	case GFX_NORMAL:
		newScalerProc = Normal1x;
		break;
#ifdef USE_SCALERS
	case GFX_DOUBLESIZE:
		newScalerProc = Normal2x;
		break;
	case GFX_TRIPLESIZE:
		newScalerProc = Normal3x;
		break;

	case GFX_2XSAI:
		newScalerProc = _2xSaI;
		break;
	case GFX_SUPER2XSAI:
		newScalerProc = Super2xSaI;
		break;
	case GFX_SUPEREAGLE:
		newScalerProc = SuperEagle;
		break;
	case GFX_ADVMAME2X:
		newScalerProc = AdvMame2x;
		break;
	case GFX_ADVMAME3X:
		newScalerProc = AdvMame3x;
		break;
#ifdef USE_HQ_SCALERS
	case GFX_HQ2X:
		newScalerProc = HQ2x;
		break;
	case GFX_HQ3X:
		newScalerProc = HQ3x;
		break;
#endif
	case GFX_TV2X:
		newScalerProc = TV2x;
		break;
	case GFX_TV3X:
		newScalerProc = TV3x;
		break;
	case GFX_DOTMATRIX:
		newScalerProc = DotMatrix;
		break;
// --------------------------------------- New Scaler Begin		
	case GFX_TV4X:
		newScalerProc = TV4x;
		break;
	case GFX_TV5X:
		newScalerProc = TV5x;
		break;	
	case GFX_TV6X:
		newScalerProc = TV6x;
		break;		
	case GFX_SCAN2X020:
		pScanlines = 20;
		newScalerProc = Scanlines2X;
		break;
	case GFX_SCAN2X040:
		pScanlines = 40;
		newScalerProc = Scanlines2X;
		break;
	case GFX_SCAN2X060:
		pScanlines = 60;
		newScalerProc = Scanlines2X;
		break;
	case GFX_SCAN2X080:
		pScanlines = 80;
		newScalerProc = Scanlines2X;
		break;
	case GFX_SCAN2X100:
		pScanlines = 100;
		newScalerProc = Scanlines2X;
		break;
	case GFX_SCAN3X020:
		pScanlines = 20;
		newScalerProc = Scanlines3X;
		break;
	case GFX_SCAN3X040:
		pScanlines = 40;
		newScalerProc = Scanlines3X;
		break;
	case GFX_SCAN3X060:
		pScanlines = 60;
		newScalerProc = Scanlines3X;
		break;
	case GFX_SCAN3X080:
		pScanlines = 80;
		newScalerProc = Scanlines3X;
		break;
	case GFX_SCAN3X100:
		pScanlines = 100;
		newScalerProc = Scanlines3X;
		break;
	case GFX_SCAN4X020:
		pScanlines = 20;
		newScalerProc = Scanlines4X;
		break;
	case GFX_SCAN4X040:
		pScanlines = 40;
		newScalerProc = Scanlines4X;
		break;
	case GFX_SCAN4X060:
		pScanlines = 60;
		newScalerProc = Scanlines4X;
		break;
	case GFX_SCAN4X080:
		pScanlines = 80;
		newScalerProc = Scanlines4X;
		break;
	case GFX_SCAN4X100:
		pScanlines = 100;
		newScalerProc = Scanlines4X;
		break;
	case GFX_SCAN5X020:
		pScanlines = 20;
		newScalerProc = Scanlines5X;
		break;
	case GFX_SCAN5X040:
		pScanlines = 40;
		newScalerProc = Scanlines5X;
		break;
	case GFX_SCAN5X060:
		pScanlines = 60;
		newScalerProc = Scanlines5X;
		break;
	case GFX_SCAN5X080:
		pScanlines = 80;
		newScalerProc = Scanlines5X;
		break;
	case GFX_SCAN5X100:
		pScanlines = 100;
		newScalerProc = Scanlines5X;
		break;			
	case GFX_SCAN6X020:
		pScanlines = 20;
		newScalerProc = Scanlines6X;
		break;
	case GFX_SCAN6X040:
		pScanlines = 40;
		newScalerProc = Scanlines6X;
		break;
	case GFX_SCAN6X060:
		pScanlines = 60;
		newScalerProc = Scanlines6X;
		break;
	case GFX_SCAN6X080:
		pScanlines = 80;
		newScalerProc = Scanlines6X;
		break;
	case GFX_SCAN6X100:
		pScanlines = 100;
		newScalerProc = Scanlines6X;
		break;			
	case GFX_RGB2X:
		newScalerProc = rgb_2x;
		break;
	case GFX_RGB4X:
		newScalerProc = rgb_4x;
		break;		
	case GFX_RGB3X:
		newScalerProc = rgb_3x;
		break;		
	case GFX_QUADSIZE:
		newScalerProc = Normal4x;
		break;
	case GFX_PENTASIZE:
		newScalerProc = Normal5x;
		break;
	case GFX_HEXASIZE:
		newScalerProc = Normal6x;
		break;
	case GFX_ADVMAME4X:
		newScalerProc = AdvMame4x;
		break;
	case GFX_ADVMAME6X:
		newScalerProc = AdvMame6x;
		break;				
// --------------------------------------- New Scaler ENd		
#endif // USE_SCALERS

	default:
		error("Unknown gfx mode %d", _videoMode.mode);
	}

	_scalerProc = newScalerProc;

	if (_videoMode.mode != GFX_NORMAL) {
		for (int i = 0; i < ARRAYSIZE(s_gfxModeSwitchTable); i++) {
			if (s_gfxModeSwitchTable[i][1] == _videoMode.mode || s_gfxModeSwitchTable[i][2] == _videoMode.mode) {
				_scalerType = i;
				break;
			}
		}
	}

	if (!_screen || !_hwScreen)
		return;

	// Blit everything to the screen
	_forceRedraw = true;

	// Even if the old and new scale factors are the same, we may have a
	// different scaler for the cursor now.
	blitCursor();
}

int SurfaceSdlGraphicsManager::getGraphicsMode() const {
	assert(_transactionMode == kTransactionNone);
	return _videoMode.mode;
}

const OSystem::GraphicsMode *SurfaceSdlGraphicsManager::getSupportedShaders() const {
	return s_supportedShaders;
}

int SurfaceSdlGraphicsManager::getShader() const {
	return _currentShader;
}

bool SurfaceSdlGraphicsManager::setShader(int id) {
	assert(id >= 0 && id < _numShaders);
	_currentShader = id;
	updateShader();
	return true;
}

void SurfaceSdlGraphicsManager::initSize(uint w, uint h, const Graphics::PixelFormat *format) {
	assert(_transactionMode == kTransactionActive);

	_newShakePos = 0;
	
#ifdef USE_RGB_COLOR
	//avoid redundant format changes
	Graphics::PixelFormat newFormat;
	if (!format)
		newFormat = Graphics::PixelFormat::createFormatCLUT8();
	else
		newFormat = *format;

	assert(newFormat.bytesPerPixel > 0);

	if (newFormat != _videoMode.format) {
		_videoMode.format = newFormat;
		_transactionDetails.formatChanged = true;
		_screenFormat = newFormat;
	}
#endif

#if !SDL_VERSION_ATLEAST(2, 0, 0)
	// Avoid redundant res changes, only in SDL1. In SDL2, redundancies may not
	// actually be redundant if ScummVM is switching between game engines and
	// the screen dimensions are being reinitialized, since window resizing is
	// supposed to reset when this happens
	if ((int)w == _videoMode.screenWidth && (int)h == _videoMode.screenHeight)
		return;
#endif

	if ((int)w != _videoMode.screenWidth || (int)h != _videoMode.screenHeight) {
		const bool useDefault = defaultGraphicsModeConfig();
		if (useDefault && w > 320) {
			resetGraphicsScale();
		} else {
			setGraphicsMode(getGraphicsModeIdByName(ConfMan.get("gfx_mode")));
		}
	}

	_videoMode.screenWidth = w;
	_videoMode.screenHeight = h;

	_transactionDetails.sizeChanged = true;
}


static void fixupResolutionForAspectRatio(AspectRatio desiredAspectRatio, int &width, int &height) {
	assert(&width != &height);

	if (desiredAspectRatio.isAuto())
		return;

	int kw = desiredAspectRatio.kw();
	int kh = desiredAspectRatio.kh();

	const int w = width;
	const int h = height;

	int bestW = 0, bestH = 0;
	uint bestMetric = (uint)-1; // Metric is wasted space

#if SDL_VERSION_ATLEAST(2, 0, 0)
	const int numModes = SDL_GetNumDisplayModes(0);
	SDL_DisplayMode modeData, *mode = &modeData;
	for (int i = 0; i < numModes; ++i) {
		if (SDL_GetDisplayMode(0, i, &modeData)) {
			continue;
		}
#else
	SDL_Rect const* const*availableModes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_SWSURFACE); //TODO : Maybe specify a pixel format
	assert(availableModes);

	while (const SDL_Rect *mode = *availableModes++) {
#endif
		if (mode->w < w)
			continue;
		if (mode->h < h)
			continue;
		if (mode->h * kw != mode->w * kh)
			continue;

		uint metric = mode->w * mode->h - w * h;
		if (metric > bestMetric)
			continue;

		bestMetric = metric;
		bestW = mode->w;
		bestH = mode->h;

	// Make editors a bit more happy by having the same amount of closing as
	// opening curley braces.
#if SDL_VERSION_ATLEAST(2, 0, 0)
	}
#else
	}
#endif

	if (!bestW || !bestH) {
		warning("Unable to enforce the desired aspect ratio");
		return;
	}
	width = bestW;
	height = bestH;
}

bool SurfaceSdlGraphicsManager::loadGFXMode() {
	_forceRedraw = true;

#if !defined(__MAEMO__) && !defined(DINGUX) && !defined(GPH_DEVICE) && !defined(LINUXMOTO)
	_videoMode.overlayWidth = _videoMode.screenWidth * _videoMode.scaleFactor;
	_videoMode.overlayHeight = _videoMode.screenHeight * _videoMode.scaleFactor;

	if (_videoMode.screenHeight != 200 && _videoMode.screenHeight != 400)
		_videoMode.aspectRatioCorrection = false;




	_videoMode.hardwareWidth = _videoMode.screenWidth * _videoMode.scaleFactor;
	_videoMode.hardwareHeight = _videoMode.screenHeight * _videoMode.scaleFactor;

	if (_videoMode.aspectRatioCorrection) {
		_videoMode.overlayHeight = real2Aspect(_videoMode.overlayHeight);
		_videoMode.hardwareHeight = real2Aspect(_videoMode.hardwareHeight);
	}

// On GPH devices ALL the _videoMode.hardware... are setup in GPHGraphicsManager::loadGFXMode()
#elif !defined(GPH_DEVICE)
	_videoMode.hardwareWidth = _videoMode.overlayWidth;
	_videoMode.hardwareHeight = _videoMode.overlayHeight;
#endif

	//
	// Create the surface that contains the 8 bit game data
	//

	const Graphics::PixelFormat &format = _screenFormat;
	const Uint32 rMask = ((0xFF >> format.rLoss) << format.rShift);
	const Uint32 gMask = ((0xFF >> format.gLoss) << format.gShift);
	const Uint32 bMask = ((0xFF >> format.bLoss) << format.bShift);
	const Uint32 aMask = ((0xFF >> format.aLoss) << format.aShift);
	_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, _videoMode.screenWidth, _videoMode.screenHeight,
						_screenFormat.bytesPerPixel * 8, rMask, gMask, bMask, aMask);




	if (_screen == NULL)
		error("allocating _screen failed");

#ifdef USE_RGB_COLOR
	// Avoid having SDL_SRCALPHA set even if we supplied an alpha-channel in the format.
	SDL_SetAlpha(_screen, 0, 255);

#endif

	// SDL 1.2 palettes default to all black,
	// SDL 1.3 palettes default to all white,
	// Thus set our own default palette to all black.
	// SDL_SetColors does nothing for non indexed surfaces.
	SDL_SetColors(_screen, _currentPalette, 0, 256);

	//
	// Create the surface that contains the scaled graphics in 16 bit mode
	//

	if (_videoMode.fullscreen) {
		fixupResolutionForAspectRatio(_videoMode.desiredAspectRatio, _videoMode.hardwareWidth, _videoMode.hardwareHeight);
	}


#ifdef ENABLE_EVENTRECORDER
	_displayDisabled = ConfMan.getBool("disable_display");

	if (_displayDisabled) {
		_hwScreen = g_eventRec.getSurface(_videoMode.hardwareWidth, _videoMode.hardwareHeight);
	} else
#endif
	{
#if defined(WIN32) && !SDL_VERSION_ATLEAST(2, 0, 0)
		// Save the original bpp to be able to restore the video mode on
		// unload. See _originalBitsPerPixel documentation.
		if (_originalBitsPerPixel == 0) {
			const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
			_originalBitsPerPixel = videoInfo->vfmt->BitsPerPixel;
		}
#endif

		_hwScreen = SDL_SetVideoMode(_videoMode.hardwareWidth, _videoMode.hardwareHeight, 16,
			_videoMode.fullscreen ? (SDL_FULLSCREEN|SDL_SWSURFACE) : SDL_SWSURFACE
			);
	}

#ifdef USE_RGB_COLOR
	detectSupportedFormats();
#endif

	if (_hwScreen == NULL) {
		// DON'T use error(), as this tries to bring up the debug
		// console, which WON'T WORK now that _hwScreen is hosed.

		if (!_oldVideoMode.setup) {
			warning("SDL_SetVideoMode says we can't switch to that mode (%s)", SDL_GetError());
			g_system->quit();
		} else {
			return false;
		}
	}

#if !SDL_VERSION_ATLEAST(2, 0, 0)
	handleResize(_videoMode.hardwareWidth, _videoMode.hardwareHeight);
#endif
	//
	// Create the surface used for the graphics in 16 bit before scaling, and also the overlay
	//

	// Need some extra bytes around when using 2xSaI
	_tmpscreen = SDL_CreateRGBSurface(SDL_SWSURFACE,
						_videoMode.screenWidth + 2 * MAX_SCALER_REACH,
						_videoMode.screenHeight + 2 * MAX_SCALER_REACH,
						16,
						_hwScreen->format->Rmask,
						_hwScreen->format->Gmask,
						_hwScreen->format->Bmask,
						_hwScreen->format->Amask);

	if (_tmpscreen == NULL)
		error("allocating _tmpscreen failed");

	_overlayscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, _videoMode.overlayWidth, _videoMode.overlayHeight,
						16,
						_hwScreen->format->Rmask,
						_hwScreen->format->Gmask,
						_hwScreen->format->Bmask,
						_hwScreen->format->Amask);

	if (_overlayscreen == NULL)
		error("allocating _overlayscreen failed");

	_overlayFormat.bytesPerPixel = _overlayscreen->format->BytesPerPixel;

	_overlayFormat.rLoss = _overlayscreen->format->Rloss;
	_overlayFormat.gLoss = _overlayscreen->format->Gloss;
	_overlayFormat.bLoss = _overlayscreen->format->Bloss;
	_overlayFormat.aLoss = _overlayscreen->format->Aloss;

	_overlayFormat.rShift = _overlayscreen->format->Rshift;
	_overlayFormat.gShift = _overlayscreen->format->Gshift;
	_overlayFormat.bShift = _overlayscreen->format->Bshift;
	_overlayFormat.aShift = _overlayscreen->format->Ashift;

	_tmpscreen2 = SDL_CreateRGBSurface(SDL_SWSURFACE,
						_videoMode.overlayWidth + 2 * MAX_SCALER_REACH,
						_videoMode.overlayHeight + 2 * MAX_SCALER_REACH,
						16,
						_hwScreen->format->Rmask,
						_hwScreen->format->Gmask,
						_hwScreen->format->Bmask,
						_hwScreen->format->Amask);

	if (_tmpscreen2 == NULL)
		error("allocating _tmpscreen2 failed");



	// Distinguish 555 and 565 mode
	if (_hwScreen->format->Rmask == 0x7C00)
		InitScalers(555);
	else
		InitScalers(565);

	return true;
}

void SurfaceSdlGraphicsManager::unloadGFXMode() {
	if (_screen) {
		SDL_FreeSurface(_screen);
		_screen = NULL;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	deinitializeRenderer();
#endif

	if (_hwScreen) {
		SDL_FreeSurface(_hwScreen);
		_hwScreen = NULL;
	}

	if (_tmpscreen) {
		SDL_FreeSurface(_tmpscreen);
		_tmpscreen = NULL;
	}

	if (_tmpscreen2) {
		SDL_FreeSurface(_tmpscreen2);
		_tmpscreen2 = NULL;
	}

	if (_overlayscreen) {
		SDL_FreeSurface(_overlayscreen);
		_overlayscreen = NULL;
	}

#ifdef USE_OSD
	if (_osdMessageSurface) {
		SDL_FreeSurface(_osdMessageSurface);
		_osdMessageSurface = NULL;
	}

	if (_osdIconSurface) {
		SDL_FreeSurface(_osdIconSurface);
		_osdIconSurface = NULL;
	}
#endif
	DestroyScalers();

#if defined(WIN32) && !SDL_VERSION_ATLEAST(2, 0, 0)
	// Reset video mode to original.
	// This will ensure that any new graphic manager will use the initial BPP
	// when listing available modes. See _originalBitsPerPixel documentation.
	if (_originalBitsPerPixel != 0)
		SDL_SetVideoMode(_videoMode.screenWidth, _videoMode.screenHeight, _originalBitsPerPixel, _videoMode.fullscreen ? (SDL_FULLSCREEN | SDL_SWSURFACE) : SDL_SWSURFACE);
#endif
}

bool SurfaceSdlGraphicsManager::hotswapGFXMode() {
	if (!_screen)
		return false;

	// Keep around the old _screen & _overlayscreen so we can restore the screen data
	// after the mode switch.
	SDL_Surface *old_screen = _screen;
	_screen = NULL;
	SDL_Surface *old_overlayscreen = _overlayscreen;
	_overlayscreen = NULL;

	// Release the HW screen surface
	if (_hwScreen) {
		SDL_FreeSurface(_hwScreen);
		_hwScreen = NULL;
	}
	if (_tmpscreen) {
		SDL_FreeSurface(_tmpscreen);
		_tmpscreen = NULL;
	}
	if (_tmpscreen2) {
		SDL_FreeSurface(_tmpscreen2);
		_tmpscreen2 = NULL;
	}

	// Setup the new GFX mode
	if (!loadGFXMode()) {
		unloadGFXMode();

		_screen = old_screen;
		_overlayscreen = old_overlayscreen;

		return false;
	}

	// reset palette
	SDL_SetColors(_screen, _currentPalette, 0, 256);

	// Restore old screen content
	SDL_BlitSurface(old_screen, NULL, _screen, NULL);
	SDL_BlitSurface(old_overlayscreen, NULL, _overlayscreen, NULL);

	// Free the old surfaces
	SDL_FreeSurface(old_screen);
	SDL_FreeSurface(old_overlayscreen);

	// Update cursor to new scale
	blitCursor();

	// Blit everything to the screen
	internUpdateScreen();

	return true;
}

void SurfaceSdlGraphicsManager::updateScreen() {
	assert(_transactionMode == kTransactionNone);

	Common::StackLock lock(_graphicsMutex);	// Lock the mutex until this function ends

	internUpdateScreen();
}

void SurfaceSdlGraphicsManager::updateShader() {
	// shader init code goes here
	// currently only used on Vita port
	// the user-selected shaderID should be obtained via ConfMan.getInt("shader")
	// and the corresponding shader should then be activated here
	// this way the user can combine any software scaling (scalers)
	// with any hardware shading (shaders). The shaders could provide
	// scanline masks, overlays, but could also serve for
	// hardware-based up-scaling (sharp-bilinear-simple, etc.)
}

void SurfaceSdlGraphicsManager::internUpdateScreen() {
	SDL_Surface *srcSurf, *origSurf;
	int height, width;
	ScalerProc *scalerProc;
	int scale1;



	// If the shake position changed, fill the dirty area with blackness
	if (_currentShakePos != _newShakePos ||
		(_cursorNeedsRedraw && _mouseBackup.y <= _currentShakePos)) {
		SDL_Rect blackrect = {0, 0, (Uint16)(_videoMode.screenWidth * _videoMode.scaleFactor), (Uint16)(_newShakePos * _videoMode.scaleFactor)};

		if (_videoMode.aspectRatioCorrection && !_overlayVisible)
			blackrect.h = real2Aspect(blackrect.h - 1) + 1;

		SDL_FillRect(_hwScreen, &blackrect, 0);

		_currentShakePos = _newShakePos;

		_forceRedraw = true;
	}

	// Check whether the palette was changed in the meantime and update the
	// screen surface accordingly.
	if (_screen && _paletteDirtyEnd != 0) {
		SDL_SetColors(_screen, _currentPalette + _paletteDirtyStart,
			_paletteDirtyStart,
			_paletteDirtyEnd - _paletteDirtyStart);

		_paletteDirtyEnd = 0;

		_forceRedraw = true;
	}

	if (!_overlayVisible) {
		origSurf = _screen;
		srcSurf = _tmpscreen;
		width = _videoMode.screenWidth;
		height = _videoMode.screenHeight;
		scalerProc = _scalerProc;
		scale1 = _videoMode.scaleFactor;
	} else {
		origSurf = _overlayscreen;
		srcSurf = _tmpscreen2;
		width = _videoMode.overlayWidth;
		height = _videoMode.overlayHeight;
		scalerProc = Normal1x;

		scale1 = 1;
	}

	// Add the area covered by the mouse cursor to the list of dirty rects if
	// we have to redraw the mouse, or if the cursor is alpha-blended since
	// alpha-blended cursors will happily blend into themselves if the surface
	// under the cursor is not reset first
	if (_cursorNeedsRedraw || _cursorFormat.bytesPerPixel == 4)
		undrawMouse();

#ifdef USE_OSD
	updateOSD();
#endif

	// Force a full redraw if requested
	if (_forceRedraw) {
		_numDirtyRects = 1;
		_dirtyRectList[0].x = 0;
		_dirtyRectList[0].y = 0;
		_dirtyRectList[0].w = width;
		_dirtyRectList[0].h = height;
	}

	// Only draw anything if necessary
	if (_numDirtyRects > 0 || _cursorNeedsRedraw) {
		SDL_Rect *r;
		SDL_Rect dst;
		uint32 srcPitch, dstPitch;
		SDL_Rect *lastRect = _dirtyRectList + _numDirtyRects;

		for (r = _dirtyRectList; r != lastRect; ++r) {
			dst = *r;
 			dst.x += MAX_SCALER_REACH;	// Shift rect by one since 2xSai needs to access the data around
 			dst.y += MAX_SCALER_REACH;	// any pixel to scale it, and we want to avoid mem access crashes.

			if (SDL_BlitSurface(origSurf, r, srcSurf, &dst) != 0)
				error("SDL_BlitSurface failed: %s", SDL_GetError());
		}

		SDL_LockSurface(srcSurf);
		SDL_LockSurface(_hwScreen);

		srcPitch = srcSurf->pitch;
		dstPitch = _hwScreen->pitch;

		for (r = _dirtyRectList; r != lastRect; ++r) {
			int dst_y = r->y + _currentShakePos;
			int dst_h = 0;
#ifdef USE_SCALERS
			int orig_dst_y = 0;
#endif
			int rx1 = r->x * scale1;

			if (dst_y < height) {
				dst_h = r->h;
				if (dst_h > height - dst_y)
					dst_h = height - dst_y;

#ifdef USE_SCALERS
				orig_dst_y = dst_y;
#endif
				dst_y = dst_y * scale1;

				if (_videoMode.aspectRatioCorrection && !_overlayVisible)
					dst_y = real2Aspect(dst_y);

				assert(scalerProc != NULL);
				scalerProc((byte *)srcSurf->pixels + (r->x * 2 + 2 * MAX_SCALER_REACH) + (r->y + MAX_SCALER_REACH) * srcPitch, srcPitch,
					(byte *)_hwScreen->pixels + rx1 * 2 + dst_y * dstPitch, dstPitch, r->w, dst_h);
			}

			r->x = rx1;
			r->y = dst_y;
			r->w = r->w * scale1;
			r->h = dst_h * scale1;

#ifdef USE_SCALERS
			if (_videoMode.aspectRatioCorrection && orig_dst_y < height && !_overlayVisible)
				r->h = stretch200To240((uint8 *) _hwScreen->pixels, dstPitch, r->w, r->h, r->x, r->y, orig_dst_y * scale1);
#endif
		}
		SDL_UnlockSurface(srcSurf);
		SDL_UnlockSurface(_hwScreen);

		// Readjust the dirty rect list in case we are doing a full update.
		// This is necessary if shaking is active.
		if (_forceRedraw) {
			_dirtyRectList[0].y = 0;
			_dirtyRectList[0].h = _videoMode.hardwareHeight;
		}

		drawMouse();

#ifdef USE_OSD
		drawOSD();
#endif

#ifdef USE_SDL_DEBUG_FOCUSRECT
		// We draw the focus rectangle on top of everything, to assure it's easily visible.
		// Of course when the overlay is visible we do not show it, since it is only for game
		// specific focus.
		if (_enableFocusRect && !_overlayVisible) {
			int y = _focusRect.top + _currentShakePos;
			int h = 0;
			int x = _focusRect.left * scale1;
			int w = _focusRect.width() * scale1;

			if (y < height) {
				h = _focusRect.height();
				if (h > height - y)
					h = height - y;

				y *= scale1;

				if (_videoMode.aspectRatioCorrection && !_overlayVisible)
					y = real2Aspect(y);

				if (h > 0 && w > 0) {
					SDL_LockSurface(_hwScreen);

					// Use white as color for now.
					Uint32 rectColor = SDL_MapRGB(_hwScreen->format, 0xFF, 0xFF, 0xFF);

					// First draw the top and bottom lines
					// then draw the left and right lines
					if (_hwScreen->format->BytesPerPixel == 2) {
						uint16 *top = (uint16 *)((byte *)_hwScreen->pixels + y * _hwScreen->pitch + x * 2);
						uint16 *bottom = (uint16 *)((byte *)_hwScreen->pixels + (y + h) * _hwScreen->pitch + x * 2);
						byte *left = ((byte *)_hwScreen->pixels + y * _hwScreen->pitch + x * 2);
						byte *right = ((byte *)_hwScreen->pixels + y * _hwScreen->pitch + (x + w - 1) * 2);

						while (w--) {
							*top++ = rectColor;
							*bottom++ = rectColor;
						}

						while (h--) {
							*(uint16 *)left = rectColor;
							*(uint16 *)right = rectColor;

							left += _hwScreen->pitch;
							right += _hwScreen->pitch;
						}
					} else if (_hwScreen->format->BytesPerPixel == 4) {
						uint32 *top = (uint32 *)((byte *)_hwScreen->pixels + y * _hwScreen->pitch + x * 4);
						uint32 *bottom = (uint32 *)((byte *)_hwScreen->pixels + (y + h) * _hwScreen->pitch + x * 4);
						byte *left = ((byte *)_hwScreen->pixels + y * _hwScreen->pitch + x * 4);
						byte *right = ((byte *)_hwScreen->pixels + y * _hwScreen->pitch + (x + w - 1) * 4);

						while (w--) {
							*top++ = rectColor;
							*bottom++ = rectColor;
						}

						while (h--) {
							*(uint32 *)left = rectColor;
							*(uint32 *)right = rectColor;

							left += _hwScreen->pitch;
							right += _hwScreen->pitch;
						}
					}

					SDL_UnlockSurface(_hwScreen);
				}
			}
		}
#endif

		// Finally, blit all our changes to the screen
		if (!_displayDisabled) {
			SDL_UpdateRects(_hwScreen, _numDirtyRects, _dirtyRectList);
		}
	}

	_numDirtyRects = 0;
	_forceRedraw = false;
	_cursorNeedsRedraw = false;
}

bool SurfaceSdlGraphicsManager::saveScreenshot(const char *filename) {
	assert(_hwScreen != NULL);

	Common::StackLock lock(_graphicsMutex);
#ifdef USE_PNG
	Common::DumpFile out;
	if (!out.open(filename)) {
		return false;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Surface *rgbScreen = SDL_ConvertSurfaceFormat(_hwScreen, SDL_PIXELFORMAT_RGB24, 0);
#else
	// This block of code was taken mostly as-is from SDL 1.2's SDL_SaveBMP_RW
	SDL_Surface *rgbScreen = SDL_CreateRGBSurface(SDL_SWSURFACE,
												  _hwScreen->w,
												  _hwScreen->h,
												  24,
#ifdef SCUMM_LITTLE_ENDIAN
												  0x0000FF, 0x00FF00, 0xFF0000,
#else
												  0xFF0000, 0x00FF00, 0x0000FF,
#endif
												  0);
	if (rgbScreen == nullptr) {
		warning("Could not create RGB24 surface");
		return false;
	}

	SDL_Rect bounds;
	bounds.x = bounds.y = 0;
	bounds.w = _hwScreen->w;
	bounds.h = _hwScreen->h;
	if (SDL_LowerBlit(_hwScreen, &bounds, rgbScreen, &bounds) < 0) {
		SDL_FreeSurface(rgbScreen);
		rgbScreen = nullptr;
	}
#endif

	if (rgbScreen == nullptr) {
		warning("Could not convert hardware surface to RGB24");
		return false;
	}

	int result = SDL_LockSurface(rgbScreen);
	if (result < 0) {
		warning("Could not lock RGB surface");
		SDL_FreeSurface(rgbScreen);
		return false;
	}

	const Graphics::PixelFormat format(3, 8, 8, 8, 0, 16, 8, 0, 0);
	Graphics::Surface data;
	data.init(rgbScreen->w, rgbScreen->h, rgbScreen->pitch, rgbScreen->pixels, format);
	const bool success = Image::writePNG(out, data);

	SDL_UnlockSurface(rgbScreen);
	SDL_FreeSurface(rgbScreen);

	return success;
#else
	return SDL_SaveBMP(_hwScreen, filename) == 0;
#endif
}

void SurfaceSdlGraphicsManager::setFullscreenMode(bool enable) {
	Common::StackLock lock(_graphicsMutex);

	if (_oldVideoMode.setup && _oldVideoMode.fullscreen == enable)
		return;

	if (_transactionMode == kTransactionActive) {
		_videoMode.fullscreen = enable;
		_transactionDetails.needHotswap = true;
	}
}

void SurfaceSdlGraphicsManager::setAspectRatioCorrection(bool enable) {
	Common::StackLock lock(_graphicsMutex);

	if (_oldVideoMode.setup && _oldVideoMode.aspectRatioCorrection == enable)
		return;

	if (_transactionMode == kTransactionActive) {
		_videoMode.aspectRatioCorrection = enable;
		_transactionDetails.needHotswap = true;
	}
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
void SurfaceSdlGraphicsManager::setFilteringMode(bool enable) {
	Common::StackLock lock(_graphicsMutex);

	if (_oldVideoMode.setup && _oldVideoMode.filtering == enable)
		return;

	if (_transactionMode == kTransactionActive) {
		_videoMode.filtering = enable;
		_transactionDetails.needTextureUpdate = true;
	}
}
#endif

void SurfaceSdlGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	assert(_transactionMode == kTransactionNone);
	assert(buf);

	if (_screen == NULL) {
		warning("SurfaceSdlGraphicsManager::copyRectToScreen: _screen == NULL");
		return;
	}

	Common::StackLock lock(_graphicsMutex);	// Lock the mutex until this function ends

	assert(x >= 0 && x < _videoMode.screenWidth);
	assert(y >= 0 && y < _videoMode.screenHeight);
	assert(h > 0 && y + h <= _videoMode.screenHeight);
	assert(w > 0 && x + w <= _videoMode.screenWidth);

	addDirtyRect(x, y, w, h);

	// Try to lock the screen surface
	if (SDL_LockSurface(_screen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());


	byte *dst = (byte *)_screen->pixels + y * _screen->pitch + x * _screenFormat.bytesPerPixel;
	if (_videoMode.screenWidth == w && pitch == _screen->pitch) {
		memcpy(dst, buf, h*pitch);
	} else {
		const byte *src = (const byte *)buf;
		do {
			memcpy(dst, src, w * _screenFormat.bytesPerPixel);
			src += pitch;
			dst += _screen->pitch;
		} while (--h);
	}


	// Unlock the screen surface
	SDL_UnlockSurface(_screen);
}

Graphics::Surface *SurfaceSdlGraphicsManager::lockScreen() {
	assert(_transactionMode == kTransactionNone);

	// Lock the graphics mutex
	g_system->lockMutex(_graphicsMutex);

	// paranoia check
	assert(!_screenIsLocked);
	_screenIsLocked = true;

	// Try to lock the screen surface
	if (SDL_LockSurface(_screen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());

	_framebuffer.init(_screen->w, _screen->h, _screen->pitch, _screen->pixels, _screenFormat);


	return &_framebuffer;
}

void SurfaceSdlGraphicsManager::unlockScreen() {
	assert(_transactionMode == kTransactionNone);

	// paranoia check
	assert(_screenIsLocked);
	_screenIsLocked = false;

	// Unlock the screen surface
	SDL_UnlockSurface(_screen);

	// Trigger a full screen update
	_forceRedraw = true;

	// Finally unlock the graphics mutex
	g_system->unlockMutex(_graphicsMutex);
}

void SurfaceSdlGraphicsManager::fillScreen(uint32 col) {
	Graphics::Surface *screen = lockScreen();
	if (screen && screen->getPixels())
		memset(screen->getPixels(), col, screen->h * screen->pitch);
	unlockScreen();
}

void SurfaceSdlGraphicsManager::addDirtyRect(int x, int y, int w, int h, bool realCoordinates) {
	if (_forceRedraw)
		return;

	if (_numDirtyRects == NUM_DIRTY_RECT) {
		_forceRedraw = true;
		return;
	}

	int height, width;

	if (!_overlayVisible && !realCoordinates) {
		width = _videoMode.screenWidth;
		height = _videoMode.screenHeight;
	} else {
		width = _videoMode.overlayWidth;
		height = _videoMode.overlayHeight;
	}

	// Extend the dirty region by 1 pixel for scalers
	// that "smear" the screen, e.g. 2xSAI
	if (!realCoordinates) {
		x--;
		y--;
		w += 2;
		h += 2;
	}

	// clip
	if (x < 0) {
		w += x;
		x = 0;
	}

	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w > width - x) {
		w = width - x;
	}

	if (h > height - y) {
		h = height - y;
	}

#ifdef USE_SCALERS
	if (_videoMode.aspectRatioCorrection && !_overlayVisible && !realCoordinates) {
		makeRectStretchable(x, y, w, h);
	}
#endif

	if (w == width && h == height) {
		_forceRedraw = true;
		return;
	}

	if (w > 0 && h > 0) {
		SDL_Rect *r = &_dirtyRectList[_numDirtyRects++];

		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
	}
}

int16 SurfaceSdlGraphicsManager::getHeight() const {
	return _videoMode.screenHeight;
}

int16 SurfaceSdlGraphicsManager::getWidth() const {
	return _videoMode.screenWidth;
}

void SurfaceSdlGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	assert(colors);


	assert(_screenFormat.bytesPerPixel == 1);


	// Setting the palette before _screen is created is allowed - for now -
	// since we don't actually set the palette until the screen is updated.
	// But it could indicate a programming error, so let's warn about it.

	if (!_screen)
		warning("SurfaceSdlGraphicsManager::setPalette: _screen == NULL");

	const byte *b = colors;
	uint i;
	SDL_Color *base = _currentPalette + start;
	for (i = 0; i < num; i++, b += 3) {
		base[i].r = b[0];
		base[i].g = b[1];
		base[i].b = b[2];
#if SDL_VERSION_ATLEAST(2, 0, 0)
		base[i].a = 255;
#endif
	}

	if (start < _paletteDirtyStart)
		_paletteDirtyStart = start;

	if (start + num > _paletteDirtyEnd)
		_paletteDirtyEnd = start + num;

	// Some games blink cursors with palette
	if (_cursorPaletteDisabled)
		blitCursor();
}

void SurfaceSdlGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	assert(colors);


	assert(_screenFormat.bytesPerPixel == 1);


	const SDL_Color *base = _currentPalette + start;

	for (uint i = 0; i < num; ++i) {
		colors[i * 3] = base[i].r;
		colors[i * 3 + 1] = base[i].g;
		colors[i * 3 + 2] = base[i].b;
	}
}

void SurfaceSdlGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	assert(colors);
	const byte *b = colors;
	uint i;
	SDL_Color *base = _cursorPalette + start;
	for (i = 0; i < num; i++, b += 3) {
		base[i].r = b[0];
		base[i].g = b[1];
		base[i].b = b[2];
#if SDL_VERSION_ATLEAST(2, 0, 0)
		base[i].a = 255;
#endif
	}

	_cursorPaletteDisabled = false;
	blitCursor();
}

void SurfaceSdlGraphicsManager::setShakePos(int shake_pos) {
	assert(_transactionMode == kTransactionNone);

	_newShakePos = shake_pos;
}

void SurfaceSdlGraphicsManager::setFocusRectangle(const Common::Rect &rect) {
#ifdef USE_SDL_DEBUG_FOCUSRECT
	// Only enable focus rectangle debug code, when the user wants it
	if (!_enableFocusRectDebugCode)
		return;

	_enableFocusRect = true;
	_focusRect = rect;

	if (rect.left < 0 || rect.top < 0 || rect.right > _videoMode.screenWidth || rect.bottom > _videoMode.screenHeight)
		warning("SurfaceSdlGraphicsManager::setFocusRectangle: Got a rect which does not fit inside the screen bounds: %d,%d,%d,%d", rect.left, rect.top, rect.right, rect.bottom);

	// It's gross but we actually sometimes get rects, which are not inside the screen bounds,
	// thus we need to clip the rect here...
	_focusRect.clip(_videoMode.screenWidth, _videoMode.screenHeight);

	// We just fake this as a dirty rect for now, to easily force an screen update whenever
	// the rect changes.
	addDirtyRect(_focusRect.left, _focusRect.top, _focusRect.width(), _focusRect.height());
#endif
}

void SurfaceSdlGraphicsManager::clearFocusRectangle() {
#ifdef USE_SDL_DEBUG_FOCUSRECT
	// Only enable focus rectangle debug code, when the user wants it
	if (!_enableFocusRectDebugCode)
		return;

	_enableFocusRect = false;

	// We just fake this as a dirty rect for now, to easily force an screen update whenever
	// the rect changes.
	addDirtyRect(_focusRect.left, _focusRect.top, _focusRect.width(), _focusRect.height());
#endif
}

#pragma mark -
#pragma mark --- Overlays ---
#pragma mark -

void SurfaceSdlGraphicsManager::clearOverlay() {
	Common::StackLock lock(_graphicsMutex);	// Lock the mutex until this function ends
	if (!_overlayVisible)
		return;
	// Clear the overlay by making the game screen "look through" everywhere.
	SDL_Rect src, dst;
	src.x = src.y = 0;
	dst.x = dst.y = 1;
	src.w = dst.w = _videoMode.screenWidth;
	src.h = dst.h = _videoMode.screenHeight;
	if (SDL_BlitSurface(_screen, &src, _tmpscreen, &dst) != 0)
		error("SDL_BlitSurface failed: %s", SDL_GetError());

	SDL_LockSurface(_tmpscreen);
	SDL_LockSurface(_overlayscreen);
	_scalerProc((byte *)(_tmpscreen->pixels) + _tmpscreen->pitch + 2, _tmpscreen->pitch,
	(byte *)_overlayscreen->pixels, _overlayscreen->pitch, _videoMode.screenWidth, _videoMode.screenHeight);

#ifdef USE_SCALERS
	if (_videoMode.aspectRatioCorrection)
		stretch200To240((uint8 *)_overlayscreen->pixels, _overlayscreen->pitch,
						_videoMode.overlayWidth, _videoMode.screenHeight * _videoMode.scaleFactor, 0, 0, 0);
#endif
	SDL_UnlockSurface(_tmpscreen);
	SDL_UnlockSurface(_overlayscreen);

	_forceRedraw = true;
}

void SurfaceSdlGraphicsManager::grabOverlay(void *buf, int pitch) const {
	assert(_transactionMode == kTransactionNone);

	if (_overlayscreen == NULL)
		return;

	if (SDL_LockSurface(_overlayscreen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());

	byte *src = (byte *)_overlayscreen->pixels;
	byte *dst = (byte *)buf;
	int h = _videoMode.overlayHeight;
	do {
		memcpy(dst, src, _videoMode.overlayWidth * 2);
		src += _overlayscreen->pitch;
		dst += pitch;
	} while (--h);

	SDL_UnlockSurface(_overlayscreen);
}

void SurfaceSdlGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	assert(_transactionMode == kTransactionNone);

	if (_overlayscreen == NULL)
		return;

	const byte *src = (const byte *)buf;

	// Clip the coordinates
	if (x < 0) {
		w += x;
		src -= x * 2;
		x = 0;
	}

	if (y < 0) {
		h += y;
		src -= y * pitch;
		y = 0;
	}

	if (w > _videoMode.overlayWidth - x) {
		w = _videoMode.overlayWidth - x;
	}

	if (h > _videoMode.overlayHeight - y) {
		h = _videoMode.overlayHeight - y;
	}

	if (w <= 0 || h <= 0)
		return;

	// Mark the modified region as dirty
	addDirtyRect(x, y, w, h);

	if (SDL_LockSurface(_overlayscreen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());

	byte *dst = (byte *)_overlayscreen->pixels + y * _overlayscreen->pitch + x * 2;
	do {
		memcpy(dst, src, w * 2);
		dst += _overlayscreen->pitch;
		src += pitch;
	} while (--h);

	SDL_UnlockSurface(_overlayscreen);
}


#pragma mark -
#pragma mark --- Mouse ---
#pragma mark -

void SurfaceSdlGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keyColor, bool dontScale, const Graphics::PixelFormat *format) {
	bool formatChanged = false;
	
	if (format) {
#ifndef USE_RGB_COLOR
		assert(format->bytesPerPixel == 1);
#endif
		if (format->bytesPerPixel != _cursorFormat.bytesPerPixel) {
			formatChanged = true;
		}
		_cursorFormat = *format;
	} else {
		if (_cursorFormat.bytesPerPixel != 1) {
			formatChanged = true;
		}		
		_cursorFormat = Graphics::PixelFormat::createFormatCLUT8();
	}

	if (_cursorFormat.bytesPerPixel < 4) {
		assert(keyColor < 1U << (_cursorFormat.bytesPerPixel * 8));
	}

	_mouseCurState.hotX = hotspotX;
	_mouseCurState.hotY = hotspotY;
	_mouseKeyColor = keyColor;

	_cursorDontScale = dontScale;

	if (_mouseCurState.w != (int)w || _mouseCurState.h != (int)h || formatChanged) {
		_mouseCurState.w = w;
		_mouseCurState.h = h;

		if (_mouseOrigSurface) {
			SDL_FreeSurface(_mouseOrigSurface);

			if (_mouseSurface == _mouseOrigSurface) {
				_mouseSurface = nullptr;
			}

			_mouseOrigSurface = nullptr;
		}

		if ((formatChanged || _cursorFormat.bytesPerPixel == 4) && _mouseSurface) {
			SDL_FreeSurface(_mouseSurface);
			_mouseSurface = nullptr;
		}

		if (!w || !h) {
			return;
		}

		if (_cursorFormat.bytesPerPixel == 4) {
			assert(!_mouseSurface);
			assert(!_mouseOrigSurface);

			
			const Uint32 rMask = ((0xFF >> format->rLoss) << format->rShift);
			const Uint32 gMask = ((0xFF >> format->gLoss) << format->gShift);
			const Uint32 bMask = ((0xFF >> format->bLoss) << format->bShift);
			const Uint32 aMask = ((0xFF >> format->aLoss) << format->aShift);
			_mouseSurface = _mouseOrigSurface = SDL_CreateRGBSurfaceFrom(const_cast<void *>(buf), w, h, format->bytesPerPixel * 8, w * format->bytesPerPixel, rMask, gMask, bMask, aMask);
		} else {
			assert(!_mouseOrigSurface);
			
		// Allocate bigger surface because AdvMame2x adds black pixel at [0,0]
		_mouseOrigSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_RLEACCEL | SDL_SRCCOLORKEY | SDL_SRCALPHA,
						_mouseCurState.w + 2 * MAX_SCALER_REACH,
						_mouseCurState.h + 2 * MAX_SCALER_REACH,
						16,
						_hwScreen->format->Rmask,
						_hwScreen->format->Gmask,
						_hwScreen->format->Bmask,
						_hwScreen->format->Amask);
		}

		if (_mouseOrigSurface == nullptr) {
			error("Allocating _mouseOrigSurface failed");
		}

		if (_cursorFormat.bytesPerPixel == 4) {
			SDL_SetColorKey(_mouseOrigSurface, SDL_SRCCOLORKEY | SDL_SRCALPHA, _mouseKeyColor);
		} else {
			SDL_SetColorKey(_mouseOrigSurface, SDL_RLEACCEL | SDL_SRCCOLORKEY | SDL_SRCALPHA, kMouseColorKey);
		}
	}

	delete[] _mouseData;
	if (_cursorFormat.bytesPerPixel == 4) {
		_mouseData = nullptr;
	} else {
		_mouseData = new byte[w * h * _cursorFormat.bytesPerPixel];
		assert(_mouseData);
		memcpy(_mouseData, buf, w * h * _cursorFormat.bytesPerPixel);
	}




	blitCursor();
}

void SurfaceSdlGraphicsManager::blitCursor() {
	const int w = _mouseCurState.w;
	const int h = _mouseCurState.h;

	if (!w || !h || !_mouseOrigSurface) {
		return;
	}

	if (_cursorFormat.bytesPerPixel != 4 && !_mouseData) {
		return;
	}

	_cursorNeedsRedraw = true;

	int cursorScale;
	if (_cursorDontScale) {
		// Don't scale the cursor at all if the user requests this behavior.
		cursorScale = 1;
	} else {
		// Scale the cursor with the game screen scale factor.
		cursorScale = _videoMode.scaleFactor;
	}



	// Adapt the real hotspot according to the scale factor.
	int rW = w * cursorScale;
	int rH = h * cursorScale;
	_mouseCurState.rHotX = _mouseCurState.hotX * cursorScale;
	_mouseCurState.rHotY = _mouseCurState.hotY * cursorScale;

	// The virtual dimensions will be the same as the original.

	_mouseCurState.vW = w;
	_mouseCurState.vH = h;
	_mouseCurState.vHotX = _mouseCurState.hotX;
	_mouseCurState.vHotY = _mouseCurState.hotY;

#ifdef USE_SCALERS
	// store original to pass to aspect-correction function later
	const int rH1 = rH;
#endif

	if (!_cursorDontScale && _videoMode.aspectRatioCorrection) {
		rH = real2Aspect(rH - 1) + 1;
		_mouseCurState.rHotY = real2Aspect(_mouseCurState.rHotY);
	}

	bool sizeChanged = false;
	if (_mouseCurState.rW != rW || _mouseCurState.rH != rH) {
		_mouseCurState.rW = rW;
		_mouseCurState.rH = rH;
		sizeChanged = true;
	}

		if (_cursorFormat.bytesPerPixel == 4) {








		if (_mouseSurface != _mouseOrigSurface) {
			SDL_FreeSurface(_mouseSurface);
		}


		if (cursorScale == 1) {
			_mouseSurface = _mouseOrigSurface;
			return;
		}

		SDL_PixelFormat *format = _mouseOrigSurface->format;
		_mouseSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCCOLORKEY | SDL_SRCALPHA,
											 rW, rH,
											 format->BitsPerPixel,
											 format->Rmask,
											 format->Gmask,
											 format->Bmask,
											 format->Amask);

		SDL_SetColorKey(_mouseSurface, SDL_SRCCOLORKEY | SDL_SRCALPHA, _mouseKeyColor);
											 
		// At least SDL 2.0.4 on Windows apparently has a broken SDL_BlitScaled
		// implementation, and SDL 1 has no such API at all, and our other
		// scalers operate exclusively at 16bpp, so here is a scrappy 32bpp
		// point scaler
		SDL_LockSurface(_mouseOrigSurface);
		SDL_LockSurface(_mouseSurface);

		const byte *src = (const byte *)_mouseOrigSurface->pixels;
		byte *dst = (byte *)_mouseSurface->pixels;
		for (int y = 0; y < _mouseOrigSurface->h; ++y) {
			uint32 *rowDst = (uint32 *)dst;

			const uint32 *rowSrc = (const uint32 *)src;
			for (int x = 0; x < _mouseOrigSurface->w; ++x) {
				for (int scaleX = 0; scaleX < cursorScale; ++scaleX) {
					*rowDst++ = *rowSrc;
				}
				++rowSrc;
			}
			for (int scaleY = 0; scaleY < cursorScale - 1; ++scaleY) {
				memcpy(dst + _mouseSurface->pitch, dst, _mouseSurface->pitch);
				dst += _mouseSurface->pitch;
			}
			dst += _mouseSurface->pitch;
			src += _mouseOrigSurface->pitch;
		}

		SDL_UnlockSurface(_mouseSurface);
		SDL_UnlockSurface(_mouseOrigSurface);

		return;
	}

	SDL_LockSurface(_mouseOrigSurface);

	byte *dstPtr;
	const byte *srcPtr = _mouseData;

	uint32 color;


	// Make whole surface transparent
	for (int i = 0; i < h + 2 * MAX_SCALER_REACH; i++) {
		dstPtr = (byte *)_mouseOrigSurface->pixels + _mouseOrigSurface->pitch * i;
		for (int j = 0; j < w + 2 * MAX_SCALER_REACH; j++) {
			*(uint16 *)dstPtr = kMouseColorKey;
			dstPtr += 2;
		}
	}

	// Draw from [1,1] since AdvMame2x adds artefact at 0,0
	dstPtr = (byte *)_mouseOrigSurface->pixels + (_mouseOrigSurface->pitch + 2) * MAX_SCALER_REACH;

	SDL_Color *palette;

	if (_cursorPaletteDisabled)
		palette = _currentPalette;
	else
		palette = _cursorPalette;

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			if (_cursorFormat.bytesPerPixel == 2) {


				color = *(const uint16 *)srcPtr;
				if (color != _mouseKeyColor) {


					uint8 r, g, b;
					_cursorFormat.colorToRGB(color, r, g, b);
					*(uint16 *)dstPtr = SDL_MapRGB(_mouseOrigSurface->format, r, g, b);

				}
				dstPtr += 2;
				srcPtr += _cursorFormat.bytesPerPixel;
			} else {

				color = *srcPtr;
				if (color != _mouseKeyColor) {
					*(uint16 *)dstPtr = SDL_MapRGB(_mouseOrigSurface->format,
						palette[color].r, palette[color].g, palette[color].b);
				}
				dstPtr += 2;
				srcPtr++;
			}


		}
		dstPtr += _mouseOrigSurface->pitch - w * 2;
	}

	if (sizeChanged || !_mouseSurface) {

		if (_mouseSurface)
			SDL_FreeSurface(_mouseSurface);

		_mouseSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_RLEACCEL | SDL_SRCCOLORKEY | SDL_SRCALPHA,
						_mouseCurState.rW,
						_mouseCurState.rH,
						16,
						_hwScreen->format->Rmask,
						_hwScreen->format->Gmask,
						_hwScreen->format->Bmask,
						_hwScreen->format->Amask);

		if (_mouseSurface == nullptr)
			error("allocating _mouseSurface failed");

		SDL_SetColorKey(_mouseSurface, SDL_RLEACCEL | SDL_SRCCOLORKEY | SDL_SRCALPHA, kMouseColorKey);
	}

	SDL_LockSurface(_mouseSurface);

	ScalerProc *scalerProc;

	// Only apply scaling, when the user allows it.
	if (!_cursorDontScale) {
		// If possible, use the same scaler for the cursor as for the rest of
		// the game. This only works well with the non-blurring scalers so we
		// actually only use the 1x, 2x and AdvMame scalers.
		if (_videoMode.mode == GFX_DOUBLESIZE || _videoMode.mode == GFX_TRIPLESIZE || _videoMode.mode == GFX_QUADSIZE || _videoMode.mode == GFX_PENTASIZE || _videoMode.mode == GFX_HEXASIZE)
			scalerProc = _scalerProc;
		else
			scalerProc = scalersMagn[_videoMode.scaleFactor - 1];
	} else {
		scalerProc = Normal1x;
	}

	scalerProc((byte *)_mouseOrigSurface->pixels + (_mouseOrigSurface->pitch + 2) * MAX_SCALER_REACH,
		_mouseOrigSurface->pitch, (byte *)_mouseSurface->pixels, _mouseSurface->pitch,
		_mouseCurState.w, _mouseCurState.h);

#ifdef USE_SCALERS
	if (!_cursorDontScale && _videoMode.aspectRatioCorrection)
		cursorStretch200To240((uint8 *)_mouseSurface->pixels, _mouseSurface->pitch, rW, rH1, 0, 0, 0);
#endif

	SDL_UnlockSurface(_mouseSurface);
	SDL_UnlockSurface(_mouseOrigSurface);
}

#ifdef USE_SCALERS
// Basically it is kVeryFastAndUglyAspectMode of stretch200To240 from
// common/scale/aspect.cpp
static int cursorStretch200To240(uint8 *buf, uint32 pitch, int width, int height, int srcX, int srcY, int origSrcY) {
	int maxDstY = real2Aspect(origSrcY + height - 1);
	int y;
	const uint8 *startSrcPtr = buf + srcX * 2 + (srcY - origSrcY) * pitch;
	uint8 *dstPtr = buf + srcX * 2 + maxDstY * pitch;

	for (y = maxDstY; y >= srcY; y--) {
		const uint8 *srcPtr = startSrcPtr + aspect2Real(y) * pitch;

		if (srcPtr == dstPtr)
			break;
		memcpy(dstPtr, srcPtr, width * 2);
		dstPtr -= pitch;
	}

	return 1 + maxDstY - srcY;
}
#endif

void SurfaceSdlGraphicsManager::undrawMouse() {
	const int x = _mouseBackup.x;
	const int y = _mouseBackup.y;

	// When we switch bigger overlay off mouse jumps. Argh!
	// This is intended to prevent undrawing offscreen mouse
	if (!_overlayVisible && (x >= _videoMode.screenWidth || y >= _videoMode.screenHeight))
		return;

	if (_mouseBackup.w != 0 && _mouseBackup.h != 0)
		addDirtyRect(x, y, _mouseBackup.w, _mouseBackup.h);
}

void SurfaceSdlGraphicsManager::drawMouse() {
	if (!_cursorVisible || !_mouseSurface || !_mouseCurState.w || !_mouseCurState.h) {
		_mouseBackup.x = _mouseBackup.y = _mouseBackup.w = _mouseBackup.h = 0;
		return;
	}

	SDL_Rect dst;
	int scale;
	int hotX, hotY;

	const Common::Point virtualCursor = convertWindowToVirtual(_cursorX, _cursorY);

	dst.x = virtualCursor.x;
	dst.y = virtualCursor.y;

	if (!_overlayVisible) {
		scale = _videoMode.scaleFactor;
		dst.w = _mouseCurState.vW;
		dst.h = _mouseCurState.vH;
		hotX = _mouseCurState.vHotX;
		hotY = _mouseCurState.vHotY;
	} else {
		scale = 1;
		dst.w = _mouseCurState.rW;
		dst.h = _mouseCurState.rH;
		hotX = _mouseCurState.rHotX;
		hotY = _mouseCurState.rHotY;
	}

	// The mouse is undrawn using virtual coordinates, i.e. they may be
	// scaled and aspect-ratio corrected.

	_mouseBackup.x = dst.x - hotX;
	_mouseBackup.y = dst.y - hotY;
	_mouseBackup.w = dst.w;
	_mouseBackup.h = dst.h;

	// We draw the pre-scaled cursor image, so now we need to adjust for
	// scaling, shake position and aspect ratio correction manually.

	dst.y += _currentShakePos;

	if (_videoMode.aspectRatioCorrection && !_overlayVisible)
		dst.y = real2Aspect(dst.y);

	dst.x = scale * dst.x - _mouseCurState.rHotX;
	dst.y = scale * dst.y - _mouseCurState.rHotY;
	dst.w = _mouseCurState.rW;
	dst.h = _mouseCurState.rH;



	// Note that SDL_BlitSurface() and addDirtyRect() will both perform any
	// clipping necessary

	if (SDL_BlitSurface(_mouseSurface, nullptr, _hwScreen, &dst) != 0)
		error("SDL_BlitSurface failed: %s", SDL_GetError());

	// The screen will be updated using real surface coordinates, i.e.
	// they will not be scaled or aspect-ratio corrected.

	addDirtyRect(dst.x, dst.y, dst.w, dst.h, true);
}

#pragma mark -
#pragma mark --- On Screen Display ---
#pragma mark -

#ifdef USE_OSD
void SurfaceSdlGraphicsManager::displayMessageOnOSD(const char *msg) {
	assert(_transactionMode == kTransactionNone);
	assert(msg);

	Common::StackLock lock(_graphicsMutex);	// Lock the mutex until this function ends

	removeOSDMessage();

	// The font we are going to use:
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kLocalizedFont);

	// Split the message into separate lines.
	Common::Array<Common::String> lines;
	const char *ptr;
	for (ptr = msg; *ptr; ++ptr) {
		if (*ptr == '\n') {
			lines.push_back(Common::String(msg, ptr - msg));
			msg = ptr + 1;
		}
	}
	lines.push_back(Common::String(msg, ptr - msg));

	// Determine a rect which would contain the message string (clipped to the
	// screen dimensions).
	const int vOffset = 6;
	const int lineSpacing = 1;
	const int lineHeight = font->getFontHeight() + 2 * lineSpacing;
	int width = 0;
	int height = lineHeight * lines.size() + 2 * vOffset;
	uint i;
	for (i = 0; i < lines.size(); i++) {
		width = MAX(width, font->getStringWidth(lines[i]) + 14);
	}

	// Clip the rect
	if (width > _hwScreen->w)
		width = _hwScreen->w;
	if (height > _hwScreen->h)
		height = _hwScreen->h;

	_osdMessageSurface = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_RLEACCEL | SDL_SRCALPHA,
		width, height, 16, _hwScreen->format->Rmask, _hwScreen->format->Gmask, _hwScreen->format->Bmask, _hwScreen->format->Amask
	);

	// Lock the surface
	if (SDL_LockSurface(_osdMessageSurface))
		error("displayMessageOnOSD: SDL_LockSurface failed: %s", SDL_GetError());

	// Draw a dark gray rect
	// TODO: Rounded corners ? Border?
	SDL_FillRect(_osdMessageSurface, nullptr, SDL_MapRGB(_osdMessageSurface->format, 64, 64, 64));

	Graphics::Surface dst;
	dst.init(_osdMessageSurface->w, _osdMessageSurface->h, _osdMessageSurface->pitch, _osdMessageSurface->pixels,
		Graphics::PixelFormat(_osdMessageSurface->format->BytesPerPixel,
			8 - _osdMessageSurface->format->Rloss, 8 - _osdMessageSurface->format->Gloss,
			8 - _osdMessageSurface->format->Bloss, 8 - _osdMessageSurface->format->Aloss,
			_osdMessageSurface->format->Rshift, _osdMessageSurface->format->Gshift,
			_osdMessageSurface->format->Bshift, _osdMessageSurface->format->Ashift));

	// Render the message, centered, and in white
	for (i = 0; i < lines.size(); i++) {
		font->drawString(&dst, lines[i],
			0, 0 + i * lineHeight + vOffset + lineSpacing, width,
			SDL_MapRGB(_osdMessageSurface->format, 255, 255, 255),
			Graphics::kTextAlignCenter);
	}

	// Finished drawing, so unlock the OSD message surface
	SDL_UnlockSurface(_osdMessageSurface);

	// Init the OSD display parameters, and the fade out
	_osdMessageAlpha = SDL_ALPHA_TRANSPARENT + kOSDInitialAlpha * (SDL_ALPHA_OPAQUE - SDL_ALPHA_TRANSPARENT) / 100;
	_osdMessageFadeStartTime = SDL_GetTicks() + kOSDFadeOutDelay;
	// Enable alpha blending
	SDL_SetAlpha(_osdMessageSurface, SDL_RLEACCEL | SDL_SRCALPHA, _osdMessageAlpha);

	// Ensure a full redraw takes place next time the screen is updated
	_forceRedraw = true;
}

SDL_Rect SurfaceSdlGraphicsManager::getOSDMessageRect() const {
	SDL_Rect rect;
	rect.x = (_hwScreen->w - _osdMessageSurface->w) / 2;
	rect.y = (_hwScreen->h - _osdMessageSurface->h) / 2;
	rect.w = _osdMessageSurface->w;
	rect.h = _osdMessageSurface->h;
	return rect;
}

void SurfaceSdlGraphicsManager::displayActivityIconOnOSD(const Graphics::Surface *icon) {
	assert(_transactionMode == kTransactionNone);

	Common::StackLock lock(_graphicsMutex);	// Lock the mutex until this function ends

	if (_osdIconSurface && !icon) {
		// Force a redraw to clear the icon on the next update
		_forceRedraw = true;
	}

	if (_osdIconSurface) {
		SDL_FreeSurface(_osdIconSurface);
		_osdIconSurface = nullptr;
	}

	if (icon) {
		const Graphics::PixelFormat &iconFormat = icon->format;

		_osdIconSurface = SDL_CreateRGBSurface(
				SDL_SWSURFACE | SDL_RLEACCEL | SDL_SRCALPHA,
				icon->w, icon->h, iconFormat.bytesPerPixel * 8,
				((0xFF >> iconFormat.rLoss) << iconFormat.rShift),
				((0xFF >> iconFormat.gLoss) << iconFormat.gShift),
				((0xFF >> iconFormat.bLoss) << iconFormat.bShift),
				((0xFF >> iconFormat.aLoss) << iconFormat.aShift)
		);

		// Lock the surface
		if (SDL_LockSurface(_osdIconSurface))
			error("displayActivityIconOnOSD: SDL_LockSurface failed: %s", SDL_GetError());

		byte *dst = (byte *) _osdIconSurface->pixels;
		const byte *src = (const byte *) icon->getPixels();
		for (uint y = 0; y < icon->h; y++) {
			memcpy(dst, src, icon->w * iconFormat.bytesPerPixel);
			src += icon->pitch;
			dst += _osdIconSurface->pitch;
		}

		// Finished drawing, so unlock the OSD icon surface
		SDL_UnlockSurface(_osdIconSurface);
	}
}

SDL_Rect SurfaceSdlGraphicsManager::getOSDIconRect() const {
	SDL_Rect dstRect;
	dstRect.x = _hwScreen->w - _osdIconSurface->w - 10;
	dstRect.y = 10;
	dstRect.w = _osdIconSurface->w;
	dstRect.h = _osdIconSurface->h;
	return dstRect;
}

void SurfaceSdlGraphicsManager::removeOSDMessage() {
	// Remove the previous message
	if (_osdMessageSurface) {
		SDL_FreeSurface(_osdMessageSurface);
		_forceRedraw = true;
	}

	_osdMessageSurface = NULL;
	_osdMessageAlpha = SDL_ALPHA_TRANSPARENT;
}

void SurfaceSdlGraphicsManager::updateOSD() {
	// OSD message visible (i.e. non-transparent)?
	if (_osdMessageAlpha != SDL_ALPHA_TRANSPARENT) {
		// Updated alpha value
		const int diff = SDL_GetTicks() - _osdMessageFadeStartTime;
		if (diff > 0) {
			if (diff >= kOSDFadeOutDuration) {
				// Back to full transparency
				_osdMessageAlpha = SDL_ALPHA_TRANSPARENT;
			} else {
				// Do a linear fade out...
				const int startAlpha = SDL_ALPHA_TRANSPARENT + kOSDInitialAlpha * (SDL_ALPHA_OPAQUE - SDL_ALPHA_TRANSPARENT) / 100;
				_osdMessageAlpha = startAlpha + diff * (SDL_ALPHA_TRANSPARENT - startAlpha) / kOSDFadeOutDuration;
			}
			SDL_SetAlpha(_osdMessageSurface, SDL_RLEACCEL | SDL_SRCALPHA, _osdMessageAlpha);
		}

		if (_osdMessageAlpha == SDL_ALPHA_TRANSPARENT) {
			removeOSDMessage();
		}
	}

	if (_osdIconSurface || _osdMessageSurface) {
		// Redraw the area below the icon and message for the transparent blit to give correct results.
		_forceRedraw = true;
	}
}

void SurfaceSdlGraphicsManager::drawOSD() {
	if (_osdMessageSurface) {
		SDL_Rect dstRect = getOSDMessageRect();
		SDL_BlitSurface(_osdMessageSurface, 0, _hwScreen, &dstRect);
	}

	if (_osdIconSurface) {
		SDL_Rect dstRect = getOSDIconRect();
		SDL_BlitSurface(_osdIconSurface, 0, _hwScreen, &dstRect);
	}
}

#endif

void SurfaceSdlGraphicsManager::handleResizeImpl(const int width, const int height) {
	SdlGraphicsManager::handleResizeImpl(width, height);
	recalculateDisplayAreas();
}
bool SurfaceSdlGraphicsManager::handleScalerHotkeys(Common::KeyCode key) {

	// Ctrl-Alt-a toggles aspect ratio correction
	if (key == 'a') {
		beginGFXTransaction();
			setFeatureState(OSystem::kFeatureAspectRatioCorrection, !_videoMode.aspectRatioCorrection);
		endGFXTransaction();
#ifdef USE_OSD
		Common::String message;
		if (_videoMode.aspectRatioCorrection)
			message = Common::String::format("%s\n%d x %d -> %d x %d",
				_("Enabled aspect ratio correction"),
				_videoMode.screenWidth, _videoMode.screenHeight,
				_hwScreen->w, _hwScreen->h
				);
		else
			message = Common::String::format("%s\n%d x %d -> %d x %d",
				_("Disabled aspect ratio correction"),
				_videoMode.screenWidth, _videoMode.screenHeight,
				_hwScreen->w, _hwScreen->h
				);
		displayMessageOnOSD(message.c_str());
#endif
		internUpdateScreen();
		return true;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	// Ctrl-Alt-f toggles filtering
	if (key == 'f') {
		beginGFXTransaction();
		setFeatureState(OSystem::kFeatureFilteringMode, !_videoMode.filtering);
		endGFXTransaction();
#ifdef USE_OSD
		if (getFeatureState(OSystem::kFeatureFilteringMode)) {
			displayMessageOnOSD(_("Filtering enabled"));
		} else {
			displayMessageOnOSD(_("Filtering disabled"));
		}
#endif
		_forceRedraw = true;
		internUpdateScreen();
		return true;
	}
#endif

	int newMode = -1;
	int factor = _videoMode.scaleFactor - 1;
	SDLKey sdlKey = (SDLKey)key;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	bool sizeChanged = false;
#endif

	// Increase/decrease the scale factor
	if (sdlKey == SDLK_EQUALS || sdlKey == SDLK_PLUS || sdlKey == SDLK_MINUS ||
		sdlKey == SDLK_KP_PLUS || sdlKey == SDLK_KP_MINUS) {
		factor += (sdlKey == SDLK_MINUS || sdlKey == SDLK_KP_MINUS) ? -1 : +1;
		if (0 <= factor && factor <= 7) {
			newMode = s_gfxModeSwitchTable[_scalerType][factor];
#if SDL_VERSION_ATLEAST(2, 0, 0)
			sizeChanged = true;
#endif
		}
	}

	const bool isNormalNumber = (SDLK_1 <= sdlKey && sdlKey <= SDLK_9);
	const bool isKeypadNumber = (SDLK_KP1 <= sdlKey && sdlKey <= SDLK_KP9);
	if (isNormalNumber || isKeypadNumber) {
		_scalerType = sdlKey - (isNormalNumber ? SDLK_1 : SDLK_KP1);
		if (_scalerType >= ARRAYSIZE(s_gfxModeSwitchTable))
			return false;

		while (s_gfxModeSwitchTable[_scalerType][factor] < 0) {
			assert(factor > 0);
			factor--;
		}
		newMode = s_gfxModeSwitchTable[_scalerType][factor];
	}

	if (newMode >= 0) {
		beginGFXTransaction();
			setGraphicsMode(newMode);
		endGFXTransaction();
#ifdef USE_OSD
		const char *newScalerName = 0;
		const OSystem::GraphicsMode *g = getSupportedGraphicsModes();
		while (g->name) {
			if (g->id == _videoMode.mode) {
				newScalerName = g->description;
				break;
			}
			g++;
		}
		if (newScalerName) {
			const Common::String message = Common::String::format(
				"%s %s\n%d x %d -> %d x %d",
				_("Active graphics filter:"),
				newScalerName,
				_videoMode.screenWidth, _videoMode.screenHeight,
				_hwScreen->w, _hwScreen->h);
			displayMessageOnOSD(message.c_str());
		}
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)

		if (sizeChanged) {
			// Forcibly resizing the window here since a user switching scaler
			// size will not normally cause the window to update
			_window->createOrUpdateWindow(_hwScreen->w, _hwScreen->h, _lastFlags);
		}
#endif

		internUpdateScreen();

		return true;
	} else {
		return false;
	}
}

bool SurfaceSdlGraphicsManager::isScalerHotkey(const Common::Event &event) {
	if ((event.kbd.flags & (Common::KBD_CTRL|Common::KBD_ALT)) == (Common::KBD_CTRL|Common::KBD_ALT)) {
		const bool isNormalNumber = (Common::KEYCODE_1 <= event.kbd.keycode && event.kbd.keycode <= Common::KEYCODE_9);
		const bool isKeypadNumber = (Common::KEYCODE_KP1 <= event.kbd.keycode && event.kbd.keycode <= Common::KEYCODE_KP9);
		const bool isScaleKey = (event.kbd.keycode == Common::KEYCODE_EQUALS || event.kbd.keycode == Common::KEYCODE_PLUS || event.kbd.keycode == Common::KEYCODE_MINUS ||
			event.kbd.keycode == Common::KEYCODE_KP_PLUS || event.kbd.keycode == Common::KEYCODE_KP_MINUS);

		if (isNormalNumber || isKeypadNumber) {
			int keyValue = event.kbd.keycode - (isNormalNumber ? Common::KEYCODE_1 : Common::KEYCODE_KP1);
			if (keyValue >= ARRAYSIZE(s_gfxModeSwitchTable))
				return false;
		}
#if SDL_VERSION_ATLEAST(2, 0, 0)
		if (event.kbd.keycode == 'f')
			return true;
#endif
		return (isScaleKey || event.kbd.keycode == 'a');
	}
	return false;
}

void SurfaceSdlGraphicsManager::toggleFullScreen() {
	beginGFXTransaction();
		setFullscreenMode(!_videoMode.fullscreen);
	endGFXTransaction();
#ifdef USE_OSD
	if (_videoMode.fullscreen)
		displayMessageOnOSD(_("Fullscreen mode"));
	else
		displayMessageOnOSD(_("Windowed mode"));
#endif
}

bool SurfaceSdlGraphicsManager::notifyEvent(const Common::Event &event) {
	switch ((int)event.type) {
	case Common::EVENT_KEYDOWN:
		// Alt-Return and Alt-Enter toggle full screen mode
		if (event.kbd.hasFlags(Common::KBD_ALT) &&
			(event.kbd.keycode == Common::KEYCODE_RETURN ||
			event.kbd.keycode == (Common::KeyCode)SDLK_KP_ENTER)) {
			toggleFullScreen();
			return true;
		}

		// Alt-S: Create a screenshot
		if (event.kbd.hasFlags(Common::KBD_ALT) && event.kbd.keycode == 's') {
			Common::String filename;

			Common::String screenshotsPath;
			OSystem_SDL *sdl_g_system = dynamic_cast<OSystem_SDL*>(g_system);
			if (sdl_g_system)
				screenshotsPath = sdl_g_system->getScreenshotsPath();

			for (int n = 0;; n++) {
				SDL_RWops *file;

#ifdef USE_PNG
				filename = Common::String::format("scummvm%05d.png", n);
#else
				filename = Common::String::format("scummvm%05d.bmp", n);
#endif

				file = SDL_RWFromFile((screenshotsPath + filename).c_str(), "r");

				if (!file)
					break;
				SDL_RWclose(file);
			}

			if (saveScreenshot((screenshotsPath + filename).c_str())) {
				if (screenshotsPath.empty())
					debug("Saved screenshot '%s' in current directory", filename.c_str());
				else
					debug("Saved screenshot '%s' in directory '%s'", filename.c_str(), screenshotsPath.c_str());
			} else {
				if (screenshotsPath.empty())
					warning("Could not save screenshot in current directory");
				else
					warning("Could not save screenshot in directory '%s'", screenshotsPath.c_str());
			}

			return true;
		}

		// Ctrl-Alt-<key> will change the GFX mode
		if (event.kbd.hasFlags(Common::KBD_CTRL|Common::KBD_ALT)) {
			if (handleScalerHotkeys(event.kbd.keycode))
				return true;
		}

		break;

	case Common::EVENT_KEYUP:
		return isScalerHotkey(event);

	default:
		break;
	}

	return false;
}

void SurfaceSdlGraphicsManager::notifyVideoExpose() {
	_forceRedraw = true;
}

void SurfaceSdlGraphicsManager::notifyResize(const int width, const int height) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	handleResize(width, height);
#endif

}

#if SDL_VERSION_ATLEAST(2, 0, 0)
void SurfaceSdlGraphicsManager::deinitializeRenderer() {
	if (_screenTexture)
		SDL_DestroyTexture(_screenTexture);
	_screenTexture = nullptr;

	if (_renderer)
		SDL_DestroyRenderer(_renderer);
	_renderer = nullptr;
}

void SurfaceSdlGraphicsManager::recreateScreenTexture() {
	if (!_renderer)
		return;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, _videoMode.filtering ? "linear" : "nearest");

	SDL_Texture *oldTexture = _screenTexture;
	_screenTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, _videoMode.hardwareWidth, _videoMode.hardwareHeight);
	if (_screenTexture)
		SDL_DestroyTexture(oldTexture);
	else
		_screenTexture = oldTexture;
}

SDL_Surface *SurfaceSdlGraphicsManager::SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags) {
	deinitializeRenderer();

	uint32 createWindowFlags = SDL_WINDOW_RESIZABLE;

	if ((flags & SDL_FULLSCREEN) != 0) {
		createWindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if (!createOrUpdateWindow(width, height, createWindowFlags)) {
		return nullptr;
	}

	_renderer = SDL_CreateRenderer(_window->getSDLWindow(), -1, 0);
	if (!_renderer) {
		deinitializeRenderer();
		return nullptr;
	}

	getWindowSizeFromSdl(&_windowWidth, &_windowHeight);
	handleResize(_windowWidth, _windowHeight);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, _videoMode.filtering ? "linear" : "nearest");

	_screenTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!_screenTexture) {
		deinitializeRenderer();
		return nullptr;
	}

	SDL_Surface *screen = SDL_CreateRGBSurface(0, width, height, 16, 0xF800, 0x7E0, 0x1F, 0);
	if (!screen) {
		deinitializeRenderer();
		return nullptr;
	} else {
		return screen;
	}
}

void SurfaceSdlGraphicsManager::SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects) {
	SDL_UpdateTexture(_screenTexture, nullptr, screen->pixels, screen->pitch);

	SDL_Rect viewport;
	viewport.x = _activeArea.drawRect.left;
	viewport.y = _activeArea.drawRect.top;
	viewport.w = _activeArea.drawRect.width();
	viewport.h = _activeArea.drawRect.height();

	SDL_RenderClear(_renderer);
	SDL_RenderCopy(_renderer, _screenTexture, NULL, &viewport);
	SDL_RenderPresent(_renderer);
}
#endif // SDL_VERSION_ATLEAST(2, 0, 0)

#endif
