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

using System;
using System.Text;
using System.IO;

namespace downloader
{
	public class Repository
	{
		public enum MapStyle
		{
			Hybrid,
			Road,
		}

		public string RootDirectory { get; private set; }
		public int CombineFactor { get; private set; }

		public string SelectionFile { get { return Path.Combine(this.RootDirectory, "selection"); } }
		public string TileDirectory { get { return Path.Combine(this.RootDirectory, "tiles"); } }

		public Repository(string path)
		{
			if (!Directory.Exists(path))
				throw new DirectoryNotFoundException(path);

			this.RootDirectory = path;
			this.CombineFactor = 4;
		}

		public bool Exist(string quadkey, MapStyle style)
		{
			var abspath = this.GetAbsolutePath(quadkey, style);
			return File.Exists(abspath);
		}

		public string GetAbsolutePath(string quadkey, MapStyle style)
		{
			var relpath = this.GetRelativePath(quadkey, style);
			return Path.Combine(this.TileDirectory, relpath);
		}

		public bool PreparePath(string quadkey, MapStyle style)
		{
			var abspath = this.GetAbsolutePath(quadkey, style);
			var dir = Path.GetDirectoryName(abspath);
			Directory.CreateDirectory(dir);
			return true;
		}

		private string GetRelativePath(string quadkey, MapStyle style)
		{
			string path = "";
			for (int i = 0; i < quadkey.Length; i += this.CombineFactor) {
				var fragment = quadkey.Substring(i, Math.Min(this.CombineFactor, quadkey.Length - i));
				path = Path.Combine(path, fragment);
			}
			path += "_";

			switch (style) {
				case MapStyle.Road:
					path += "r.png_";
					break;
				case MapStyle.Hybrid:
					path += "h.jpeg_";
					break;
			}

			return path;
		}
	}
}
