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
using System.Collections.Generic;
using System.Net;
using System.Collections.ObjectModel;
using System.Xml.Serialization;
using System.IO;

namespace downloader
{
	public static class TileSelectionFile
	{
		public static ObservableCollection<TileSelection> Read(string path)
		{
			if (!File.Exists(path))
				return null;

			using (TextReader reader = new StreamReader(path)) {
				var xs = new XmlSerializer(typeof(ObservableCollection<TileSelection>));
				return (ObservableCollection<TileSelection>)xs.Deserialize(reader);
			}
		}

		public static void Write(string path, ObservableCollection<TileSelection> selections)
		{
			using (TextWriter writer = new StreamWriter(path)) {
				var xs = new XmlSerializer(selections.GetType());
				xs.Serialize(writer, selections);
			}
		}
	}

	public class TileSelection
	{
		public string Name { get; set; }
		public int Level { get; set; }
		public TileLayer.TilePair NW { get; set; }
		public TileLayer.TilePair SE { get; set; }
		public bool HasAerial { get; set; }
		public bool HasRoad { get; set; }
		public int MaxLevel { get; set; }
		public bool Complete { get; set; }

		public TileSelection()
		{
		}

		public TileSelection(string name, BingMap.PixelSelection selection, TileLayer tilelayer, bool aerial, bool road)
		{
			this.Name = name;
			this.Level = tilelayer.ZoomLevel;
			this.NW = tilelayer.GetEnclosingTile(selection.TopLeft);
			this.SE = tilelayer.GetEnclosingTile(selection.BottomRight);
			this.HasAerial = aerial;
			this.HasRoad = road;
			this.MaxLevel = tilelayer.ZoomLevel;
		}

		public string LatLong
		{
			get
			{
				int pixx, pixy;
				TileSystem.TileXYToPixelXY(this.NW.X, this.NW.Y, out pixx, out pixy);

				double lat, lon;
				TileSystem.PixelXYToLatLong(pixx, pixy, this.Level, out lat, out lon);

				return string.Format("{0:0.###}, {1:0.###}", lat, lon);
			}
		}

		public long NumTotalTiles
		{
			get
			{
				int modes = 0;
				if (this.HasAerial) ++modes;
				if (this.HasRoad) ++modes;

				long numtiles = 0;
				for (int i = TileLayer.MINZOOMLEVEL; i <= this.MaxLevel; ++i)
					numtiles += this.GetNumTiles(i) * modes;

				return numtiles;
			}
		}

		public long TotalFileSize
		{
			get
			{
				return this.NumTotalTiles * TileLayer.AVGTILEFILESIZE;
			}
		}

		public int GetNumTiles(int lod)
		{
			int numtiles = TileLayer.CountTiles(this.NW, this.SE, this.Level, lod);
			return numtiles;
		}

		public IEnumerable<string> Tiles
		{
			get
			{
				for (int i = TileLayer.MINZOOMLEVEL; i <= this.MaxLevel; ++i) {
					var tiles = TileLayer.IterateTiles(this.NW, this.SE, this.Level, i);
					foreach (var tile in tiles) {
						var quadkey = TileLayer.QuadKey(tile, i);
						yield return quadkey;
					}
				}
			}
		}

		public long GetFileSize(int lod)
		{
			return (long)this.GetNumTiles(lod) * TileLayer.AVGTILEFILESIZE;
		}
	}
}
