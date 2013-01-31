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

#ifdef WIN32
extern "C" 
#endif
GL_API int GL_APIENTRY _dgles_load_library(void *, void *(*)(void *, const char *));

static void *proc_loader(void *h, const char *name)
{
	return SDL_GL_GetProcAddress(name);
}

namespace config
{
	static const char *reposroot = ".";

	static const int zoomlevel = 13;
	static const int screenwidth = 1024;
	static const int screenheight = 700;

	static const int numtileslimit = 100;

	namespace location
	{
		static const double initial_lon = -88.228411;
		static const double initial_lat = 40.110539;
	};
}

namespace logger
{
	static void internal(const std::string &msg)
	{
		std::cout << msg.c_str() << std::endl;
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

static void on_tileupdate();

class lindsay
{
public:
	lindsay()
		: screen(nullptr), map(nullptr)
	{
	}

	~lindsay()
	{
		if (this->map)
			delete this->map;
	}

	bool initialize()
	{
		mapctrl::screenpair mapsize(config::screenwidth, config::screenheight);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);

		this->screen = SDL_SetVideoMode(mapsize.x, mapsize.y, 0, SDL_OPENGL);

#if WIN32
		_dgles_load_library(NULL, proc_loader);
#endif

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof(0.0f, static_cast<float>(mapsize.x), static_cast<float>(mapsize.y), 0.0f, -1.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
//		glEnable(GL_BLEND);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		this->map = mapctrl::mapcontrol::create(config::reposroot, config::zoomlevel, mapsize, config::numtileslimit);

		return this->prepare();
	}

	void run()
	{
		const Uint32 probeperiod = 100;

		for ( ; ; ) {
			if (!this->handle_tick()) break;
		}
	}

	void finalize()
	{
	}

private:
	SDL_Surface *screen;
	mapctrl::mapcontrol *map;

	bool prepare()
	{
		if (!this->map->initialize(config::reposroot, &on_tileupdate)) return false;

		double lon = config::location::initial_lon;
		double lat = config::location::initial_lat;
		mapctrl::lonlat ll(lon, lat);

		this->map->set_center(ll);
		this->map->set_gps(&ll, 0.0f);
		return true;
	}

	bool display()
	{
		// It seems mouse movement invalidates the client area. So, the scene should be drawn each tick.
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glClear(GL_COLOR_BUFFER_BIT);

		this->map->draw();

		return !this->map->needs_draw();
	}

	bool handle_tick()
	{
		SDL_Event evt;

		bool complete = this->display();
		SDL_GL_SwapBuffers();

		if (complete) {
//			SDL_WaitEvent(&evt);
			while (!SDL_PollEvent(&evt)) {
				SDL_Delay(100);
			}
		}
		else {
			if (!SDL_PollEvent(&evt))
				return true;
		}

		do {
			switch (evt.type) {
			case SDL_KEYDOWN:
				switch (evt.key.keysym.sym) {
				case SDLK_ESCAPE:
					return false;
				}
				break;
			}
			this->map->handle_message(evt);
		} while (SDL_PollEvent(&evt));

		return evt.type != SDL_QUIT;
	}
};

#define SDL_TILE_UPDATED  (SDL_USEREVENT + 1)

static void on_tileupdate()
{
	SDL_Event evt;
	evt.type = SDL_TILE_UPDATED;

	SDL_PushEvent(&evt);
}

class program
{
public:
	void initialize()
	{
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
		PDL_Init(0);
	}

	bool run()
	{
		lindsay l;
		if (!l.initialize()) return false;
		l.run();
		l.finalize();
		return true;
	}

	void finalize()
	{
		PDL_Quit();
		SDL_Quit();
	}
};

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

int main(int argc, char **argv)
{
	program pgm;

	pgm.initialize();
	pgm.run();
	pgm.finalize();

	return 0;
}
