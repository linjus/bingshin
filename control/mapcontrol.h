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

#ifdef PLATFORM_WIN32
#ifdef CONTROL_EXPORTS
#define CONTROL_API __declspec(dllexport)
#else
#define CONTROL_API __declspec(dllimport)
#endif
#else
#define CONTROL_API
#endif

#ifdef _DEBUG
//#define LOGGING
#endif

#ifdef LOGGING
//#define LOGGING_TEXTURE
//#define LOGGING_QUADTRIE
//#define LOGGING_VISIBLETILES
#define LOGGING_INERTIA
#endif

#include "tile.h"
#include "render.h"
#include "map.h"
