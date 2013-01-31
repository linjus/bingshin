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

#ifdef LOGGING
#ifdef PLATFORM_WEBOS
#include <syslog.h>
#endif
#endif

#ifdef LOGGING
namespace logger
{
	static void internal(const std::string &msg)
	{
#ifdef PLATFORM_WEBOS
		syslog(LOG_INFO, "%s", msg.c_str());
#else
		std::cout << msg.c_str() << std::endl;
#endif
	}

	template<typename T>
	static void info(const T &a)
	{
		std::ostringstream oss;
		oss << a;
		internal(oss.str());
	}
	template<typename T, typename U>
	static void info(const T &a, const U &b)
	{
		std::ostringstream oss;
		oss << a << ' ' << b;
		internal(oss.str());
	}
	template<typename T, typename U, typename V>
	static void info(const T &a, const U &b, const V &c)
	{
		std::ostringstream oss;
		oss << a << ' ' << b << ' ' << c;
		internal(oss.str());
	}
	template<typename T, typename U, typename V, typename W>
	static void info(const T &a, const U &b, const V &c, const W &d)
	{
		std::ostringstream oss;
		oss << a << ' ' << b << ' ' << c << ' ' << d;
		internal(oss.str());
	}
}
#endif

#include "file.h"
#include "repository.h"
