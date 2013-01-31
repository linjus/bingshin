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

#include "platform.h"

#define _USE_MATH_DEFINES
#include <memory>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <set>


#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#endif
#if defined PLATFORM_IOS || defined PLATFORM_WEBOS
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

#if defined PLATFORM_CLR
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>
#include <msclr/auto_gcroot.h>
#include <vcclr.h>
#using <mscorlib.dll>
#using "PresentationFramework.dll"
#using "PresentationCore.dll"
#using "WindowsBase.dll"
#endif

#ifdef USE_SDL
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_thread.h"
#ifdef PLATFORM_IOS
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#else
#include "GLES/gl.h"
#include "GLES/glext.h"
#endif
#endif

// Set compiler-specific pragmas, macros and so on
#ifdef _MSC_VER
#pragma warning(disable:4512)
#else
#define nullptr  0
#endif

#include "mapcontrol.h"

#include "impl.h"

#ifdef PLATFORM_IOS
#include "objc.h"
#endif
