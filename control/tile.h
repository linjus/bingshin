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

namespace mapctrl
{

struct CONTROL_API pixelpair
{
	pixelpair(int x, int y)
		: x(x), y(y)
	{
	}

	int x;
	int y;
};

struct CONTROL_API tilepair
{
	tilepair(int x, int y)
		: x(x), y(y)
	{
	}

	int x;
	int y;
};

struct CONTROL_API lonlat
{
	lonlat(double lon, double lat)
		: lon(lon), lat(lat)
	{
	}

	double lon;
	double lat;
};

struct CONTROL_API screenpair
{
	screenpair(const screenpair &s)
		: x(s.x), y(s.y)
	{
	}

	screenpair(int x, int y)
		: x(x), y(y)
	{
	}

	int x;
	int y;
};

class CONTROL_API quadkey
{
public:
#ifdef _WIN32
	typedef __int64 quadkey_t;
#else
	typedef int64_t quadkey_t;
#endif

public:
	quadkey(const std::string &key);
	quadkey(quadkey_t key, unsigned int len);
	static quadkey epsilon();

	unsigned char operator[](int index) const;
	int get_lod() const;
	bool empty() const;

	bool has_upper() const;
	quadkey upper() const;

	quadkey intersect(const quadkey &r) const;
	unsigned char prefix() const;
	quadkey suffix(const quadkey &prefix) const;
	quadkey concat(const quadkey &suffix) const;

	std::string str() const;

	bool operator<(const quadkey &r) const
	{
		if (this->len < r.len) return true;
		if (this->len > r.len) return false;

		if (this->key < r.key) return true;
		return false;
	}

	bool operator==(const quadkey &r) const
	{
		if (this->len != r.len) return false;
		if (this->key != r.key) return false;
		return true;
	}
	bool operator!=(const quadkey &r) const
	{
		return !this->operator==(r);
	}

private:
	quadkey_t key;
	unsigned int len;
};

class CONTROL_API tilelayer
{
public:
	tilelayer(int lod);

	unsigned int get_tilesize() const;
	std::pair<double, double> get_lon_range() const;
	std::pair<double, double> get_lat_range() const;

	int get_zoomlevel() const
	{
		return this->zoomlevel;
	}

	const pixelpair & get_pixelnw() const
	{
		return this->pixelnw;
	}

	const pixelpair & get_pixelse() const
	{
		return this->pixelse;
	}

	void set_map(int lod, const pixelpair &center, const screenpair &size);
	void move_map(const screenpair &delta);
	void zoom_map(int delta);

	unsigned int get_mapsize() const;
	unsigned int get_tilenum() const;

	double get_resolution(const lonlat &ll) const;
	int get_zoomlevel(const lonlat &center, double londist, double latdist) const;

	pixelpair get_pixel(const lonlat &ll) const;
	pixelpair get_pixel(const screenpair &pos) const;
	pixelpair get_pixel(const tilepair &tile) const;

	tilepair get_tile(const pixelpair &pix) const;

	lonlat get_lonlat(const pixelpair &pix) const;

	screenpair get_screen(const tilepair &tile) const;
	screenpair get_screen(const pixelpair &pix) const;

	quadkey get_quadkey(const tilepair &tile) const;

	bool is_visible(const pixelpair &pix) const;

	struct iterator
	{
		iterator(const tilelayer &enclosing);
		bool movenext();
		tilepair currenttile() const;
		pixelpair currentpixel() const;

	private:
		const tilelayer &enclosing;
		pixelpair cur;
		pixelpair bound;
	};

	iterator visible() const;

private:
	int zoomlevel;
	pixelpair pixelnw;
	pixelpair pixelse;
	screenpair screensize;

	void adjust_range();
};

}
