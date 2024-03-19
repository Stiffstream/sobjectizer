/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helper macros for detection of compiler/platform.
 *
 * \since v.5.8.2
 */

#pragma once

/*!
 * \def SO_5_OS_WIN32
 * \brief A marker for Win32 platform.
 *
 * \note This marker is also defined for Win64 platform.
 */

/*!
 * \def SO_5_OS_WIN64
 * \brief A marker for Win64 platform.
 */

/*!
 * \def SO_5_OS_WINDOWS
 * \brief A marker for Windows platform.
 *
 * Defined if SO_5_OS_WIN32 or SO_5_OS_WIN64 are defined.
 */

/*!
 * \def SO_5_OS_UNIX
 * \brief A marker for Unix platforms, but not macOS/iOS.
 */

/*!
 * \def SO_5_OS_APPLE
 * \brief A marker for macOS/iOS.
 */

#if defined( _WIN64 )
	#define SO_5_OS_WIN64
#endif

#if defined( _WIN32 )
	#define SO_5_OS_WIN32
#endif

#if defined( SO_5_OS_WIN32 ) || defined( SO_5_OS_WIN64 )
	#define SO_5_OS_WINDOWS
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
	#define SO_5_OS_UNIX
#endif

#if defined(__APPLE__)
	#define SO_5_OS_APPLE
#endif

