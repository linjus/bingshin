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

class repository
{
public:
	repository(const std::string &rootdir);

	bool exists(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style) const;
	std::string get_absolutepath(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style) const;
#ifdef IMPLEMENT_DOWNLOAD
	std::string get_url(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style) const;
	bool prepare_path(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style) const;
#endif

private:
	const std::string tilerootdir;
	const unsigned int combinefactor;

	std::string get_relativepath(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style) const;
};

template<typename T>
class quadtrie
{
public:
	class node
	{
		node(const mapctrl::quadkey &path)
			: path(path), data(std::make_pair(false, std::unique_ptr<T>(nullptr))), accesstimestamp(std::make_pair(0, 0))
		{
		}

		node(const mapctrl::quadkey &path, T *data, int firstaccess)
			: path(path), data(std::make_pair(true, data)), accesstimestamp(std::make_pair(firstaccess, 0))
		{
		}
    
	private:
		mapctrl::quadkey path;
		std::pair<bool, std::unique_ptr<T>> data;
		std::pair<int, int> accesstimestamp;
		std::unique_ptr<node> children[4];

		friend class quadtrie;
	};

public:
	quadtrie()
		: root(new node(mapctrl::quadkey::epsilon())), numdatanodes(0)
	{
	}

	~quadtrie()
	{
		delete this->root;
	}

	void insert(const mapctrl::quadkey &key, T *data, int firstaccess = 0)
	{
#ifdef LOGGING_QUADTRIE
		//this->dump(0);
		//logger::info("inserting", key.str());
#endif
		this->insert(key, data, firstaccess, this->root);
#ifdef LOGGING_QUADTRIE
		//this->dump(0);
#endif

		if (data) this->numdatanodes++;
	}

	bool remove(const mapctrl::quadkey &key)
	{
		if (!this->remove(key, nullptr, this->root)) return false;

		this->numdatanodes--;
		return true;
	}

	std::pair<bool, std::pair<mapctrl::quadkey, T *>> search(const mapctrl::quadkey &key, int timestamp)
	{
		std::pair<mapctrl::quadkey, node *> best(mapctrl::quadkey::epsilon(), static_cast<node *>(0));
		bool exactdatanode = this->search(key, best, mapctrl::quadkey::epsilon(), this->root);

		if (!best.second)
			return std::make_pair(exactdatanode, std::make_pair(best.first, static_cast<T *>(nullptr)));

		best.second->accesstimestamp.second = timestamp;
		if (!best.second->accesstimestamp.first)
			best.second->accesstimestamp.first = timestamp;
		return std::make_pair(exactdatanode, std::make_pair(best.first, best.second->data.second.get()));
	}

	unsigned int get_numnodes() const
	{
		return this->numdatanodes;
	}

	void sweep(int timestamp)
	{
#ifdef LOGGING_QUADTRIE
		int prevcount = this->dump(timestamp);
#endif

		quadtrie newtrie;
		this->sweep(timestamp, newtrie, mapctrl::quadkey::epsilon(), this->root);

#ifdef LOGGING_QUADTRIE
		int newcount = newtrie.dump(timestamp);
		logger::info("sweep at ", timestamp, prevcount, newcount);
#endif

		delete this->root;

		this->root = newtrie.root;
		this->numdatanodes = newtrie.numdatanodes;

		newtrie.root = nullptr;
	}

	void clear()
	{
		delete this->root;

		this->root = new node(mapctrl::quadkey::epsilon());
		this->numdatanodes = 0;
	}

#ifdef LOGGING_QUADTRIE
	int dump(int timestamp) const
	{
		logger::info(" === DUMP === ", timestamp);
		int count = 0;
		this->dump(timestamp, mapctrl::quadkey::epsilon(), this->root, 0, count);
		logger::info(" # nodes", count);
		return count;
	}
#endif

private:
	node *root;
	unsigned int numdatanodes;

	void insert(const mapctrl::quadkey &key, T *data, int firstaccess, node *parent)
	{
		auto &child = parent->children[key.prefix()];
		if (child.get()) {
			auto commonkey = child->path.intersect(key);
			auto subchild = child->path.suffix(commonkey);
			auto subadding = key.suffix(commonkey);

			if (subchild.empty() && subadding.empty())
				child->data = std::make_pair(true, std::unique_ptr<T>(data));
			else if (subchild.empty())
				this->insert(subadding, data, firstaccess, child.get());
			else if (subadding.empty()) {
				auto adding = new node(key, data, firstaccess);
				auto childnode = child.release();
				child.reset(adding);
				childnode->path = subchild;
				adding->children[subchild.prefix()].reset(childnode);
			}
			else {
				auto pivot = new node(commonkey);
				auto childnode = child.release();
				child.reset(pivot);
				childnode->path = subchild;
				auto adding = new node(subadding, data, firstaccess);
				pivot->children[subchild.prefix()].reset(childnode);
				pivot->children[subadding.prefix()].reset(adding);
			}
		}
		else {
			auto adding = new node(key, data, firstaccess);
			child.reset(adding);
		}
	}

	bool get_singlechild(node *parent, int *singlechildindex)
	{
		bool nothing = true;
		bool multiple = false;
		int index = -1;
		for (int i = 0; i < 4; ++i) {
			if (parent->children[i].get()) {
				nothing = false;
				if (index == -1)
					index = i;
				else
					multiple = true;
			}
		}

		*singlechildindex = multiple ? -1 : index;
		return nothing;
	}

	bool remove(const mapctrl::quadkey &key, node *grandparent, node *parent)
	{
		auto &child = parent->children[key.prefix()];
		if (!child.get()) return false;

		auto commonkey = child->path.intersect(key);
		auto subchild = child->path.suffix(commonkey);
		auto subvictim = key.suffix(commonkey);
		if (subchild.empty() && subvictim.empty()) {
			bool dataremoved = child->data.get() != nullptr;
			child->data.reset(nullptr);
			int singlegrandchildindex;
			if (this->get_singlechild(child.get(), &singlegrandchildindex)) {
				child.reset(nullptr);
				if (!parent->data.get() && grandparent) {
					int singlesiblingindex;
					this->get_singlechild(parent, &singlesiblingindex);
					if (singlesiblingindex != -1) {
						auto &singlesibling = parent->children[singlesiblingindex];
						singlesibling->path = parent->path.concat(singlesibling->path);
						grandparent->children[parent->path.prefix()].reset(singlesibling.release());
					}
				}
			}
			else if (singlegrandchildindex != -1) {
				auto &singlegrandchild = child->children[singlegrandchildindex];
				singlegrandchild->path = child->path.concat(singlegrandchild->path);
				child.reset(singlegrandchild.release());
			}
			return dataremoved;
		}
		else if (subchild.get_lod() > subvictim.get_lod())
			return false;
		else if (subchild.empty())
			return this->remove(subvictim, parent, child.get());
		return false;
	}

	bool search(const mapctrl::quadkey &key, std::pair<mapctrl::quadkey, node *> &best, mapctrl::quadkey abspath, node *parent) const
	{
		auto &child = parent->children[key.prefix()];
		if (!child.get()) return false;

		auto commonkey = child->path.intersect(key);
		auto subchild = child->path.suffix(commonkey);
		auto subneedle = key.suffix(commonkey);
		if (subchild.empty() && subneedle.empty()) {
			if (child->data.second.get())
				best = std::make_pair(abspath.concat(key), child.get());
			return child->data.first;
		}
		else if (subchild.get_lod() > subneedle.get_lod())
			return false;
		else if (subchild.empty()) {
			if (child->data.second.get())
				best = std::make_pair(abspath.concat(commonkey), child.get());
			return this->search(subneedle, best, abspath.concat(commonkey), child.get());
		}
		return false;
	}

	void sweep(int timestamp, quadtrie &newtrie, mapctrl::quadkey abspath, node *parent)
	{
		auto path = abspath.concat(parent->path);

		if (parent->data.second.get()) {
			if (parent->accesstimestamp.second >= timestamp)
				newtrie.insert(path, parent->data.second.release(), parent->accesstimestamp.first);
		}

		for (int i = 0; i < 4; ++i) {
			auto &child = parent->children[i];
			if (child.get())
				this->sweep(timestamp, newtrie, path, child.get());
		}
	}

#ifdef LOGGING_QUADTRIE
	void dump(int timestamp, mapctrl::quadkey parentabspath, const node *n, int level, int &count) const
	{
		bool visible = timestamp == n->accesstimestamp.second;

		std::ostringstream line;
		for (int i = 0; i < level; ++i) {
			if (visible)
				line << "-- ";
			else
				line << "   ";
		}

		mapctrl::quadkey abspath = parentabspath.concat(n->path);
		line << n->path.str() << " '" << abspath.str() << "' : ";

		if (n->data.first) {
			if (n->data.second.get())
				line << "O [" << n->accesstimestamp.first << " ~ " << n->accesstimestamp.second << "]";
			else
				line << 'X';
		}
		logger::info(line.str());
		++count;

		for (int i = 0; i < 4; ++i) {
			auto &child = n->children[i];
			if (child.get())
				this->dump(timestamp, abspath, child.get(), level + 1, count);
		}
	}
#endif
};

class pngtexture_queued
{
public:
	pngtexture_queued(tilecache *enclosing, pngtexture *tex)
		: enclosing(enclosing), tex(tex)
	{
	}

	pngtexture * get_tex()
	{
		return this->tex;
	}

	~pngtexture_queued();

private:
	tilecache *enclosing;
	pngtexture *tex;
};

class tilecache
{
public:
	typedef void (*tileloadedhandler)();

	tilecache(const std::string &rootdir, unsigned int tilesize, unsigned int numtileslimit);
	~tilecache();

	bool initialize(tileloadedhandler handler);
	void work();

	void insert_tile(const mapctrl::quadkey &key, int timestamp, pngtexture *tex);

	void set_mode(mapctrl::mapcontrol::mapstyle style);
	mapctrl::mapcontrol::mapstyle get_mode() const;

	std::pair<mapctrl::quadkey, const pngtexture *> get_texture(const mapctrl::quadkey &key, int timestamp);
#ifdef IMPLEMENT_DOWNLOAD
	void download(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style, int timestamp);
	void prepare_path(const mapctrl::quadkey &key, mapctrl::mapcontrol::mapstyle style);
#endif

#ifdef USE_OPENGL
	void enqueue_useless_texture(pngtexture *tex);
#endif
	void destroy_useless_textures();

	void sweep(int timestamp);

	void clear_dirty();
	bool is_dirty() const
	{
		return this->dirty;
	}

#ifdef LOGGING_QUADTRIE
	void dump_trie(int timestamp) const;
#endif

private:
	const repository repos;
	const unsigned int tilesize;
	mapctrl::mapcontrol::mapstyle tilestyle;
	std::unique_ptr<mapctrl::thread> worker;
	bool flagterminate;
	std::unique_ptr<mapctrl::mutex> mapmutex;
	quadtrie<pngtexture_queued> tiletextures;
	const unsigned int numtileslimit;
	bool dirty;
	std::unique_ptr<mapctrl::mutex> quemutex;
	std::unique_ptr<mapctrl::condvar> quecond;
	std::map<mapctrl::quadkey, unsigned int> requested;
#ifdef USE_OPENGL
	std::vector<std::unique_ptr<pngtexture>> useless;
#endif
	tileloadedhandler on_tileloaded;

	void clear();
};
