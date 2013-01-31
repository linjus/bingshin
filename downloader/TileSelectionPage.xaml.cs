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
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;

namespace downloader
{
	public partial class TileSelectionPage : ChildWindow
	{
		private readonly MainPage enclosing;
		public TileSelection SelectedTile { get; private set; }

		public TileSelectionPage(MainPage enclosing, TileSelection selection)
		{
			InitializeComponent();

			this.enclosing = enclosing;
			this.SelectedTile = selection;

			this.textName.Text = selection.Name;
			this.checkAerial.IsChecked = selection.HasAerial;
			this.checkAerial.IsEnabled = !selection.HasAerial;
			this.checkRoad.IsChecked = selection.HasRoad;
			this.checkRoad.IsEnabled = !selection.HasRoad;

			this.sliderLevel.Minimum = this.enclosing.TileLayer.MinZoomLevel;
			this.sliderLevel.Maximum = this.enclosing.TileLayer.MaxZoomLevel;
			this.sliderLevel.Value = this.SelectedTile.MaxLevel;

			this.UpdateTileList();
			this.UpdateSummary();
		}

		private void OKButton_Click(object sender, RoutedEventArgs e)
		{
			this.SelectedTile.Name = this.textName.Text;

			this.SelectedTile.HasAerial = this.checkAerial.IsChecked.Value;
			this.SelectedTile.HasRoad = this.checkRoad.IsChecked.Value;
			this.SelectedTile.MaxLevel = (int)this.sliderLevel.Value;

			this.DialogResult = true;
		}

		private void CancelButton_Click(object sender, RoutedEventArgs e)
		{
			this.DialogResult = false;
		}

		private void sliderLevel_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
		{
			var newval = (int)e.NewValue;

			if (newval >= this.SelectedTile.Level)
				this.SelectedTile.MaxLevel = newval;

			this.enclosing.SetZoomLevel(newval);
			this.UpdateSummary();
		}

		private void checkAerial_Checked(object sender, RoutedEventArgs e)
		{
			this.SelectedTile.HasAerial = this.checkAerial.IsChecked.Value;

			this.UpdateSummary();
		}

		private void checkRoad_Checked(object sender, RoutedEventArgs e)
		{
			this.SelectedTile.HasRoad = this.checkRoad.IsChecked.Value;

			this.UpdateSummary();
		}

		private void UpdateTileList()
		{
			var levels = new List<TileLevel>();
			for (int i = this.enclosing.TileLayer.MinZoomLevel; i <= this.enclosing.TileLayer.MaxZoomLevel; ++i) {
				var nt = this.SelectedTile.GetNumTiles(i);
				var fs = this.SelectedTile.GetFileSize(i);
				levels.Add(new TileLevel(i, nt, fs));
			}

			this.listTiles.ItemsSource = levels;
		}

		private void UpdateSummary()
		{
			long numtiles = this.SelectedTile.NumTotalTiles;
			long numbytes = this.SelectedTile.TotalFileSize;

			string[] suffix = { "", "K", "M", "G", "T" };
			double remaining = numbytes;
			int suffixindex = 0;
			for (; suffixindex < suffix.Length; ++suffixindex) {
				if (remaining < 1024)
					break;
				remaining /= 1024;
			}
			string prettysize = string.Format("{0:0.###}", remaining) + " " + suffix[suffixindex] + "bytes";

			this.textSummary.Text = "Level of Details: " + this.enclosing.TileLayer.MinZoomLevel + " ~ " + this.SelectedTile.MaxLevel;
			this.textSizes.Text = numtiles.ToString() + " tiles, " + prettysize + " (approximate)";
		}
	}

	public class TileLevel
	{
		public int Level { get; set; }
		public int NumTiles { get; set; }
		public long NumBytes { get; set; }

		public TileLevel(int level, int numtiles, long numbytes)
		{
			this.Level = level;
			this.NumTiles = numtiles;
			this.NumBytes = numbytes;
		}
	}
}
