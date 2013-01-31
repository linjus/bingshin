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

#ifdef PLATFORM_CLR
using namespace System::Collections::Generic;
using namespace System::Windows;
using namespace System::Windows::Controls;
using namespace System::Windows::Media;

namespace Map
{
	public interface class IShape
	{
		property UIElement^ UI;
		property bool Visible;
		property TranslateTransform^ Transform;
	};

	public interface class ILocation : public IShape
	{
		void SetPixelPair(int x, int y);
	};

	public interface class ILocationPath : public IShape
	{
		void StartOver(int numlocations);
		void AddPixelPair(int x, int y);
		void EndPoint();
	};
}
#endif

namespace mapctrl
{

struct CONTROL_API progresstimer
{
	virtual void start(float duration) = 0;
	virtual void stop() = 0;
	virtual float get_progress() = 0;
	virtual float get_elapsed() const = 0;
	virtual bool is_running() const = 0;

	static progresstimer * create();
};

struct CONTROL_API mutex
{
	virtual ~mutex()
	{
	}

	virtual void lock() = 0;
	virtual void unlock() = 0;

#if defined PLATFORM_CLR
	virtual CRITICAL_SECTION * get_mutex() = 0;
#elif defined PLATFORM_IOS
    virtual pthread_mutex_t * get_mutex() = 0;
#else
	virtual SDL_mutex * get_mutex() = 0;
#endif

	static mutex * create();
};

struct CONTROL_API condvar
{
    virtual ~condvar()
    {
    }
    
    virtual void signal() = 0;
    virtual void wait(mutex &mut) = 0;
        
    static condvar * create();
};

struct CONTROL_API thread
{
	virtual ~thread()
	{
	}

	virtual void waitjoin() = 0;

#ifdef PLATFORM_CLR
	static thread * create(void (*entry)(void *data), void *data);
#else
	static thread * create(int (*entry)(void *data), void *data);
#endif
};

struct CONTROL_API mapcontrol
{
	enum mapstyle
	{
		HYBRID = 1,
		ROAD = 2,
	};

	typedef void (*tileupdatehandler)();

	virtual ~mapcontrol()
	{
	}

	virtual bool initialize(const std::string &reposroot, tileupdatehandler handler) = 0;

#ifdef USE_SDL
	virtual void handle_message(const SDL_Event &evt) = 0;
#elif defined PLATFORM_IOS
	virtual void move_map(int deltax, int deltay) = 0;
#endif

#ifdef PLATFORM_CLR
	virtual renderer * get_renderer() = 0;

	virtual void tick() = 0;

	virtual void handle_mousepress() = 0;
	virtual void handle_mousemoving(short deltax, short deltay) = 0;
	virtual void handle_mouserelease() = 0;
	virtual void handle_zooming(int delta) = 0;
#endif

	virtual void draw() = 0;
	virtual bool needs_draw() const = 0;

	virtual void set_screensize(const screenpair &size) = 0;
	virtual void set_center(const lonlat &ll) = 0;
	virtual void set_gps(const lonlat *ll, float radius) = 0;
	virtual void set_mode(mapstyle style) = 0;
	virtual void set_zoomlevel(int level) = 0;
	virtual void set_zoomdelta(int delta) = 0;

	virtual lonlat get_center() const = 0;
	virtual const lonlat * get_gps() const = 0;
	virtual mapstyle get_mode() const = 0;
	virtual int get_zoomlevel() const = 0;

#ifdef PLATFORM_CLR
	virtual long add_pin(const lonlat &location, Map::ILocation^ ui) = 0;
	virtual Map::ILocation^ remove_pin(long id) = 0;
	virtual long add_trail(const std::vector<lonlat> &locations, Map::ILocationPath^ ui) = 0;
	virtual Map::ILocationPath^ remove_trail(long id) = 0;

	virtual std::pair<int, lonlat> get_optimal_map(const std::vector<lonlat> &locations) = 0;
	virtual void animate_to(int destlod, const lonlat &destll) = 0;
#endif

	static mapcontrol * create(const std::string &reposroot, int zoomlevel, const screenpair &size, unsigned int numtileslimit);
};

}
