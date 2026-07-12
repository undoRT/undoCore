/**
 * @file undoCoreVer.hpp
 * @brief Version information for undoCore
 * @author Salvatore Bamundo
 * @date June 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 undoRT
 */

#pragma once

#include <string>

// Version numbers following Semantic Versioning (semver.org)
#define UNDOCORE_VERSION_MAJOR  0
#define UNDOCORE_VERSION_MINOR  2
#define UNDOCORE_VERSION_PATCH  2
#define UNDOCORE_VERSION_PREREL ""

// Helper macros for stringification (workaround for MSVC)
#define UNDOCORE_STRINGIFY_IMPL(x) #x
#define UNDOCORE_STRINGIFY(x)      UNDOCORE_STRINGIFY_IMPL(x)

// Version string for display
#define UNDOCORE_VERSION_STRING \
   UNDOCORE_STRINGIFY(UNDOCORE_VERSION_MAJOR) \
   "." UNDOCORE_STRINGIFY(UNDOCORE_VERSION_MINOR) "." UNDOCORE_STRINGIFY(UNDOCORE_VERSION_PATCH) UNDOCORE_VERSION_PREREL

// Build date (automatically updated by compiler)
#define UNDOCORE_BUILD_DATE __DATE__ " " __TIME__

// Compiler information
#ifdef __clang__
#define UNDOCORE_COMPILER "Clang " __clang_version__
#elif defined(__GNUC__)
#define UNDOCORE_COMPILER "GCC " __VERSION__
#elif defined(_MSC_VER)
// For MSVC, _MSC_VER is a number, need to stringify it
#define UNDOCORE_COMPILER "MSVC " UNDOCORE_STRINGIFY(_MSC_VER)
#else
#define UNDOCORE_COMPILER "Unknown"
#endif

/**
 * @brief Get version as a string
 * @return Version string (e.g., "1.0.0")
 */
inline std::string getVersion()
{
   return UNDOCORE_VERSION_STRING;
}

/**
 * @brief Get full version information
 * @return Detailed version string with compiler and build date
 */
inline std::string getFullVersion()
{
   return std::string("undoCore version ") + UNDOCORE_VERSION_STRING + " (" + UNDOCORE_COMPILER + ", built " + UNDOCORE_BUILD_DATE + ")";
}

/**
 * @brief Get version numbers as tuple
 * @return Struct with major, minor, patch
 */
struct Version
{
   int major = UNDOCORE_VERSION_MAJOR;
   int minor = UNDOCORE_VERSION_MINOR;
   int patch = UNDOCORE_VERSION_PATCH;

   std::string toString() const { return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); }
};

inline Version getVersionNumbers()
{
   return Version{};
}