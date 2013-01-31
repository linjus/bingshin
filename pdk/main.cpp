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

#include <memory>
#include <string>
#include <sstream>
#include <iostream>

#include <syslog.h>

#include "SDL.h"
#include "SDL_image.h"
#include "GLES/gl.h"
#include "GLES/glext.h"
#include "PDL.h"

#include "mapcontrol.h"

#define nullptr  0

namespace config
{
	static const char *reposroot = "/media/internal/.lindsay_repository";

	static const int zoomlevel = 13;
	static const int screenwidth = 320;
	static const int screenheight = 400;

	static const int numtileslimit = 30;

	namespace location
	{
		// Champaign, IL
		// static const double initial_lon = -88.228411;
		// static const double initial_lat = 40.110539;

		// Cambridge, MA
		static const double initial_lon = -71.110556;
		static const double initial_lat = 42.373611;
	};
}

namespace log
{
	static void internal(const std::string &msg)
	{
		syslog(LOG_INFO, "%s", msg.c_str());
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

void no_memory_handler()
{
	log::info("failed to allocated memory");
	exit(1);
}

PDL_bool mojo_handler_zoom(PDL_JSParameters *params);
PDL_bool mojo_handler_showGPS(PDL_JSParameters *params);
PDL_bool mojo_handler_updateGPS(PDL_JSParameters *params);
PDL_bool mojo_handler_invalidateGPS(PDL_JSParameters *params);
PDL_bool mojo_handler_showGPSLocation(PDL_JSParameters *params);

class mojo_exception : public std::exception
{
public:
	mojo_exception(const std::string &str)
		: message(str)
	{
	}

	~mojo_exception() throw()
	{
	}

	virtual const char * what() const throw()
	{
		return this->message.c_str();
	}

private:
	std::string message;
};

class mojo
{
	struct gpslocation
	{
		bool valid;
		mapctrl::lonlat location;
		float accuracy;

		gpslocation()
			: valid(false), location(0.0f, 0.0f)
		{
		}

		gpslocation(double lon, double lat, double acc)
			: valid(true), location(lon, lat), accuracy(static_cast<float>(acc))
		{
		}
	};

public:
	mojo()
	{
		this->reset_flags();
	}

#define PDKCALL(exp)  this->pdkcall(exp, __LINE__)
	bool initialize()
	{
#define REGISTER(name) \
		PDKCALL(PDL_RegisterJSHandler(#name, mojo_handler_##name))
		REGISTER(zoom);
		REGISTER(showGPS);
		REGISTER(updateGPS);
		REGISTER(invalidateGPS);
		REGISTER(showGPSLocation);
#undef REGISTER

		PDKCALL(PDL_JSRegistrationComplete());
		return true;
	}

	void zoom(int delta)
	{
		this->flag_zoom = std::make_pair(true, delta);
	}
	std::pair<bool, int> read_flag_zoom()
	{
		auto ret = this->flag_zoom;
		this->flag_zoom.first = false;
		return ret;
	}

	void show_gps(bool show)
	{
		this->flag_showgps = std::make_pair(true, show);
	}
	std::pair<bool, bool> read_flag_showgps()
	{
		auto ret = this->flag_showgps;
		this->flag_showgps.first = false;
		return ret;
	}

	void update_gps(double lon, double lat, double accuracy)
	{
		this->flag_gpslocation = std::make_pair(true, gpslocation(lon, lat, accuracy));
	}
	void invalidate_gps()
	{
		this->flag_gpslocation = std::make_pair(true, gpslocation());
	}
	std::pair<bool, gpslocation> read_flag_gps_location()
	{
		auto ret = this->flag_gpslocation;
		this->flag_gpslocation.first = false;
		return ret;
	}

	void locate_me()
	{
		this->flag_locateme = true;
	}
	bool read_flag_locate_me()
	{
		auto ret = this->flag_locateme;
		this->flag_locateme = false;
		return ret;
	}

#undef PDKCALL

private:
	std::pair<bool, int> flag_zoom;
	std::pair<bool, bool> flag_showgps;
	std::pair<bool, gpslocation> flag_gpslocation;
	bool flag_locateme;

	void reset_flags()
	{
		this->flag_zoom.first = false;
		this->flag_showgps.first = false;
		this->flag_gpslocation.first = false;
		this->flag_locateme = false;
	}

	void pdkcall(PDL_Err errcode, int line)
	{
		if (errcode == PDL_NOERROR) return;

		std::ostringstream oss;
		oss << line << ": " << errcode;
		log::info(oss.str());
		throw mojo_exception(oss.str());
	}
};

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

	bool initialize(const mapctrl::screenpair &mapsize)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);

		this->screen = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof(0.0f, static_cast<float>(mapsize.x), static_cast<float>(mapsize.y), 0.0f, -1.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_ALPHA_TEST);

		this->map = mapctrl::mapcontrol::create(config::reposroot, config::zoomlevel, mapsize, config::numtileslimit);

		return this->prepare();
	}

	void run(mojo *mojoitf)
	{
		for (bool paused = false; ; ) {
			this->handle_mojo(mojoitf);

			if (!this->handle_tick(paused)) break;
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
		if (!this->map->initialize(config::reposroot, on_tileupdate)) return false;

		double lon = config::location::initial_lon;
		double lat = config::location::initial_lat;
		mapctrl::lonlat ll(lon, lat);

		this->map->set_center(ll);
		return true;
	}

	bool display(bool force)
	{
		bool drawn = false;

		if (force || this->map->needs_draw()) {
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glClear(GL_COLOR_BUFFER_BIT);

			this->map->draw();

			drawn = true;
		}

		return drawn;
	}

	bool handle_tick(bool &paused)
	{
		SDL_Event evt;

		if (paused) {
			SDL_WaitEvent(&evt);
			switch (evt.type) {
			case SDL_ACTIVEEVENT:
				if (evt.active.state & SDL_APPACTIVE && evt.active.gain == 1) {
					paused = false;
					this->display(true);
				}
				break;
			case SDL_VIDEOEXPOSE:
				this->display(true);
				break;
			}
		}
		else {
			bool drawn = this->display(false);
			SDL_GL_SwapBuffers();

			if (!drawn)
				SDL_WaitEvent(&evt);
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
				case SDL_ACTIVEEVENT:
					if (evt.active.state & SDL_APPACTIVE && evt.active.gain == 0) {
						paused = true;
					}
					break;
				}
				this->map->handle_message(evt);
			} while (SDL_PollEvent(&evt));
		}

		return evt.type != SDL_QUIT;
	}

	void handle_mojo(mojo *mojoitf)
	{
		auto zoom = mojoitf->read_flag_zoom();
		if (zoom.first)
			this->map->set_zoomdelta(zoom.second);

		auto showgps = mojoitf->read_flag_showgps();
		if (showgps.first) {
			if (!showgps.second) this->map->set_gps(nullptr, 0.0f);
		}

		auto gpsloc = mojoitf->read_flag_gps_location();
		if (gpsloc.first) {
			if (gpsloc.second.valid)
				this->map->set_gps(&gpsloc.second.location, gpsloc.second.accuracy);
			else
				this->map->set_gps(nullptr, -1.0f);
		}

		auto locme = mojoitf->read_flag_locate_me();
		if (locme) {
			auto loc = this->map->get_gps();
			if (loc) this->map->set_center(*loc);
		}
	}
};

static mojo *mojoitf = nullptr;

#define SDL_MOJO_INPUT  (SDL_USEREVENT + 0)
#define SDL_TILE_UPDATED  (SDL_USEREVENT + 1)

static void notify_mojo_handling()
{
	SDL_Event evt;
	evt.type = SDL_MOJO_INPUT;

	SDL_PushEvent(&evt);
}

PDL_bool mojo_handler_zoom(PDL_JSParameters *params)
{
	int nargs = PDL_GetNumJSParams(params);
	if (nargs != 1) return PDL_FALSE;

	int delta = PDL_GetJSParamInt(params, 0);
	log::info("mojo zoom", delta);

	mojoitf->zoom(delta);
	notify_mojo_handling();
	return PDL_TRUE;
}

PDL_bool mojo_handler_showGPS(PDL_JSParameters *params)
{
	int nargs = PDL_GetNumJSParams(params);
	if (nargs != 1) return PDL_FALSE;

	bool show = PDL_GetJSParamInt(params, 0) ? true : false;
	log::info("mojo showGPS", show);

	mojoitf->show_gps(show);
	notify_mojo_handling();
	return PDL_TRUE;
}

PDL_bool mojo_handler_updateGPS(PDL_JSParameters *params)
{
	int nargs = PDL_GetNumJSParams(params);
	if (nargs != 3) return PDL_FALSE;

	double lon = PDL_GetJSParamDouble(params, 0);
	double lat = PDL_GetJSParamDouble(params, 1);
	double accuracy = PDL_GetJSParamDouble(params, 2);
	log::info("mojo updateGPS", lon, lat, accuracy);

	mojoitf->update_gps(lon, lat, accuracy);
	notify_mojo_handling();
	return PDL_TRUE;
}

PDL_bool mojo_handler_invalidateGPS(PDL_JSParameters *params)
{
	int nargs = PDL_GetNumJSParams(params);
	if (nargs != 1) return PDL_FALSE;

	int errcode = PDL_GetJSParamInt(params, 0);
	log::info("mojo invalidateGPS", errcode);

	mojoitf->invalidate_gps();
	notify_mojo_handling();
	return PDL_TRUE;
}

PDL_bool mojo_handler_showGPSLocation(PDL_JSParameters *params)
{
	int nargs = PDL_GetNumJSParams(params);
	if (nargs != 0) return PDL_FALSE;

	log::info("mojo showGPSLocation");

	mojoitf->locate_me();
	notify_mojo_handling();
	return PDL_TRUE;
}

static void on_tileupdate()
{
	SDL_Event evt;
	evt.type = SDL_TILE_UPDATED;

	SDL_PushEvent(&evt);
}

class program
{
public:
	program()
	{
		this->mojoitf.reset(new mojo());
	}

	void initialize()
	{
		SDL_Init(SDL_INIT_VIDEO);
		PDL_Init(0);
		this->mojoitf->initialize();
		::mojoitf = this->mojoitf.get();
	}

	bool run(const mapctrl::screenpair &mapsize)
	{
		lindsay l;
		if (!l.initialize(mapsize)) return false;
		l.run(this->mojoitf.get());
		l.finalize();
		return true;
	}

	void finalize()
	{
		PDL_Quit();
		SDL_Quit();
	}

private:
	std::unique_ptr<mojo> mojoitf;
};

int main(int argc, char **argv)
{
	mapctrl::screenpair mapsize(config::screenwidth, config::screenheight);
	if (argc == 3) {
		std::istringstream is1(argv[1]);
		is1 >> mapsize.x;
		std::istringstream is2(argv[2]);
		is2 >> mapsize.y;
	}

	openlog("lindsay", 0, LOG_USER);
	std::set_new_handler(no_memory_handler);

	program pgm;

	pgm.initialize();
	pgm.run(mapsize);
	pgm.finalize();

	return 0;
}
