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

namespace config
{
	static bool showwireframe = false;
}

#ifdef PLATFORM_CLR
using namespace System;
using namespace System::IO;
using namespace System::Windows;
using namespace System::Windows::Media;
using namespace System::Windows::Media::Imaging;
#endif

#ifdef PLATFORM_CLR
class wpf_pngtexture : public pngtexture
{
public:
	static wpf_pngtexture * load(tilecache *enclosing, const std::string &path)
	{
		auto pathm = gcnew System::String(path.c_str());
		if (!System::IO::File::Exists(pathm)) return nullptr;

		return new wpf_pngtexture(pathm);
	}

	virtual ~wpf_pngtexture()
	{
	}

	virtual bool is_bound() const
	{
		return this->bound;
	}

	virtual void bind()
	{
		if (this->is_bound()) return;

//		auto uri = gcnew System::Uri(this->imgpath.get());
//		this->imgsrc = gcnew BitmapImage(uri);

		auto bitmap = gcnew BitmapImage();
		bitmap->BeginInit();
		bitmap->StreamSource = this->imgbuffer.get();
		bitmap->EndInit();

		this->imgsrc = bitmap;
		this->bound = true;
	}

	virtual ImageSource^ get_tex() const
	{
		return this->imgsrc.get();
	}

private:
	msclr::auto_gcroot<MemoryStream^> imgbuffer;
	bool bound;
	msclr::auto_gcroot<ImageSource^> imgsrc;

	wpf_pngtexture(String^ imgpath)
		: bound(false)
	{
		auto bytes = File::ReadAllBytes(imgpath);

		this->imgbuffer = gcnew MemoryStream(bytes);
	}
};
#elif defined USE_OPENGL
class opengl_pngtexture : public pngtexture
{
public:
	static opengl_pngtexture * load(tilecache *enclosing, const std::string &path)
	{
#ifdef USE_SDL
		SDL_Surface *raw = IMG_Load(path.c_str());
		if (!raw) return nullptr;

		int width = raw->w;
		int height = raw->h;

		SDL_Surface *converted = nullptr;

		GLint mode = GL_RGB;
		if (raw->format->BytesPerPixel == 4)
			mode = GL_RGBA;
		else if (raw->format->BytesPerPixel == 1) {
			converted = SDL_CreateRGBSurface(0, width, height, 24, 0x0000ff, 0x00ff00, 0xff0000, 0);
			SDL_BlitSurface(raw, 0, converted, 0);
		}

		std::unique_ptr<preparation> prepared(new preparation());
		prepared->rawsurface = raw;
		prepared->cvtsurface = converted;
		prepared->mode = mode;
		prepared->width = width;
		prepared->height = height;
		return new opengl_pngtexture(enclosing, prepared.release());
#elif defined PLATFORM_IOS
		GLuint width, height;
		void *imageData = read_texture(path, &width, &height);
		if (!imageData) return nullptr;
    
		std::auto_ptr<preparation> prepared(new preparation());
		prepared->rawdata = imageData;
		prepared->mode = GL_RGBA;
		prepared->width = width;
		prepared->height = height;
		return new opengl_pngtexture(enclosing, prepared.release());
#else
		return nullptr;
#endif
	}

	virtual ~opengl_pngtexture()
	{
		if (this->gltex) {
#ifdef LOGGING_TEXTURE
			logger::info("preparing destructing texture", this->gltex);
#endif
			glDeleteTextures(1, &this->gltex);
		}
	}

	virtual bool is_bound() const
	{
		return this->prepared.get() == nullptr;
	}

	virtual void bind()
	{
		if (!this->prepared.get()) return;

		GLuint tex = 0;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef USE_SDL
		SDL_Surface *used = this->prepared->cvtsurface ? this->prepared->cvtsurface : this->prepared->rawsurface;    
		glTexImage2D(GL_TEXTURE_2D, 0, this->prepared->mode, this->prepared->width, this->prepared->height, 0, this->prepared->mode, GL_UNSIGNED_BYTE, used->pixels);
#elif defined PLATFORM_IOS
		glTexImage2D(GL_TEXTURE_2D, 0, this->prepared->mode, this->prepared->width, this->prepared->height, 0, this->prepared->mode, GL_UNSIGNED_BYTE, this->prepared->rawdata);
#endif

		this->gltex = tex;
		this->prepared.reset(nullptr);
#ifdef LOGGING_TEXTURE
			logger::info("creating texture", this->gltex);
#endif
	}

	virtual GLuint get_tex() const
	{
		return this->gltex;
	}

private:
	struct preparation
	{
#ifdef USE_SDL
		SDL_Surface *rawsurface;
		SDL_Surface *cvtsurface;
#elif defined PLATFORM_IOS
        void *rawdata;
#endif
		GLint mode;
		int width;
		int height;

		~preparation()
		{
#ifdef USE_SDL
			if (this->rawsurface) SDL_FreeSurface(this->rawsurface);
			if (this->cvtsurface) SDL_FreeSurface(this->cvtsurface);
#elif defined PLATFORM_IOS
			 if (this->rawdata) free(this->rawdata);
#endif
		}
	};

	tilecache *enclosing;
	std::unique_ptr<preparation> prepared;
	GLuint gltex;

	opengl_pngtexture(tilecache *enclosing, preparation *prepared)
		: enclosing(enclosing), prepared(prepared), gltex(0)
	{
	}

	opengl_pngtexture(const pngtexture &r);
	opengl_pngtexture & operator=(const pngtexture &r);
};
#endif

pngtexture * pngtexture::load(tilecache *enclosing, const std::string &path)
{
#ifdef PLATFORM_CLR
	try {
		return wpf_pngtexture::load(enclosing, path);
	}
	catch (IOException^) {
		return nullptr;
	}
#else
	return opengl_pngtexture::load(enclosing, path);
#endif
}

#ifdef PLATFORM_CLR
class wpf_renderer : public renderer
{
public:
	wpf_renderer(short tilesize)
		: drawctxvalid(false)
	{
	}

	virtual void set_context(render_context &ctx)
	{
		this->drawctxvalid = ctx.valid;
		this->drawctx = ctx.drawctx;
	}

	virtual void draw_tile(int timestamp, tilecache *tiles, const mapctrl::screenpair &screensize, const mapctrl::tilelayer &layer, const float *factor, bool draw, const mapctrl::tilepair &tile, const mapctrl::pixelpair &pixel)
	{
		if (!this->drawctxvalid) return;

		auto pos = layer.get_screen(tile);
		{
			auto mapsize = static_cast<int>(layer.get_mapsize());

			int rotatecount = 0;
			if (pixel.x < 0)
				rotatecount = (-pixel.x + mapsize - 1) / mapsize;
			else if (pixel.x >= mapsize)
				rotatecount = -pixel.x / mapsize;

			if (rotatecount) {
				auto safetile = tile;
				safetile.x -= rotatecount * layer.get_tilenum();
				pos = layer.get_screen(safetile);
			}
		}

		auto key = layer.get_quadkey(tile);

		TransformGroup^ transform = gcnew TransformGroup();
		transform->Children->Add(gcnew TranslateTransform(static_cast<float>(pos.x), static_cast<float>(pos.y)));
		if (factor) {
			transform->Children->Add(gcnew TranslateTransform(-static_cast<float>(screensize.x) / 2, -static_cast<float>(screensize.y) / 2));
			transform->Children->Add(gcnew ScaleTransform(*factor, *factor));
			transform->Children->Add(gcnew TranslateTransform(static_cast<float>(screensize.x) / 2, static_cast<float>(screensize.y) / 2));
		}

		bool found = false;
		int texzoomlevel = 1;
		auto image = this->get_texture(key, timestamp, tiles, &found, &texzoomlevel);
		if (draw && found) {
			this->drawctx->PushTransform(transform);

			int difflevel = layer.get_zoomlevel() - texzoomlevel;
			if (difflevel == 0) {
				Size size(layer.get_tilesize(), layer.get_tilesize());
				Rect rect(size);
				this->drawctx->DrawImage(image, rect);
			}
			else {
				auto targetpix = layer.get_pixel(tile);
				float scale = std::pow(2.f, difflevel);
				auto sourcepix = mapctrl::pixelpair(static_cast<int>(targetpix.x / scale), static_cast<int>(targetpix.y / scale));
				auto sourcealignedpix = layer.get_pixel(layer.get_tile(sourcepix));

				double w = layer.get_tilesize() * scale;
				double x = (sourcepix.x - sourcealignedpix.x) * scale;
				double y = (sourcepix.y - sourcealignedpix.y) * scale;
				Size size(w, w);
				Point start(-x, -y);
				Rect rect(start, size);

				auto cliprect = gcnew RectangleGeometry(Rect(Size(layer.get_tilesize(), layer.get_tilesize())));
				this->drawctx->PushClip(cliprect);

				this->drawctx->DrawImage(image, rect);

				this->drawctx->Pop();
			}

			this->drawctx->Pop();
		}
	}

private:
	bool drawctxvalid;
	gcroot<DrawingContext^> drawctx;

	ImageSource^ get_texture(const mapctrl::quadkey &key, int timestamp, tilecache *tiles, bool *found, int *zoomlevel)
	{
		auto result = tiles->get_texture(key, timestamp);
		if (!result.second) {
			*found = false;
			*zoomlevel = 0;
			return nullptr;
		}

		*found = true;
		*zoomlevel = result.first.get_lod();
		return result.second->get_tex();
	}
};
#elif defined USE_OPENGL
class opengl_renderer : public renderer
{
public:
	opengl_renderer(short tilesize)
		: tilevertices(nullptr)
	{
		static const char vertices[] =
		{
			0, 0,
			1, 0,
			1, 1,
			0, 1,
			// the followings are for lines
			1, 0,
			1, 1,
			0, 0,
			0, 1,
		};
		int numvert = sizeof(vertices) / sizeof(char);

		this->tilevertices = new GLshort[numvert];
		for (int i = 0; i < numvert; ++i)
			this->tilevertices[i] = vertices[i] * tilesize;
	}

	~opengl_renderer()
	{
		if (this->tilevertices)
			delete [] this->tilevertices;
	}

	virtual void draw_tile(int timestamp, tilecache *tiles, const mapctrl::screenpair &screensize, const mapctrl::tilelayer &layer, const float *factor, bool draw, const mapctrl::tilepair &tile, const mapctrl::pixelpair &pixel)
	{
		auto pos = layer.get_screen(tile);
		{
			auto mapsize = static_cast<int>(layer.get_mapsize());

			int rotatecount = 0;
			if (pixel.x < 0)
				rotatecount = (-pixel.x + mapsize - 1) / mapsize;
			else if (pixel.x >= mapsize)
				rotatecount = -pixel.x / mapsize;

			if (rotatecount) {
				auto safetile = tile;
				safetile.x -= rotatecount * layer.get_tilenum();
				pos = layer.get_screen(safetile);
			}
		}

		auto key = layer.get_quadkey(tile);
#ifdef LOGGING_VISIBLETILES
		{
			logger::info(key.str(), tile.x, tile.y);
			logger::info(" -> ", pos.x, pos.y);
		}
#endif

		glPushMatrix();

		glLoadIdentity();
		if (factor) {
			glTranslatef(static_cast<float>(screensize.x) / 2, static_cast<float>(screensize.y) / 2, 0.0);
			glScalef(*factor, *factor, 0.0);
			glTranslatef(-static_cast<float>(screensize.x) / 2, -static_cast<float>(screensize.y) / 2, 0.0);
		}
		glTranslatef(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0);

		if (config::showwireframe) {
			static const float colors[][3] =
			{
				{ 1.0f, 0.0f, 0.0f, },
				{ 1.0f, 1.0f, 0.0f, },
				{ 0.0f, 1.0f, 0.0f, },
				{ 0.0f, 0.0f, 1.0f, },
			};
			const float *color = colors[layer.get_zoomlevel() % 4];
			glColor4f(color[0], color[1], color[2], 1.0f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_SHORT, 0, this->tilevertices);
			glDrawArrays(GL_LINES, 0, 8);
			glDisableClientState(GL_VERTEX_ARRAY);

			if (false) {
				static const short markvertices[] =
				{
					0, 0,
					5, 0,
					5, 5,
					0, 5,
				};
				for (int i = 0; i < key.get_lod(); ++i) {
					auto bit2 = key[i];
					const float *markcolor = colors[bit2];
					glColor4f(markcolor[0], markcolor[1], markcolor[2], 1.0f);

					glPushMatrix();

					glLoadIdentity();
					glTranslatef(static_cast<float>(i) * 6.0f + 2.0f, 2.0f, 0.0f);
					if (factor)
						glScalef(*factor, *factor, 0.0f);
					glTranslatef(static_cast<float>(pos.x), static_cast<float>(pos.y), 0.0f);

					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(2, GL_SHORT, 0, markvertices);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
					glDisableClientState(GL_VERTEX_ARRAY);

					glPopMatrix();
				}
			}
		}
		else {
			GLuint texture = 0;
			auto result = this->get_texture(key, timestamp, tiles, &texture);
			if (draw && result.first) {
				glBindTexture(GL_TEXTURE_2D, texture);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(2, GL_SHORT, 0, this->tilevertices);

				int difflevel = layer.get_zoomlevel() - result.second;
				if (difflevel == 0) {
					static const GLshort texvertices[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
					glTexCoordPointer(2, GL_SHORT, 0, texvertices);
				}
				else {
					auto targetpix = layer.get_pixel(tile);
					float scale = std::pow(2.f, difflevel);
					auto sourcepix = mapctrl::pixelpair(static_cast<int>(targetpix.x / scale), static_cast<int>(targetpix.y / scale));
					auto sourcealignedpix = layer.get_pixel(layer.get_tile(sourcepix));

					GLfloat x, y, w, h;
					w = h = layer.get_tilesize() / scale / layer.get_tilesize();
					x = (sourcepix.x - sourcealignedpix.x) / static_cast<float>(layer.get_tilesize());
					y = (sourcepix.y - sourcealignedpix.y) / static_cast<float>(layer.get_tilesize());
					GLfloat scaledtexvertices[] = { x, y, x + w, y, x + w, y + h, x, y + h };

					glTexCoordPointer(2, GL_FLOAT, 0, scaledtexvertices);
				}

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				glDisableClientState(GL_VERTEX_ARRAY);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}

		glPopMatrix();
	}

#ifdef IMPLEMENT_GPSPIN
	virtual void draw_gpspin(const mapctrl::screenpair &screen, const pngtexture *texture, const mapctrl::screenpair &hotpoint)
	{
		if (!texture) return;

		static const GLshort texvertices[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
		static const GLshort vertices[] = { 0, 0, 16, 0, 16, 16, 0, 16 };

		glPushMatrix();
		glLoadIdentity();
		glTranslatef(static_cast<float>(screen.x - hotpoint.x), static_cast<float>(screen.y - hotpoint.y), 0.0f);

		glBindTexture(GL_TEXTURE_2D, texture->get_tex());
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_SHORT, 0, vertices);
		glTexCoordPointer(2, GL_SHORT, 0, texvertices);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glPopMatrix();
	}

	virtual void draw_gpspin_shadow(const mapctrl::screenpair &screen, float radius)
	{
		const int nummaxsegs = 50;
		GLfloat vertices[nummaxsegs * 2];
		int numsegs = nummaxsegs;
		for (int i = 0; i < numsegs; ++i) {
			float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(numsegs);
			vertices[i * 2 + 0] = radius * std::cos(theta);
			vertices[i * 2 + 1] = radius * std::sin(theta);
		}

		glPushMatrix();
		glLoadIdentity();
		glTranslatef(static_cast<float>(screen.x), static_cast<float>(screen.y), 0.0f);

		glColor4f(0.2f, 0.2f, 0.4f, 0.3f);

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, vertices);
		glDrawArrays(GL_TRIANGLE_FAN, 0, numsegs);
		glDisableClientState(GL_VERTEX_ARRAY);

		glDisable(GL_LINE_SMOOTH);

		glPopMatrix();
	}
#endif

private:
	GLshort *tilevertices;

	std::pair<bool, int> get_texture(const mapctrl::quadkey &key, int timestamp, tilecache *tiles, GLuint *texture)
	{
		auto result = tiles->get_texture(key, timestamp);
		if (!result.second)
			return std::make_pair(false, 0);
		*texture = result.second->get_tex();
		return std::make_pair(true, result.first.get_lod());
	}
};
#endif

renderer * renderer::create(short tilesize)
{
#ifdef PLATFORM_CLR
	return new wpf_renderer(tilesize);
#else
	return new opengl_renderer(tilesize);
#endif
}
