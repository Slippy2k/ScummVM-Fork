#ifndef INCLUDED_FROM_BASE_VERSION_CPP
#error This file may only be included by base/version.cpp
#endif

// Reads revision number from file
// (this is used when building with Visual Studio)
#ifdef SCUMMVM_INTERNAL_REVISION
#include "internal_revision.h"
#endif

#ifdef RELEASE_BUILD
#undef SCUMMVM_REVISION
#endif

#ifndef SCUMMVM_REVISION
#define SCUMMVM_REVISION
#endif

#define SCUMMVM_VERSION "2.1.0git 64bit" SCUMMVM_REVISION
