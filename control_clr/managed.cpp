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

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows;
using namespace System::Windows::Controls;
using namespace System::Windows::Media;
using namespace System::Windows::Input;
using namespace msclr::interop;

namespace Map
{

public enum class MapStyle
{
	HYBRID = 1,
	ROAD = 2,
};

static void on_tileupdate()
{
}

public ref class MapControl : public UserControl
{
public:
	MapControl()
		: map(nullptr)
	{
		this->ClipToBounds = true;

		this->eventtarget = gcnew Canvas();
		this->eventtarget->Background = Brushes::Transparent;
		this->AddChild(this->eventtarget);

		this->Loaded += gcnew RoutedEventHandler(this, &MapControl::MapControl_Loaded);
		this->SizeChanged += gcnew SizeChangedEventHandler(this, &MapControl::MapControl_SizeChanged);

		eventtarget->MouseLeftButtonDown += gcnew MouseButtonEventHandler(this, &MapControl::MapControl_MouseLeftButtonDown);
		eventtarget->MouseLeftButtonUp += gcnew MouseButtonEventHandler(this, &MapControl::MapControl_MouseLeftButtonUp);
		eventtarget->MouseMove += gcnew MouseEventHandler(this, &MapControl::MapControl_MouseMove);
		eventtarget->MouseWheel += gcnew MouseWheelEventHandler(this, &MapControl::MapControl_MouseWheel);

		self = this;
	}

	void Initialize(String^ reposroot, int zoomlevel, int width, int height, double lon, double lat, int numtileslimit)
	{
		auto reposrootu = marshal_as<std::string>(reposroot);

		mapctrl::screenpair mapsize(width, height);
		this->map = mapctrl::mapcontrol::create(reposrootu, zoomlevel, mapsize, numtileslimit);

		if (!this->map->initialize(reposrootu, on_tileupdate)) {
			delete this->map;
			this->map = nullptr;
			return;
		}

		mapctrl::lonlat ll(lon, lat);
		this->map->set_center(ll);
	}

	property KeyValuePair<double, double> Center
	{
		KeyValuePair<double, double> get()
		{
			mapctrl::lonlat ll = this->map->get_center();
			return KeyValuePair<double, double>(ll.lon, ll.lat);
		}

		void set(KeyValuePair<double, double> value)
		{
			this->map->set_center(mapctrl::lonlat(value.Key, value.Value));
		}
	}

	property int ZoomLevel
	{
		int get()
		{
			return this->map->get_zoomlevel();
		}

		void set(int value)
		{
			this->map->set_zoomlevel(value);
		}
	}

	property MapStyle ViewMode
	{
		MapStyle get()
		{
			return static_cast<MapStyle>(this->map->get_mode());
		}

		void set(MapStyle value)
		{
			this->map->set_mode(static_cast<mapctrl::mapcontrol::mapstyle>(value));
		}
	}

	long AddPin(double lon, double lat, ILocation^ ctrl)
	{
		mapctrl::lonlat ll(lon, lat);
		long id = this->map->add_pin(ll, ctrl);

		auto uielem = dynamic_cast<UIElement^>(ctrl);
		if (uielem != nullptr)
			this->eventtarget->Children->Add(uielem);

		return id;
	}

	ILocation^ RemovePin(long id)
	{
		auto ctrl = this->map->remove_pin(id);
		if (ctrl == nullptr) return nullptr;

		auto uielem = ctrl->UI;
		if (uielem != nullptr)
			this->eventtarget->Children->Remove(uielem);

		return ctrl;
	}

	long AddTrail(List<KeyValuePair<double, double>>^ locations, ILocationPath^ ctrl)
	{
		std::vector<mapctrl::lonlat> lonlats;
		lonlats.reserve(locations->Count);

		for each (KeyValuePair<double, double> loc in locations) {
			mapctrl::lonlat ll(loc.Key, loc.Value);
			lonlats.push_back(ll);
		}

		long id = this->map->add_trail(lonlats, ctrl);

		if (ctrl->UI != nullptr)
			this->eventtarget->Children->Add(ctrl->UI);

		return id;
	}

	ILocationPath^ RemoveTrail(long id)
	{
		auto ctrl = this->map->remove_trail(id);
		if (ctrl == nullptr) return nullptr;

		auto uielem = ctrl->UI;
		if (uielem != nullptr)
			this->eventtarget->Children->Remove(uielem);

		return ctrl;
	}

	KeyValuePair<int, KeyValuePair<double, double>> GetOptimalMap(List<KeyValuePair<double, double>>^ locations)
	{
		std::vector<mapctrl::lonlat> lonlats;
		lonlats.reserve(locations->Count);

		for each (KeyValuePair<double, double> loc in locations) {
			mapctrl::lonlat ll(loc.Key, loc.Value);
			lonlats.push_back(ll);
		}

		std::pair<int, mapctrl::lonlat> optimal = this->map->get_optimal_map(lonlats);
		return KeyValuePair<int, KeyValuePair<double, double>>(optimal.first, KeyValuePair<double, double>(optimal.second.lon, optimal.second.lat));
	}

	void AnimateTo(int zoomlevel, KeyValuePair<double, double> lonlat)
	{
		mapctrl::lonlat ll(lonlat.Key, lonlat.Value);

		this->map->animate_to(zoomlevel, ll);
	}

	~MapControl()
	{
		delete this->map;
	}

protected:
	property Canvas^ ContentGrid
	{
		Canvas^ get()
		{
			return this->eventtarget;
		}
	}

	virtual void OnRender(DrawingContext^ dc) override
	{
		if (!this->map) return;

		this->map->tick();

		auto render = this->map->get_renderer();

		render_context ctx;
		ctx.valid = true;
		ctx.drawctx = dc;
		render->set_context(ctx);

		this->map->draw();

		ctx.valid = false;
		render->set_context(ctx);

		UserControl::OnRender(dc);
	}

private:
	mapctrl::mapcontrol *map;
	Canvas^ eventtarget;
	Point lastmousepos;
	static MapControl^ self;

	static void OnCompositionTargetRendering(Object^ sender, EventArgs^ args)
	{
		auto mapinst = self;
		if (!mapinst) return;
		mapinst->InvalidateVisual();
	}

	void MapControl_Loaded(Object^ sender, RoutedEventArgs^ e)
	{
		CompositionTarget::Rendering += gcnew EventHandler(&OnCompositionTargetRendering);
	}

	void MapControl_SizeChanged(Object^ sender, SizeChangedEventArgs^ e)
	{
		auto width = static_cast<int>(e->NewSize.Width);
		auto height = static_cast<int>(e->NewSize.Height);

		mapctrl::screenpair mapsize(width, height);
		this->map->set_screensize(mapsize);
	}

	void MapControl_MouseMove(Object^ sender, MouseEventArgs^ e)
	{
		auto ctrl = safe_cast<UIElement ^>(sender);
		if (ctrl->IsMouseCaptured) {
			auto cur = e->GetPosition(ctrl);
			auto deltax = static_cast<short>(cur.X - this->lastmousepos.X);
			auto deltay = static_cast<short>(cur.Y - this->lastmousepos.Y);
			this->lastmousepos = cur;

			this->map->handle_mousemoving(deltax, deltay);
		}
	}

	void MapControl_MouseLeftButtonUp(Object^ sender, MouseButtonEventArgs^ e)
	{
		auto ctrl = safe_cast<UIElement ^>(sender);
		ctrl->ReleaseMouseCapture();

		this->map->handle_mouserelease();
	}

	void MapControl_MouseLeftButtonDown(Object^ sender, MouseButtonEventArgs^ e)
	{
		auto ctrl = safe_cast<UIElement ^>(sender);
		bool result = ctrl->CaptureMouse();
		this->lastmousepos = e->GetPosition(ctrl);

		this->map->handle_mousepress();
	}

	void MapControl_MouseWheel(Object^ sender, MouseWheelEventArgs^ e)
	{
		int delta = e->Delta / 20;

		if (delta)
			this->map->handle_zooming(delta);
	}
};

}
