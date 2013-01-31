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

using namespace mapctrl;

static quadkey::quadkey_t extract_quadkey(const std::string &key)
{
	quadkey::quadkey_t r = 0;
	bool error = false;

	for (auto i = key.begin(); i != key.end(); ++i) {
		r <<= 2;

		unsigned char bit2 = *i - '0';
		if (bit2 > 3) error = true;

		r |= bit2;
	}

	if (error)
		return 0;
	return r;
}

quadkey::quadkey(const std::string &key)
	: key(extract_quadkey(key)), len(key.length())
{
}

quadkey::quadkey(quadkey_t key, unsigned int len)
	: key(key), len(len)
{
}

quadkey quadkey::epsilon()
{
	return quadkey(0, 0);
}

unsigned char quadkey::operator[](int index) const
{
	auto numshift = this->len - index;
	auto bit2 = (this->key >> ((numshift - 1) * 2)) & 3;
	return static_cast<unsigned char>(bit2);
}

int quadkey::get_lod() const
{
	return static_cast<int>(this->len);
}

bool quadkey::empty() const
{
	return this->len == 0;
}

bool quadkey::has_upper() const
{
	return this->len > 1;
}

quadkey quadkey::upper() const
{
	auto newkey = this->key >> 2;
	auto newlen = this->len - 1;
	return quadkey(newkey, newlen);
}

quadkey quadkey::intersect(const quadkey &r) const
{
	unsigned int i = 0;
	for ( ; i < this->len && i < r.len; ++i) {
		if ((*this)[i] != r[i]) break;
	}

	quadkey_t newkey = this->key >> (this->len - i) * 2;
	return quadkey(newkey, i);
}

unsigned char quadkey::prefix() const
{
	return this->operator[](0);
}

quadkey quadkey::suffix(const quadkey &prefix) const
{
	unsigned int newlen = this->len - prefix.len;

	quadkey_t mask = 0;
	for (unsigned int i = 0; i < newlen; ++i) {
		if (i) mask <<= 2;
		mask |= 3;
	}

	quadkey_t newkey = this->key & mask;
	return quadkey(newkey, newlen);
}

quadkey quadkey::concat(const quadkey &suffix) const
{
	quadkey_t newkey = this->key << (suffix.len * 2);
	newkey |= suffix.key;
	return quadkey(newkey, this->len + suffix.len);
}

std::string quadkey::str() const
{
	std::string r(this->len, '0');

	for (unsigned int i = 0; i < this->len; ++i) {
		unsigned char bit2 = this->operator[](i);
		r[i] = bit2 + '0';
	}

	return r;
}

template<typename T>
static T clip(T v, T mn, T mx)
{
	return std::min<T>(std::max<T>(v, mn), mx);
}

static const double EARTHRADIUS = 6378137;
static const double MIN_LAT = -85.05112878;
static const double MAX_LAT = 85.05112878;
static const double MIN_LON = -180;
static const double MAX_LON = 180;

struct tilesystem
{
	static const unsigned int TILESIZE = 256;

	static unsigned int mapsize(int lod)
	{
		return TILESIZE << lod;
	}

	static unsigned int tilenum(int lod)
	{
		return mapsize(lod) / TILESIZE;
	}

	static double ground_resolution(double latitude, int lod)
	{
		auto safelat = clip(latitude, MIN_LAT, MAX_LAT);
		return std::cos(safelat * M_PI / 180) * 2 * M_PI * EARTHRADIUS / mapsize(lod);
	}

	static pixelpair latlong2pixel(const lonlat &ll, int lod)
	{
		auto safelat = clip(ll.lat, MIN_LAT, MAX_LAT);
		auto safelon = clip(ll.lon, MIN_LON, MAX_LON);

		double x = (safelon + 180) / 360;

		double sinlat = std::sin(safelat * M_PI / 180);
		double y = 0.5 - std::log((1 + sinlat) / (1 - sinlat)) / (4 * M_PI);

		auto size = mapsize(lod);
		auto pixx = clip(static_cast<unsigned int>(x * size + 0.5), 0U, size - 1);
		auto pixy = clip(static_cast<unsigned int>(y * size + 0.5), 0U, size - 1);
		return pixelpair(pixx, pixy);
	}

	static lonlat pixel2latlong(const pixelpair &pix, int lod)
	{
		double size = mapsize(lod);
		double x = clip<double>(pix.x, 0, size - 1) / size - 0.5;
		double y = 0.5 - clip<double>(pix.y, 0, size - 1) / size;

		double lat = 90 - 360 * std::atan(std::exp(-y * 2 * M_PI)) / M_PI;
		double lon = 360 * x;
		return lonlat(lon, lat);
	}

	static tilepair pixel2tile(const pixelpair &pix)
	{
		return tilepair(pix.x / TILESIZE, pix.y / TILESIZE);
	}

	static pixelpair tile2pixel(const tilepair &tile)
	{
		return pixelpair(tile.x * TILESIZE, tile.y * TILESIZE);
	}

	static quadkey tile2quadkey(const tilepair &tile, int lod)
	{
		quadkey::quadkey_t key = 0;

		for (int i = lod; i > 0; --i) {
			unsigned char bit2 = 0;
			int mask = 1 << (i - 1);
			if (tile.x & mask)
				bit2++;
			if (tile.y & mask)
				bit2 += 2;

			key <<= 2;
			key |= bit2;
		}

		return quadkey(key, lod);
	}

	static tilepair quadkey2tile(const quadkey &key)
	{
		int tilex = 0, tiley = 0;

		int lod = key.get_lod();
		for (int i = lod; i > 0; --i) {
			int mask = i << (i - 1);
			switch (key[lod - i]) {
			case 0:
				break;
			case 1:
				tilex |= mask;
				break;
			case 2:
				tiley |= mask;
				break;
			case 3:
				tilex |= mask;
				tiley |= mask;
				break;
			}
		}

		return tilepair(tilex, tiley);
	}
};

static const int MINZOOMLEVEL = 1;
static const int MAXZOOMLEVEL = 20;

tilelayer::tilelayer(int lod)
	: zoomlevel(lod), pixelnw(pixelpair(0, 0)), pixelse(pixelpair(0, 0)), screensize(0, 0)
{
}

unsigned int tilelayer::get_tilesize() const
{
	return tilesystem::TILESIZE;
}

std::pair<double, double> tilelayer::get_lon_range() const
{
	return std::make_pair(MIN_LON, MAX_LON);
}

std::pair<double, double> tilelayer::get_lat_range() const
{
	return std::make_pair(MIN_LAT, MAX_LAT);
}

void tilelayer::set_map(int lod, const pixelpair &center, const screenpair &size)
{
	this->zoomlevel = lod;
	this->pixelnw = pixelpair(center.x - size.x / 2, center.y - size.y / 2);
	this->pixelse = pixelpair(this->pixelnw.x + size.x, this->pixelnw.y + size.y);
	this->screensize = size;

	this->adjust_range();
}

void tilelayer::move_map(const screenpair &delta)
{
	this->pixelnw.x += delta.x;
	this->pixelnw.y += delta.y;
	this->pixelse.x += delta.x;
	this->pixelse.y += delta.y;

	this->adjust_range();
}

void tilelayer::zoom_map(int delta)
{
	int newlod = this->zoomlevel + delta;
	newlod = std::max<int>(newlod, MINZOOMLEVEL);
	newlod = std::min<int>(newlod, MAXZOOMLEVEL);

	pixelpair pixcenter((this->pixelnw.x + this->pixelse.x) / 2, (this->pixelnw.y + this->pixelse.y) / 2);

	auto llcenter = tilesystem::pixel2latlong(pixcenter, this->zoomlevel);

	auto newpixcenter = tilesystem::latlong2pixel(llcenter, newlod);

	this->set_map(newlod, newpixcenter, this->screensize);
}

unsigned int tilelayer::get_mapsize() const
{
	return tilesystem::mapsize(this->zoomlevel);
}

unsigned int tilelayer::get_tilenum() const
{
	return tilesystem::tilenum(this->zoomlevel);
}

double tilelayer::get_resolution(const lonlat &ll) const
{
	return tilesystem::ground_resolution(ll.lat, this->zoomlevel);
}

int tilelayer::get_zoomlevel(const lonlat &center, double londist, double latdist) const
{
	for (int lod = MAXZOOMLEVEL; lod >= MINZOOMLEVEL; --lod) {
		pixelpair centerpix = tilesystem::latlong2pixel(center, lod);

		bool enough = false;
		for (int i = 0; ; ++i) {
			if (i == 4) {
				enough = true;
				break;
			}

			lonlat corner = center;
			if (i < 2) corner.lon -= londist;
			else corner.lon += londist;
			if (i % 2) corner.lat -= latdist;
			else corner.lat += latdist;

			pixelpair cornerpix = tilesystem::latlong2pixel(corner, lod);
			if (std::abs(cornerpix.x - centerpix.x) > this->screensize.x / 2) break;
			if (std::abs(cornerpix.y - centerpix.y) > this->screensize.y / 2) break;
		}
		if (enough) return lod;
	}
	return MINZOOMLEVEL;
}

pixelpair tilelayer::get_pixel(const lonlat &ll) const
{
	return tilesystem::latlong2pixel(ll, this->zoomlevel);
}

pixelpair tilelayer::get_pixel(const screenpair &pos) const
{
	auto x = this->pixelnw.x + pos.x;
	auto y = this->pixelnw.y + pos.y;
	return pixelpair(x, y);
}

pixelpair tilelayer::get_pixel(const tilepair &tile) const
{
	return tilesystem::tile2pixel(tile);
}

tilepair tilelayer::get_tile(const pixelpair &pix) const
{
	return tilesystem::pixel2tile(pix);
}

lonlat tilelayer::get_lonlat(const pixelpair &pix) const
{
	return tilesystem::pixel2latlong(pix, this->zoomlevel);
}

screenpair tilelayer::get_screen(const tilepair &tile) const
{
	auto pix = tilesystem::tile2pixel(tile);
	auto x = pix.x - this->pixelnw.x;
	auto y = pix.y - this->pixelnw.y;
	return screenpair(x, y);
}

screenpair tilelayer::get_screen(const pixelpair &pix) const
{
	auto x = pix.x - this->pixelnw.x;
	auto y = pix.y - this->pixelnw.y;
	return screenpair(x, y);
}

quadkey tilelayer::get_quadkey(const tilepair &tile) const
{
	return tilesystem::tile2quadkey(tile, this->zoomlevel);
}

bool tilelayer::is_visible(const pixelpair &pix) const
{
	if (pix.x < this->pixelnw.x) return false;
	if (pix.y < this->pixelnw.y) return false;

	if (pix.x > this->pixelse.x) return false;
	if (pix.y > this->pixelse.y) return false;

	return true;
}

tilelayer::iterator::iterator(const tilelayer &enclosing)
	: enclosing(enclosing), cur(pixelpair(0, 0)), bound(pixelpair(-1, -1))
{
}

bool tilelayer::iterator::movenext()
{
	if (this->bound.x == -1) {
		auto nw = this->enclosing.get_pixelnw();
		auto se = this->enclosing.get_pixelse();
		this->bound.x = se.x;
		if (nw.x > this->bound.x) {
			auto size = this->enclosing.get_mapsize();
			this->bound.x += size;
		}
		this->bound.y = se.y;

		this->cur.x = nw.x;
		this->cur.y = nw.y;
	}
	else {
		this->cur.x += tilesystem::TILESIZE;
		if (this->cur.x >= this->bound.x + static_cast<int>(tilesystem::TILESIZE)) {
			auto nw = this->enclosing.get_pixelnw();
			this->cur.x = nw.x;
			this->cur.y += tilesystem::TILESIZE;
			if (this->cur.y >= this->bound.y + static_cast<int>(tilesystem::TILESIZE))
				return false;
			if (this->cur.y >= this->bound.y && this->cur.y % tilesystem::TILESIZE == 0)
				return false;
		}
	}
	return true;
}

tilepair tilelayer::iterator::currenttile() const
{
	auto size = this->enclosing.get_mapsize();
	auto actualx = this->cur.x % size;
	return tilesystem::pixel2tile(pixelpair(actualx, this->cur.y));
}

pixelpair tilelayer::iterator::currentpixel() const
{
	return this->cur;
}

tilelayer::iterator tilelayer::visible() const
{
	return iterator(*this);
}

void tilelayer::adjust_range()
{
	auto mapsize = static_cast<int>(this->get_mapsize());

	if (mapsize < this->screensize.y) {
		this->pixelnw.y = 0;
		this->pixelse.y = mapsize;
	}
	else {
		if (this->pixelnw.y < 0) {
			auto shifty = -this->pixelnw.y;
			this->pixelnw.y = 0;
			this->pixelse.y += shifty;
		}
		else if (this->pixelse.y >= mapsize) {
			auto shifty = this->pixelse.y - mapsize;
			this->pixelnw.y -= shifty;
			this->pixelse.y = mapsize;
		}
	}

	if (this->pixelnw.x > mapsize || this->pixelnw.x < -mapsize) {
		auto count = this->pixelnw.x / mapsize;
		this->pixelnw.x -= mapsize * count;
		this->pixelse.x -= mapsize * count;
	}

#ifdef LOGGING_VISIBLETILES
	logger::info("NW", this->pixelnw.x, this->pixelnw.y);
	logger::info("SE", this->pixelse.x, this->pixelse.y);
#endif
}
