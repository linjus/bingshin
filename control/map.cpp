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

#ifdef PLATFORM_CLR
using namespace System::Windows;
using namespace System::Windows::Controls;
#endif

namespace config
{
	namespace control
	{
#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
		static const float move_inertia_duration = 0.5f;
		static const float move_inertia_mindeltat = 0.03f;
		static const float zoom_duration = 0.3f;
		static const float fly_zoom_duration_factor = 0.3f;
		static const float fly_move_duration_factor = 0.2f;
#else
		static const float move_inertia_duration = 0.5f;
		static const float move_inertia_mindeltat = 0.1f;
		static const float zoom_duration = 0.3f;
		static const float fly_zoom_duration_factor = 0.3f;
		static const float fly_move_duration_factor = 0.2f;
#endif
	};
}

class progresstimer_impl : public progresstimer
{
public:
	progresstimer_impl()
		: running(false)
	{
#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
		::QueryPerformanceFrequency(&this->freq);
#endif
	}

	virtual void start(float duration)
	{
#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
		::QueryPerformanceCounter(&this->startsat);
#else
		gettimeofday(&this->startsat, NULL);
#endif
		this->duration = duration;
		this->running = true;
	}

	virtual void stop()
	{
		this->running = false;
	}

	virtual float get_progress()
	{
		float elapsed = this->get_elapsed();

		if (elapsed > this->duration) {
			this->running = false;
			return 1;
		}
		return elapsed / this->duration;
	}

	virtual float get_elapsed() const
	{
#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
		LARGE_INTEGER current;
		::QueryPerformanceCounter(&current);

		float elapsed = (current.QuadPart - this->startsat.QuadPart) / static_cast<float>(this->freq.QuadPart);
		return elapsed;
#else
		struct timeval current;
		gettimeofday(&current, NULL);

		struct timeval diff;
		timeval_subtract(&diff, current, this->startsat);

		float elapsed = diff.tv_sec + diff.tv_usec / 1000000.0f;
		return elapsed;
#endif
	}

	virtual bool is_running() const
	{
		return this->running;
	}

private:
#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
	LARGE_INTEGER freq;
	LARGE_INTEGER startsat;
#else
	struct timeval startsat;
#endif
	float duration;
	bool running;

#if defined PLATFORM_WIN32 || defined PLATFORM_CLR
#else
	static int timeval_subtract (struct timeval *result, struct timeval x, struct timeval y)
	{
		if (x.tv_usec < y.tv_usec) {
			int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
			y.tv_usec -= 1000000 * nsec;
			y.tv_sec += nsec;
		}
		if (x.tv_usec - y.tv_usec > 1000000) {
			int nsec = (x.tv_usec - y.tv_usec) / 1000000;
			y.tv_usec += 1000000 * nsec;
			y.tv_sec -= nsec;
		}

		result->tv_sec = x.tv_sec - y.tv_sec;
		result->tv_usec = x.tv_usec - y.tv_usec;

		return x.tv_sec < y.tv_sec;
	}
#endif
};

progresstimer * progresstimer::create()
{
	return new progresstimer_impl();
}

class mutex_impl : public mutex
{
#ifdef USE_SDL
public:
	mutex_impl()
		: sdlmutex(nullptr)
	{
		this->sdlmutex = SDL_CreateMutex();
	}

	virtual ~mutex_impl()
	{
		if (this->sdlmutex)
			SDL_DestroyMutex(this->sdlmutex);
	}

	virtual void lock()
	{
		SDL_mutexP(this->sdlmutex);
	}

	virtual void unlock()
	{
		SDL_mutexV(this->sdlmutex);
	}

	virtual SDL_mutex * get_mutex()
	{
		return this->sdlmutex;
	}

private:
	SDL_mutex *sdlmutex;
#elif defined PLATFORM_CLR
public:
	mutex_impl()
	{
		::InitializeCriticalSection(&this->cs);
	}

	virtual ~mutex_impl()
	{
		::DeleteCriticalSection(&this->cs);
	}

	virtual void lock()
	{
		::EnterCriticalSection(&this->cs);
	}

	virtual void unlock()
	{
		::LeaveCriticalSection(&this->cs);
	}

	virtual CRITICAL_SECTION * get_mutex()
	{
		return &this->cs;
	}

private:
	CRITICAL_SECTION cs;
#elif defined PLATFORM_IOS
public:
	mutex_impl()
    {
        pthread_mutex_init(&this->pthmutex, NULL);
	}
    
	virtual ~mutex_impl()
	{
        pthread_mutex_destroy(&this->pthmutex);
	}
    
	virtual void lock()
	{
        pthread_mutex_lock(&this->pthmutex);
	}
    
	virtual void unlock()
    {
        pthread_mutex_unlock(&this->pthmutex);
	}
    
	virtual pthread_mutex_t * get_mutex()
	{
		return &this->pthmutex;
	}
    
private:
    pthread_mutex_t pthmutex;
#endif
};

mutex * mutex::create()
{
	return new mutex_impl();
}

class condvar_impl : public condvar
{
#ifdef USE_SDL
public:
    condvar_impl()
        : sdlcond(nullptr)
    {
        this->sdlcond = SDL_CreateCond();
    }
    
    virtual ~condvar_impl()
    {
        if (this->sdlcond)
            SDL_DestroyCond(this->sdlcond);
    }
    
    virtual void signal()
    {
        SDL_CondSignal(this->sdlcond);
    }

    virtual void wait(mutex &mut)
    {
		SDL_CondWait(this->sdlcond, mut.get_mutex());
    }

private:
    SDL_cond *sdlcond;
#elif defined PLATFORM_CLR
public:
    condvar_impl()
    {
		::InitializeConditionVariable(&this->cv);
    }
    
    virtual ~condvar_impl()
    {
    }
    
    virtual void signal()
    {
		::WakeConditionVariable(&this->cv);
    }

    virtual void wait(mutex &mut)
    {
		::SleepConditionVariableCS(&this->cv, mut.get_mutex(), INFINITE);
    }

private:
	CONDITION_VARIABLE cv;
#elif defined PLATFORM_IOS
public:
    condvar_impl()
    {
        pthread_cond_init(&this->pthcond, NULL);
    }

    virtual ~condvar_impl()
    {
        pthread_cond_destroy(&this->pthcond);
    }
    
    virtual void signal()
    {
        pthread_cond_signal(&this->pthcond);
    }
    
    virtual void wait(mutex &mut)
    {
        pthread_cond_wait(&this->pthcond, mut.get_mutex());
    }

private:
    pthread_cond_t pthcond;
#endif
};

#ifdef USE_SDL
class thread_impl : public thread
{
public:
	thread_impl(int (*entry)(void *data), void *data)
	{
		this->worker = SDL_CreateThread(entry, data);
	}

	virtual void waitjoin()
	{
		int status = 0;
		SDL_WaitThread(this->worker, &status);
	}

private:
	SDL_Thread *worker;
};
#elif defined PLATFORM_CLR
class thread_impl : public thread
{
public:
	thread_impl(void (*entry)(void *data), void *data)
	{
		_beginthread(entry, 0, data);
	}

	virtual void waitjoin()
	{
		::WaitForSingleObject(this->handle, INFINITE);
	}

private:
	HANDLE handle;
};
#elif defined PLATFORM_IOS
class thread_impl : public thread
{
public:
	thread_impl(int (*entry)(void *data), void *data)
	{
		pthread_create(&this->worker, NULL, entry, data);
	}

	virtual void waitjoin()
	{
		pthread_join(this->worker, NULL);
	}

private:
	pthread_t worker;
};
#endif

#ifdef PLATFORM_CLR
thread * thread::create(void (*entry)(void *data), void *data)
{
	return new thread_impl(entry, data);
}
#else
thread * thread::create(int (*entry)(void *data), void *data)
{
	return new thread_impl(entry, data);
}
#endif

condvar * condvar::create()
{
    return new condvar_impl();
}

struct moving
{
	moving()
		: tracking(false), lastmove(std::make_pair(0.0f, screenpair(0, 0))), speed(std::make_pair(0.0f, 0.0f)), releasetime(0.0f)
	{
	}

	void press()
	{
		this->timer.start(0.0f);
		this->tracking = true;

		this->speed = std::make_pair(0.0f, 0.0f);

		this->lastmove.first = this->timer.get_elapsed();
		this->lastmove.second = screenpair(0, 0);
#ifdef LOGGING_INERTIA
		logger::info("moving::press");
#endif
	}

	void move(int deltax, int deltay)
	{
		float now = this->timer.get_elapsed();
		float t = std::max<float>(config::control::move_inertia_mindeltat, now - this->lastmove.first);

		this->lastmove.first = now;
		this->lastmove.second.x += -deltax;
		this->lastmove.second.y += -deltay;

		this->speed.first = this->lastmove.second.x / t;
		this->speed.second = this->lastmove.second.y / t;

#ifdef LOGGING_INERTIA
		logger::info("moving::move lastmove", this->lastmove.first, this->lastmove.second.x, this->lastmove.second.y);
		logger::info("moving::move speed", this->speed.first, this->speed.second);
#endif
	}

	void release()
	{
		this->tracking = false;

		this->releasetime = this->timer.get_elapsed();

		this->lastmove.second.x = 0;
		this->lastmove.second.y = 0;

#ifdef LOGGING_INERTIA
		logger::info("moving::release speed", this->lastmove.first, this->speed.first, this->speed.second);
#endif
	}

	screenpair get_delta()
	{
		if (this->tracking) {
			screenpair delta(this->lastmove.second);
			this->lastmove.second = screenpair(0, 0);
#ifdef LOGGING_INERTIA
			logger::info("moving::get_delta (tracking)", delta.x, delta.y);
#endif
			return delta;
		}

		float t = this->timer.get_elapsed() - this->releasetime;
		if (t > config::control::move_inertia_duration) {
			this->timer.stop();
#ifdef LOGGING_INERTIA
			logger::info("moving::get_delta (total inertia)", this->lastmove.second.x, this->lastmove.second.y);
#endif
			return screenpair(0, 0);
		}

		screenpair loc(this->get_inertia_location(t));
		screenpair delta(loc.x - this->lastmove.second.x, loc.y - this->lastmove.second.y);
#ifdef LOGGING_INERTIA
		logger::info("moving::get_delta (location)", loc.x, loc.y);
		logger::info("moving::get_delta (done)", this->lastmove.second.x, this->lastmove.second.y);
		logger::info("moving::get_delta (inertia)", delta.x, delta.y);
#endif
		this->lastmove.second.x = loc.x;
		this->lastmove.second.y = loc.y;
		return delta;
#if 0
		if (this->tracking) {
			screenpair delta(this->lastmove.second);
			this->lastmove.second = screenpair(0, 0);
#ifdef LOGGING_INERTIA
			logger::info("moving::get_delta (tracking)", delta.x, delta.y);
#endif
			return delta;
		}

		float curtime = this->timer.get_elapsed() - this->lastmove.first;
		float reltime = this->releasetime - this->lastmove.first;

		if (curtime > config::control::move_inertia_duration) {
			this->timer.stop();
			return screenpair(0, 0);
		}
		else {
			screenpair curloc = this->get_inertia_location(curtime);
			screenpair relloc = this->get_inertia_location(reltime);
			screenpair delta(curloc.x - relloc.x, curloc.y - relloc.y);
#ifdef LOGGING_INERTIA
			logger::info("moving::get_delta current", curtime, curloc.x, curloc.y);
			logger::info("moving::get_delta release", reltime, relloc.x, relloc.y);
#endif
			return delta;
		}
#endif
	}

	bool is_active() const
	{
		return this->timer.is_running();
	}

	bool is_tracking() const
	{
		return this->tracking;
	}

private:
	progresstimer_impl timer;
	bool tracking;
	std::pair<float, screenpair> lastmove;
	std::pair<float, float> speed;
	float releasetime;

	screenpair get_inertia_location(float t)
	{
		float x = -0.5f * this->speed.first / config::control::move_inertia_duration * t * t + this->speed.first * t;
		float y = -0.5f * this->speed.second / config::control::move_inertia_duration * t * t + this->speed.second * t;
		return screenpair(static_cast<int>(x), static_cast<int>(y));
	}
};

struct zooming
{
	void start()
	{
		this->timer.start(config::control::zoom_duration);
	}

	template<typename T>
	T smooth(const T &from, const T &to)
	{
		float progress = this->timer.get_progress();
		float current = (to - from) * progress * progress + from;
		return static_cast<T>(current);
	}

	bool is_active() const
	{
		return this->timer.is_running();
	}

private:
	progresstimer_impl timer;
};

struct flying
{
	flying()
		: state(0)
	{
		this->layers.reserve(4);
	}

	void start(const tilelayer &currentlayer, int destlod, const lonlat &destll, const screenpair &screensize)
	{
		{
			this->layers.clear();
			this->layers.push_back(currentlayer);

			tilelayer first = currentlayer;
			first.zoom_map(destlod - currentlayer.get_zoomlevel());
			this->layers.push_back(first);

			tilelayer third(destlod);
			auto destpix = third.get_pixel(destll);
			third.set_map(destlod, destpix, screensize);
			this->layers.push_back(third);
			this->layers.push_back(third);

			{
				auto nw = third.get_pixelnw();
				auto se = third.get_pixelse();
				pixelpair pair((nw.x + se.x) / 2, (nw.y + se.y) / 2);
				auto llnw = third.get_lonlat(nw);
				auto llse = third.get_lonlat(se);
				auto llcenter = third.get_lonlat(pair);
			}
		}

		{
			auto tiledist = this->calculate_tiledist(this->layers[1], this->layers[3]);
			int lodboost = static_cast<int>(tiledist / 100);
			this->layers[1].zoom_map(-lodboost);
			this->layers[2].zoom_map(this->layers[1].get_zoomlevel() - this->layers[2].get_zoomlevel());
		}

		this->state = this->layers[0].get_zoomlevel() == this->layers[1].get_zoomlevel() ? 1 : 0;
		this->next();
	}

	bool tick()
	{
		this->timer.get_progress();
		if (this->timer.is_running())
			return true;
		return this->next();
	}

	bool is_zooming() const
	{
		return this->state == 1 || this->state == 3;
	}

	float zooming(tilelayer &destlayer, tilelayer &srclayer)
	{
		int layerindex = this->state - 1;
		srclayer = this->layers[layerindex];
		destlayer = this->layers[layerindex + 1];

		float progress = this->timer.get_progress();
		return progress * progress;
	}

	void moving(tilelayer &mainlayer)
	{
		float progress = this->timer.get_progress();

		auto &srcnw = this->layers[1].get_pixelnw();
		auto &destnw = this->layers[2].get_pixelnw();

		float curx = (destnw.x - srcnw.x) * progress + srcnw.x;
		float cury = (destnw.y - srcnw.y) * progress + srcnw.y;

		auto previous = mainlayer.get_pixelnw();
		int deltax = static_cast<int>(curx) - previous.x;
		int deltay = static_cast<int>(cury) - previous.y;
		mainlayer.move_map(screenpair(deltax, deltay));
	}

	bool is_active() const
	{
		return this->state != 0;
	}

private:
	progresstimer_impl timer;
	int state;
	std::vector<tilelayer> layers;

	bool next()
	{
		++this->state;

		switch (this->state) {
		case 1:
		case 3: {
				int layerindex = this->state - 1;
				auto &srclayer = this->layers[layerindex];
				auto &destlayer = this->layers[layerindex + 1];
				int deltalod = std::abs(destlayer.get_zoomlevel() - srclayer.get_zoomlevel());
				this->timer.start(config::control::fly_zoom_duration_factor * deltalod);
			}
			break;
		case 2: {
				auto tiledist = this->calculate_tiledist(this->layers[1], this->layers[2]);
				this->timer.start(config::control::fly_move_duration_factor * tiledist);
			}
			break;
		default:
			this->state = 0;
			break;
		}

		return this->state != 0;
	}

	float calculate_tiledist(const tilelayer &src, const tilelayer &dest) const
	{
		auto &srcnw = src.get_pixelnw();
		auto &destnw = dest.get_pixelnw();
		auto pixdistx = static_cast<double>(destnw.x - srcnw.x);
		auto pixdisty = static_cast<double>(destnw.y - srcnw.y);
		double tiledist = std::sqrt(pixdistx * pixdistx + pixdisty * pixdisty) / dest.get_tilesize();
		return static_cast<float>(tiledist);
	}
};

class icons
{
	enum iconkind
	{
		GPS_STABLE = 0,
		GPS_UNSTABLE = 1,
		MARK = 2,
	};

public:
	bool load(const std::string &rootdir)
	{
		auto icondir = path::combine(rootdir, "icons");

#define LOADTEX(fname, hotx, hoty) \
		{ \
			auto tex = pngtexture::load(nullptr, path::combine(icondir, fname)); \
			if (!tex) return false; \
			std::unique_ptr<pngtexture> t(tex); \
			t->bind(); \
			this->textures.push_back(std::make_pair(t.release(), screenpair(hotx, hoty))); \
		}
		LOADTEX("gps_stable.png", 8, 8)
		LOADTEX("gps_unstable.png", 8, 8)
		LOADTEX("mark.png", 16, 28)
#undef LOADTEX
		return true;
	}

#define GETTER(propname, i) \
	std::pair<const pngtexture *, screenpair> propname() const \
	{ \
		auto &ret = this->textures[i]; \
		return std::make_pair(ret.first.get(), ret.second); \
	}

	GETTER(gps_stable, GPS_STABLE)
	GETTER(gps_unstable, GPS_UNSTABLE)
	GETTER(mark, MARK)
#undef GETTER

private:
	std::vector<std::pair<std::unique_ptr<pngtexture>, screenpair>> textures;
};

class shape
{
public:
#ifdef PLATFORM_CLR
	shape(Map::IShape^ ui)
		: invalidated(true), uictrl(ui)
	{
	}
	shape()
		: invalidated(true), uictrl(nullptr)
	{
	}
#else
	shape()
		: invalidated(true)
	{
	}
#endif

#ifdef PLATFORM_CLR
	Map::IShape^ get_uictrl()
	{
		return this->uictrl;
	}
#endif

	bool is_invalidated() const
	{
		return this->invalidated;
	}

	virtual void invalidate()
	{
		this->invalidated = true;

		this->hide();
	}

	virtual bool is_visible(const tilelayer &layer) const = 0;

	virtual void show(const screenpair &screen)
	{
#ifdef PLATFORM_CLR
		auto trans = gcnew TranslateTransform(screen.x, screen.y);
		this->uictrl->Transform = trans;

		if (!this->uictrl->Visible)
			this->uictrl->Visible = true;
#endif
	}

	virtual void hide()
	{
#ifdef PLATFORM_CLR
		if (this->uictrl->Visible)
			this->uictrl->Visible = false;
#endif
	}

protected:
	bool invalidated;
#ifdef PLATFORM_CLR
	gcroot<Map::IShape^> uictrl;
#endif
};

class pin : public shape
{
public:
#ifdef PLATFORM_CLR
	pin(const lonlat &ll)
		: location(ll), uilocctrl(nullptr)
	{
	}

	pin(const lonlat &ll, Map::ILocation^ ui)
		: shape(ui), location(ll), uilocctrl(ui)
	{
	}
#else
	pin(const lonlat &ll)
		: location(ll)
	{
	}
#endif

	const lonlat & get_location() const
	{
		return this->location;
	}

#ifdef IMPLEMENT_PIN
	void set_position(const pixelpair &pix)
	{
		this->uilocctrl->SetPixelPair(pix.x, pix.y);

		this->invalidated = false;
	}
#endif

	virtual bool is_visible(const tilelayer &layer) const
	{
		auto pix = layer.get_pixel(this->location);
		return layer.is_visible(pix);
	}

protected:
	lonlat location;
#ifdef PLATFORM_CLR
	gcroot<Map::ILocation^> uilocctrl;
#endif
};

class dynamic_pin : public pin
{
public:
	dynamic_pin()
		: pin(lonlat(0.0, 0.0)), stable(false)
	{
	}

	dynamic_pin(const lonlat &ll)
		: pin(ll), stable(true)
	{
	}

	void validate(const lonlat &ll)
	{
		this->location = ll;
		this->stable = true;
	}

	void invalidate()
	{
		this->stable = false;
	}

	bool is_stable() const
	{
		return this->stable;
	}

protected:
	bool stable;
};

class gps_pin : public dynamic_pin
{
public:
	gps_pin()
		: radius(0.0f)
	{
	}

	void set(const lonlat &ll, float radius)
	{
		this->validate(ll);
		this->radius = radius;
	}

	void unknown()
	{
		this->invalidate();
	}

	float get_radius() const
	{
		return this->radius;
	}

private:
	float radius;
};

#ifdef IMPLEMENT_TRAIL
class trail : public shape
{
public:
	trail(const std::vector<lonlat> &locations, Map::ILocationPath^ ui)
		: shape(ui), range(std::make_pair(false, std::make_pair(pixelpair(0, 0), pixelpair(0,0)))), locations(locations), uilocctrl(ui)
	{
	}

	const std::vector<lonlat> & get_locations() const
	{
		return this->locations;
	}

#ifdef IMPLEMENT_TRAIL
	virtual void invalidate()
	{
		shape::invalidate();

		this->range.first = false;
	}

	void start_over()
	{
		this->uilocctrl->StartOver(this->locations.size());
	}

	void add_position(const pixelpair &pix)
	{
		this->uilocctrl->AddPixelPair(pix.x, pix.y);

		auto &nw = this->range.second.first;
		auto &se = this->range.second.second;

		if (!this->range.first) {
			nw = pix;
			se = pix;
			this->range.first = true;
		}
		else {
			if (pix.y < nw.y) nw.y = pix.y;
			if (pix.y > se.y) se.y = pix.y;

			if (pix.x < nw.x) nw.x = pix.x;
			if (pix.x > se.x) se.x = pix.x;
		}
	}

	void end_point()
	{
		this->uilocctrl->EndPoint();

		this->invalidated = false;
	}

	virtual bool is_visible(const tilelayer &layer) const
	{
		if (!this->range.first) return false;

		auto &trailnw = this->range.second.first;
		auto &trailse = this->range.second.second;

		auto &layernw = layer.get_pixelnw();
		auto &layerse = layer.get_pixelse();

		if (trailse.x < layernw.x || trailnw.x > layerse.x) return false;
		if (trailse.y < layernw.y || trailnw.y > layerse.y) return false;
		return true;
	}
#endif

private:
	std::pair<bool, std::pair<pixelpair, pixelpair>> range;
	std::vector<lonlat> locations;
#ifdef PLATFORM_CLR
	gcroot<Map::ILocationPath^> uilocctrl;
#endif
};
#endif

class mapcontrol_impl : public mapcontrol
{
public:
	mapcontrol_impl(const std::string &reposroot, int zoomlevel, const screenpair &size, unsigned int numtileslimit)
		: mainlayer(zoomlevel), sublayer(0), size(size), timestamp(1), dirty(true), showtilekeys(false)
	{
		this->tiles.reset(new tilecache(reposroot, this->mainlayer.get_tilesize(), numtileslimit));

		auto tilesize = static_cast<short>(this->mainlayer.get_tilesize());
		this->render.reset(renderer::create(tilesize));

		this->gpspin.reset(new gps_pin());
	}

	virtual bool initialize(const std::string &reposroot, tileupdatehandler handler)
	{
		if (!this->iconset.load(reposroot)) return false;
		if (!this->tiles->initialize(handler)) return false;

		return true;
	}

#ifdef USE_SDL
	virtual void handle_message(const SDL_Event &evt)
	{
		bool moving = this->move.second.is_active();
		bool zooming = this->zoom.second.is_active();

		switch (evt.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (!zooming) {
				if (evt.motion.which == 0 && evt.button.button == SDL_BUTTON_LEFT)
					this->move.second.press();
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (evt.motion.which == 0 && evt.button.button == SDL_BUTTON_LEFT)
				this->move.second.release();
			break;
		case SDL_MOUSEMOTION:
			if (this->move.second.is_tracking())
				this->keep_moving(evt.motion.xrel, evt.motion.yrel);
			break;
		case SDL_KEYDOWN:
			switch (evt.key.keysym.sym) {
			case SDLK_w:
				if (!moving) this->begin_zoom(1);
				break;
			case SDLK_s:
				if (!moving) this->begin_zoom(-1);
				break;
			case SDLK_z:
				if (!moving && !zooming) this->set_mode(mapcontrol::ROAD);
				break;
			case SDLK_x:
				if (!moving && !zooming) this->set_mode(mapcontrol::HYBRID);
				break;
			case SDLK_r:
				this->dirty = true;
				break;
#ifdef LOGGING_QUADTRIE
			case SDLK_d:
				this->tiles->dump_trie(this->timestamp);
				break;
#endif
			case SDLK_e:
				this->showtilekeys = true;
				break;
			case SDLK_c:
				this->tiles->sweep(this->timestamp);
				break;
			}
			break;
		}
	}
#elif defined PLATFORM_IOS
	virtual void move_map(int deltax, int deltay)
	{
		this->move.press();
		this->keep_moving(deltax * 10, deltay * 10);
		this->move.release();
	}
#endif

#ifdef PLATFORM_CLR
	virtual renderer * get_renderer()
	{
		return this->render.get();
	}

	virtual void tick()
	{
		if (this->move.second.is_active()) {
			this->update_pins();
			this->update_trails();
		}
	}

	virtual void handle_mousepress()
	{
		bool zooming = this->zoom.second.is_active();
		if (!zooming)
			this->move.second.press();
	}

	virtual void handle_mousemoving(short deltax, short deltay)
	{
		if (this->move.second.is_tracking())
			this->keep_moving(deltax, deltay);
	}

	virtual void handle_mouserelease()
	{
		this->move.second.release();
	}

	virtual void handle_zooming(int delta)
	{
		bool moving = this->move.second.is_active();
		if (!moving)
			this->begin_zoom(delta);
	}
#endif

	virtual void draw()
	{
		this->tiles->clear_dirty();

		this->timestamp++;
		this->tiles->destroy_useless_textures();
		bool complete = this->draw_layers();

#ifdef IMPLEMENT_GPSPIN
		if (!this->zoom.second.is_active())
			this->draw_gpspin();
#endif

		this->dirty = !complete;
	}

	virtual bool needs_draw() const
	{
		if (this->dirty) return true;
		if (this->tiles->is_dirty()) return true;

		return false;
	}

	virtual void set_screensize(const mapctrl::screenpair &newsize)
	{
		auto oldsize = this->size;

		auto nw = this->mainlayer.get_pixelnw();
		auto se = this->mainlayer.get_pixelse();

		this->size = newsize;

		pixelpair pixcenter((nw.x + se.x) / 2, (nw.y + se.y) / 2);
		this->mainlayer.set_map(this->mainlayer.get_zoomlevel(), pixcenter, this->size);

		this->dirty = true;
	}

	virtual void set_center(const lonlat &ll)
	{
		auto pixel = this->mainlayer.get_pixel(ll);
		this->mainlayer.set_map(this->mainlayer.get_zoomlevel(), pixel, this->size);

		this->dirty = true;
	}

	virtual void set_gps(const lonlat *ll, float radius)
	{
		if (ll)
			this->gpspin->set(*ll, radius);
		else
			this->gpspin->unknown();

		this->dirty = true;
	}

	virtual void set_mode(mapcontrol::mapstyle style)
	{
		this->tiles->set_mode(style);

		this->dirty = true;
	}

	virtual void set_zoomlevel(int level)
	{
		auto current = this->mainlayer.get_zoomlevel();
		this->set_zoomdelta(level - current);
	}

	virtual void set_zoomdelta(int delta)
	{
		this->begin_zoom(delta);
	}

	virtual lonlat get_center() const
	{
		auto nw = this->mainlayer.get_pixelnw();
		auto se = this->mainlayer.get_pixelse();
		pixelpair pixcenter((nw.x + se.x) / 2, (nw.y + se.y) / 2);

		return this->mainlayer.get_lonlat(pixcenter);
	}

	virtual const lonlat * get_gps() const
	{
		if (this->gpspin->is_stable())
			return &this->gpspin->get_location();
		return nullptr;
	}

	virtual mapstyle get_mode() const
	{
		return this->tiles->get_mode();
	}

	virtual int get_zoomlevel() const
	{
		return this->mainlayer.get_zoomlevel();
	}

#ifdef PLATFORM_CLR
	virtual long add_pin(const lonlat &location, Map::ILocation^ ui)
	{
		std::unique_ptr<pin> newpin(new pin(location, ui));
		this->update_pin(*newpin);

		auto id = reinterpret_cast<long>(newpin.get());
		this->pins.insert(std::make_pair(id, newpin.release()));
		return id;
	}

	virtual Map::ILocation^ remove_pin(long id)
	{
		auto pin = this->pins.find(id);
		if (pin == this->pins.end()) return nullptr;

		auto uictrl = dynamic_cast<Map::ILocation^>(pin->second->get_uictrl());

		this->pins.erase(pin);
		return uictrl;
	}

	virtual long add_trail(const std::vector<lonlat> &locations, Map::ILocationPath^ ui)
	{
		std::unique_ptr<trail> newtrail(new trail(locations, ui));
		this->update_trail(*newtrail);

		auto id = reinterpret_cast<long>(newtrail.get());
		this->trails.insert(std::make_pair(id, newtrail.release()));
		return id;
	}

	virtual Map::ILocationPath^ remove_trail(long id)
	{
		auto trail = this->trails.find(id);
		if (trail == this->trails.end()) return nullptr;

		auto uictrl = dynamic_cast<Map::ILocationPath^>(trail->second->get_uictrl());

		this->trails.erase(trail);
		return uictrl;
	}

	virtual std::pair<int, lonlat> get_optimal_map(const std::vector<lonlat> &locations)
	{
		std::pair<double, double> lonrange = this->mainlayer.get_lon_range();
		std::pair<double, double> latrange = this->mainlayer.get_lat_range();
		lonlat minrange(lonrange.second, latrange.second);
		lonlat maxrange(lonrange.first, latrange.first);

		for (auto i = locations.begin(); i != locations.end(); ++i) {
			if (i->lon < minrange.lon) minrange.lon = i->lon;
			if (maxrange.lon < i->lon) maxrange.lon = i->lon;
			if (i->lat < minrange.lat) minrange.lat = i->lat;
			if (maxrange.lat < i->lat) maxrange.lat = i->lat;
		}
		lonlat center((minrange.lon + maxrange.lon) / 2, (minrange.lat + maxrange.lat) / 2);

		double londist = std::max<double>(std::abs(minrange.lon - center.lon), std::abs(maxrange.lon - center.lon));
		double latdist = std::max<double>(std::abs(minrange.lat - center.lat), std::abs(maxrange.lat - center.lat));
		auto zoomlevel = this->mainlayer.get_zoomlevel(center, londist, latdist);

		return std::make_pair(zoomlevel, center);
	}

	virtual void animate_to(int destlod, const lonlat &destll)
	{
		this->fly.second.start(this->mainlayer, destlod, destll, this->size);
	}
#endif

private:
	tilelayer mainlayer;
	tilelayer sublayer;
	std::unique_ptr<tilecache> tiles;
	std::unique_ptr<renderer> render;
	screenpair size;
	icons iconset;
	std::pair<bool, moving> move;
	std::pair<bool, zooming> zoom;
	std::pair<bool, flying> fly;
	int timestamp;
	bool dirty;
	bool showtilekeys;
	std::unique_ptr<gps_pin> gpspin;
#ifdef IMPLEMENT_PIN
	std::map<long, std::unique_ptr<pin>> pins;
#endif
#ifdef IMPLEMENT_TRAIL
	std::map<long, std::unique_ptr<trail>> trails;
#endif

	void begin_zoom(int delta)
	{
		this->zoom.second.start();

		this->sublayer = this->mainlayer;
		this->mainlayer.zoom_map(delta);

		this->dirty = true;
	}

	void keep_moving(short xrel, short yrel)
	{
		this->move.second.move(xrel, yrel);

		this->dirty = true;
	}

	void trycall_eventhandler(std::pair<bool, moving> &move, bool active)
	{
		if (active && !move.first) {
			move.first = true;
			this->on_movebegin();
		}
		if (!active && move.first) {
			move.first = false;
			this->on_moveend();
		}
	}

	void trycall_eventhandler(std::pair<bool, zooming> &zoom, bool active)
	{
		if (active && !zoom.first) {
			zoom.first = true;
			this->on_zoombegin();
		}
		if (!active && zoom.first) {
			zoom.first = false;
			this->on_zoomend();
		}
	}

	void trycall_eventhandler(std::pair<bool, flying> &fly, bool active)
	{
		if (active && !fly.first) {
			fly.first = true;
			this->on_flybegin();
		}
		if (!active && fly.first) {
			fly.first = false;
			this->on_flyend();
		}
	}

	bool draw_layers()
	{
		if (this->fly.second.is_active()) {
			this->trycall_eventhandler(fly, true);

			if (this->fly.second.tick()) {
				if (this->fly.second.is_zooming()) {
					auto progress = this->fly.second.zooming(this->mainlayer, this->sublayer);
					this->draw_zoominglayers(progress, this->mainlayer, this->sublayer);
				}
				else {
					this->fly.second.moving(this->mainlayer);
					this->draw_layer(this->mainlayer, nullptr, true);
				}
			}
		}
		else if (this->move.second.is_active()) {
			auto delta = this->move.second.get_delta();

			if (delta.x && delta.y) {
				this->trycall_eventhandler(move, true);

				this->mainlayer.move_map(delta);
			}

			this->draw_layer(this->mainlayer, nullptr, true);
		}
		else if (this->zoom.second.is_active()) {
			this->trycall_eventhandler(zoom, true);

			auto progress = this->zoom.second.smooth<float>(0, 1);
			this->draw_zoominglayers(progress, this->mainlayer, this->sublayer);
		}
		else {
			this->trycall_eventhandler(move, false);
			this->trycall_eventhandler(zoom, false);
			this->trycall_eventhandler(fly, false);

			this->draw_layer(this->mainlayer, nullptr, true);
			return true;
		}

		return false;
	}

	void draw_zoominglayers(float progress, const tilelayer &destlayer, const tilelayer &srclayer)
	{
		int difflevel = destlayer.get_zoomlevel() - srclayer.get_zoomlevel();
		float scale = std::pow(2.f, difflevel);
		float subtilefactor = std::pow(scale, progress);
		float maintilefactor = std::pow(scale, progress - 1);

		const float showdetailedafter = 0.8f;
		const float showdetaileduntil = 0.08f;
		if (difflevel > 0) {
			this->draw_layer(srclayer, &subtilefactor, true);
			this->draw_layer(destlayer, &maintilefactor, progress > showdetailedafter);
		}
		else {
			this->draw_layer(destlayer, &maintilefactor, true);
			this->draw_layer(srclayer, &subtilefactor, progress < showdetaileduntil);
		}
	}

	void draw_layer(const tilelayer &layer, const float *factor, bool draw)
	{
		if (this->showtilekeys) {
#ifdef LOGGING_VISIBLETILES
			logger::info(" === VISIBLE TILES === ");
#endif
		}

		for (auto i = layer.visible(); i.movenext(); ) {
			auto tile = i.currenttile();
			auto pixel = i.currentpixel();
			this->render->draw_tile(this->timestamp, this->tiles.get(), this->size, layer, factor, draw, tile, pixel);
		}

		this->showtilekeys = false;
	}

#ifdef IMPLEMENT_PIN
	void update_pins()
	{
		for (auto i = this->pins.begin(); i != this->pins.end(); ++i) {
			auto pin = i->second.get();
			this->update_pin(*pin);
		}
	}

	void update_pin(pin &pin)
	{
		if (pin.is_invalidated()) {
			auto pixel = this->mainlayer.get_pixel(pin.get_location());
			pin.set_position(pixel);
		}

		if (pin.is_visible(this->mainlayer)) {
			pixelpair pixzero(0, 0);
			auto screen = this->mainlayer.get_screen(pixzero);
			pin.show(screen);
		}
		else
			pin.hide();
	}

	void invalidate_pins()
	{
		for (auto i = this->pins.begin(); i != this->pins.end(); ++i) {
			auto pin = i->second.get();
			pin->invalidate();
		}
	}
#endif

#ifdef IMPLEMENT_GPSPIN
	void draw_gpspin()
	{
		auto &gpsloc = this->gpspin->get_location();
		auto pixel = this->mainlayer.get_pixel(gpsloc);
		if (!this->mainlayer.is_visible(pixel))
			return;

		auto screen = this->mainlayer.get_screen(pixel);
		bool stable = this->gpspin->is_stable();

		if (stable) {
			float radius = 0.0f;
			auto accuracy = this->gpspin->get_radius();
			double resolution = this->mainlayer.get_resolution(gpsloc);
			radius = static_cast<float>(accuracy / resolution);

			if (radius > 5.0f)
				this->render->draw_gpspin_shadow(screen, radius);
		}

		auto texture = stable ? this->iconset.gps_stable() : this->iconset.gps_unstable();
		this->render->draw_gpspin(screen, texture.first, texture.second);
	}
#endif

#ifdef IMPLEMENT_TRAIL
	void update_trails()
	{
		for (auto i = this->trails.begin(); i != this->trails.end(); ++i) {
			auto trail = i->second.get();
			this->update_trail(*trail);
		}
	}

	void update_trail(trail &trail)
	{
		if (trail.is_invalidated()) {
			trail.start_over();
			for (auto j = trail.get_locations().begin(); j != trail.get_locations().end(); ++j) {
				auto pixel = this->mainlayer.get_pixel(*j);
				trail.add_position(pixel);
			}
			trail.end_point();
		}

		if (trail.is_visible(this->mainlayer)) {
			pixelpair pixzero(0, 0);
			auto screen = this->mainlayer.get_screen(pixzero);
			trail.show(screen);
		}
		else
			trail.hide();
	}

	void invalidate_trails()
	{
		for (auto i = this->trails.begin(); i != this->trails.end(); ++i) {
			auto trail = i->second.get();
			trail->invalidate();
		}
	}
#endif

	void on_movebegin()
	{
	}

	void on_moveend()
	{
#ifdef IMPLEMENT_PIN
		this->update_pins();
#endif
#ifdef IMPLEMENT_TRAIL
		this->update_trails();
#endif
	}

	void on_zoombegin()
	{
#ifdef IMPLEMENT_PIN
		this->invalidate_pins();
#endif
#ifdef IMPLEMENT_TRAIL
		this->invalidate_trails();
#endif
	}

	void on_zoomend()
	{
#ifdef IMPLEMENT_PIN
		this->update_pins();
#endif
#ifdef IMPLEMENT_TRAIL
		this->update_trails();
#endif
	}

	void on_flybegin()
	{
#ifdef IMPLEMENT_PIN
		this->invalidate_pins();
#endif
#ifdef IMPLEMENT_TRAIL
		this->invalidate_trails();
#endif
	}

	void on_flyend()
	{
#ifdef IMPLEMENT_PIN
		this->update_pins();
#endif
#ifdef IMPLEMENT_TRAIL
		this->update_trails();
#endif
	}
};

mapcontrol * mapcontrol::create(const std::string &reposroot, int zoomlevel, const screenpair &size, unsigned int numtileslimit)
{
	return new mapcontrol_impl(reposroot, zoomlevel, size, numtileslimit);
}
