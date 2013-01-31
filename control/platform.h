// This file is part of bingshin.
// 
// bingshin is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
// 
// bingshin is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with bingshin. If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

// Detect the platform
#ifdef _WIN32
#ifdef _MANAGED
#define PLATFORM_CLR
#else
#define PLATFORM_WIN32
#endif
#elif defined __APPLE__
#define PLATFORM_IOS
#else
#define PLATFORM_WEBOS
#endif

// Set platform-specific configuration
#if defined PLATFORM_WIN32 || defined PLATFORM_WEBOS
#define USE_SDL
#endif
#if defined PLATFORM_WIN32 || defined PLATFORM_IOS || defined PLATFORM_WEBOS
#define USE_OPENGL
#endif

// Decide features to implement
#if defined PLATFORM_WIN32 || defined PLATFORM_IOS || defined PLATFORM_WEBOS
#define IMPLEMENT_GPSPIN
#endif

#ifdef PLATFORM_CLR
#define IMPLEMENT_DOWNLOAD
#define IMPLEMENT_PIN
#define IMPLEMENT_TRAIL
#endif
