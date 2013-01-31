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

#include "stdafx.h"

std::string path::combine(const std::string &a, const std::string &b)
{
	if (a.length() > 0 && a[a.length() - 1] == separator)
		return a + b;
	return a + separator + b;
}

#ifdef PLATFORM_CLR
bool file::exists(const std::string &abspath)
{
	auto str = gcnew System::String(abspath.c_str());
	return System::IO::File::Exists(str);
}
#elif defined PLATFORM_WIN32
bool file::exists(const std::string &abspath)
{
	DWORD attr = ::GetFileAttributesA(abspath.c_str());
	return attr != 0xFFFFFFFF;
}
#else
bool file::exists(const std::string &abspath)
{
	struct stat st;
	return stat(abspath.c_str(), &st) == 0;
}
#endif
