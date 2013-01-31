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
using namespace System;
using namespace System::Net;
using namespace System::IO;
using namespace msclr::interop;
#endif

repository::repository(const std::string &rootdir)
	: tilerootdir(path::combine(rootdir, "tiles")), combinefactor(4)
{
}

bool repository::exists(const quadkey &key, mapcontrol::mapstyle style) const
{
	auto abspath = this->get_absolutepath(key, style);
	return file::exists(abspath);
}

std::string repository::get_absolutepath(const quadkey &key, mapcontrol::mapstyle style) const
{
	auto relpath = this->get_relativepath(key, style);
	return path::combine(this->tilerootdir, relpath);
}

#ifdef IMPLEMENT_DOWNLOAD
std::string repository::get_url(const quadkey &key, mapcontrol::mapstyle style) const
{
	std::ostringstream s;
	s << "http://ecn.t0.tiles.virtualearth.net/tiles/";

	switch (style) {
	case mapcontrol::ROAD:
		s << 'r';
		s << key.str();
		s << ".png?g=471&mkt=en-us&shading=hill";
		break;
	case mapcontrol::HYBRID:
		s << 'h';
		s << key.str();
		s << ".jpeg?g=471";
		break;
	}

	return s.str();
}

bool repository::prepare_path(const quadkey &key, mapcontrol::mapstyle style) const
{
	auto abspath = marshal_as<String^>(this->get_absolutepath(key, style));
	auto dir = Path::GetDirectoryName(abspath);
	Directory::CreateDirectory(dir);
	return true;
}
#endif

std::string repository::get_relativepath(const quadkey &key, mapcontrol::mapstyle style) const
{
	auto keystr = key.str();

	std::ostringstream s;
	for (unsigned int i = 0; i < keystr.length(); ++i) {
		if (i > 0 && (i % this->combinefactor == 0))
			s << path::separator;

		s << keystr[i];
	}

	s << '_';

	switch (style) {
	case mapcontrol::ROAD:
		s << "r.png_";
		break;
	case mapcontrol::HYBRID:
		s << "h.jpeg_";
		break;
	}

	return s.str();
}

pngtexture_queued::~pngtexture_queued()
{
#ifdef USE_OPENGL
	if (this->enclosing)
		this->enclosing->enqueue_useless_texture(this->tex);
#endif
}

tilecache::tilecache(const std::string &rootdir, unsigned int tilesize, unsigned int numtileslimit)
	: repos(rootdir), tilesize(tilesize), tilestyle(mapcontrol::ROAD), flagterminate(false), numtileslimit(numtileslimit), dirty(false), on_tileloaded(nullptr)
{
}

tilecache::~tilecache()
{
	if (this->worker.get()) {
		this->flagterminate = true;

		this->quemutex->lock();
		{
			this->quecond->signal();
		}
		this->quemutex->unlock();

		this->worker->waitjoin();
	}
}

#ifdef PLATFORM_CLR
static void tilecache_thread_entry(void *data)
{
	auto o = static_cast<tilecache *>(data);
	o->work();
}
#else
static int tilecache_thread_entry(void *data)
{
	auto o = static_cast<tilecache *>(data);
	o->work();
	return 0;
}
#endif

bool tilecache::initialize(tileloadedhandler handler)
{
	this->mapmutex.reset(mutex::create());
	this->quemutex.reset(mutex::create());
	this->quecond.reset(condvar::create());

	this->on_tileloaded = handler;
	this->worker.reset(thread::create(tilecache_thread_entry, this));
	return true;
}

void tilecache::work()
{
	this->quemutex->lock();

	while (!this->flagterminate) {
		this->quecond->wait(*this->quemutex);

		while (!this->requested.empty()) {
			auto victim = *this->requested.begin();
			this->requested.erase(victim.first);
			this->quemutex->unlock();

			auto path = this->repos.get_absolutepath(victim.first, this->tilestyle);
			std::unique_ptr<pngtexture> tex(pngtexture::load(this, path));

#ifdef IMPLEMENT_DOWNLOAD
			if (!tex.get() && victim.second)
				this->download(victim.first, this->tilestyle, victim.second);
#else
			if (!tex.get() && victim.second && victim.first.has_upper()) {
				this->quemutex->lock();
				{
					this->requested.insert(std::make_pair(victim.first.upper(), victim.second));
				}
				this->quemutex->unlock();
			}
#endif

			this->insert_tile(victim.first, victim.second, tex.get() ? tex.release() : nullptr);

			this->quemutex->lock();
		}
	}

	this->quemutex->unlock();
}

void tilecache::insert_tile(const quadkey &key, int timestamp, pngtexture *tex)
{
	this->mapmutex->lock();
	{
		if (this->tiletextures.get_numnodes() >= this->numtileslimit)
			this->tiletextures.sweep(timestamp);

		bool update = tex != nullptr;
		std::unique_ptr<pngtexture_queued> texqd;
		if (tex)
			texqd.reset(new pngtexture_queued(this, tex));

		this->tiletextures.insert(key, texqd.release());
		if (update) {
			this->dirty = true;
			if (this->on_tileloaded)
				this->on_tileloaded();
		}
	}
	this->mapmutex->unlock();
}

void tilecache::set_mode(mapcontrol::mapstyle style)
{
	this->tilestyle = style;
	this->clear();
}

mapcontrol::mapstyle tilecache::get_mode() const
{
	return this->tilestyle;
}

std::pair<mapctrl::quadkey, const pngtexture *> tilecache::get_texture(const quadkey &key, int timestamp)
{
	std::pair<mapctrl::quadkey, pngtexture *> result(mapctrl::quadkey::epsilon(), static_cast<pngtexture *>(0));

	this->mapmutex->lock();
	{
		std::pair<bool, std::pair<mapctrl::quadkey, pngtexture_queued *>> pair = this->tiletextures.search(key, timestamp);
		result.first = pair.second.first;
		if (pair.second.second)
			result.second = pair.second.second->get_tex();

		if (!pair.first) {
			this->quemutex->lock();
			{
				this->requested.insert(std::make_pair(key, timestamp));
				this->quecond->signal();
			}
			this->quemutex->unlock();
		}

		{
			auto texture = result.second;
			if (texture && !texture->is_bound())
				texture->bind();
		}
	}
	this->mapmutex->unlock();

	return result;
}

#ifdef IMPLEMENT_DOWNLOAD
ref class DownloaderArgument
{
public:
	tilecache *TileCache;
	String^ Key;
	mapcontrol::mapstyle Style;
	int Timestamp;
	String^ TargetPath;
};

static void client_OpenReadCompleted(Object^ sender, OpenReadCompletedEventArgs^ e)
{
	auto arg = safe_cast<DownloaderArgument^>(e->UserState);

	auto keystr = arg->Key;
	quadkey key(marshal_as<std::string>(keystr));

	arg->TileCache->prepare_path(key, arg->Style);

	bool successful = false;
	try {
		{
			auto writer = gcnew FileStream(arg->TargetPath, FileMode::CreateNew);
			auto buffer = gcnew array<System::Byte, 1>(4096);
			for (; ; ) {
				auto count = e->Result->Read(buffer, 0, buffer->Length);
				if (count <= 0) break;
				writer->Write(buffer, 0, count);
			}
			writer->Close();
		}

		successful = true;
	}
	catch (Exception^) {
	}
	finally {
		try {
			if (e->Result != nullptr)
				e->Result->Close();
		}
		catch (Exception^) {
			successful = false;
		}
	}

	if (!successful) {
		try {
			File::Delete(arg->TargetPath);
		}
		catch (Exception^) {
		}
	}

	auto targetpath = arg->TargetPath;
	auto targetpathu = marshal_as<std::string>(targetpath);
	std::unique_ptr<pngtexture> tex(pngtexture::load(nullptr, targetpathu));
	arg->TileCache->insert_tile(key, arg->Timestamp, tex.get() ? tex.release() : nullptr);
}

void tilecache::download(const quadkey &key, mapcontrol::mapstyle style, int timestamp)
{
	auto sourcepath = marshal_as<String^>(this->repos.get_url(key, style));
	auto sourceuri = gcnew Uri(sourcepath);

	auto arg = gcnew DownloaderArgument();
	arg->TileCache = this;
	arg->Key = marshal_as<String^>(key.str());
	arg->Style = style;
	arg->Timestamp = timestamp;
	arg->TargetPath = marshal_as<String^>(this->repos.get_absolutepath(key, style));

	auto client = gcnew WebClient();
	client->OpenReadCompleted += gcnew OpenReadCompletedEventHandler(client_OpenReadCompleted);
	client->OpenReadAsync(sourceuri, arg);
}

void tilecache::prepare_path(const quadkey &key, mapcontrol::mapstyle style)
{
	this->repos.prepare_path(key, style);
}
#endif


#ifdef USE_OPENGL
void tilecache::enqueue_useless_texture(pngtexture *tex)
{
	this->useless.push_back(std::unique_ptr<pngtexture>(tex));
}
#endif

void tilecache::destroy_useless_textures()
{
#ifdef USE_OPENGL
	this->mapmutex->lock();
	{
		this->useless.clear();
	}
	this->mapmutex->unlock();
#endif
}

void tilecache::sweep(int timestamp)
{
	this->mapmutex->lock();
	{
		this->tiletextures.sweep(timestamp);
	}
	this->mapmutex->unlock();
}

void tilecache::clear_dirty()
{
	this->dirty = false;
}

#ifdef LOGGING_QUADTRIE
void tilecache::dump_trie(int timestamp) const
{
	this->tiletextures.dump(timestamp);
}
#endif

void tilecache::clear()
{
	this->quemutex->lock();
	{
		decltype(this->requested) newqueue;
		this->requested.swap(newqueue);
	}
	this->quemutex->unlock();

	this->mapmutex->lock();
	{
		this->tiletextures.clear();
	}
	this->mapmutex->unlock();
}
