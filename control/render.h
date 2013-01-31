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

class tilecache;

struct pngtexture
{
	static pngtexture * load(tilecache *enclosing, const std::string &path);

	virtual ~pngtexture()
	{
	}

	virtual bool is_bound() const = 0;
	virtual void bind() = 0;

#ifdef PLATFORM_CLR
	virtual System::Windows::Media::ImageSource^ get_tex() const = 0;
#else
	virtual GLuint get_tex() const = 0;
#endif
};

#ifdef PLATFORM_CLR
struct render_context
{
	bool valid;
	gcroot<System::Windows::Media::DrawingContext^> drawctx;
};
#endif

struct renderer
{
#ifdef PLATFORM_CLR
	virtual void set_context(render_context &ctx) = 0;
#endif

	virtual void draw_tile(int timestamp, tilecache *tiles, const mapctrl::screenpair &screensize, const mapctrl::tilelayer &layer, const float *factor, bool draw, const mapctrl::tilepair &tile, const mapctrl::pixelpair &pixel) = 0;

#ifdef IMPLEMENT_GPSPIN
	virtual void draw_gpspin(const mapctrl::screenpair &screen, const pngtexture *texture, const mapctrl::screenpair &hotpoint) = 0;
	virtual void draw_gpspin_shadow(const mapctrl::screenpair &screen, float radius) = 0;
#endif

	virtual ~renderer()
	{
	}

	static renderer * create(short tilesize);
};
