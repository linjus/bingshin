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
using Microsoft.Maps.MapControl;
using System.Text;
using System.IO;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Threading;

namespace downloader
{
	public partial class MainPage : UserControl
	{
		private readonly int TileSize = 256;
		private readonly Repository repository;
		private readonly TileLayer tilelayer = new TileLayer();
		private ObservableCollection<TileSelection> tileselections;
		private Point cursorpos;
		private bool mapunstable = true;
		private BackgroundWorker downloadworker;

		public TileLayer TileLayer { get { return this.tilelayer; } }

		public MainPage()
		{
			InitializeComponent();

			var rootdir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "lindsay");
			this.repository = new Repository(rootdir);

			this.downloadworker = new BackgroundWorker();
			this.downloadworker.WorkerReportsProgress = true;
			this.downloadworker.WorkerSupportsCancellation = true;
			this.downloadworker.DoWork += new DoWorkEventHandler(downloadworker_DoWork);
			this.downloadworker.ProgressChanged += new ProgressChangedEventHandler(downloadworker_ProgressChanged);
			this.downloadworker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(downloadworker_RunWorkerCompleted);

			if (!Application.Current.HasElevatedPermissions)
				this.textLog.Text = "permission!";

			this.borderMouseOverTile.Width = this.TileSize;
			this.borderMouseOverTile.Height = this.TileSize;

			this.borderSelectingTiles.Visibility = System.Windows.Visibility.Collapsed;
			this.borderSelectedTiles.Visibility = System.Windows.Visibility.Collapsed;

			this.mapMain.Layer = this.tilelayer;
			this.mapMain.Loaded += new RoutedEventHandler(mapMain_Loaded);
			this.mapMain.ViewChangeStart += new EventHandler<MapEventArgs>(mapMain_ViewChangeStart);
			this.mapMain.ViewChangeEnd += new EventHandler<MapEventArgs>(mapMain_ViewChangeEnd);
			this.mapMain.MouseMove += new MouseEventHandler(mapMain_MouseMove);
			this.mapMain.SelectionDoneEvent += new MouseButtonEventHandler(mapMain_SelectionDoneEvent);
		}

		#region Event

		void mapMain_Loaded(object sender, RoutedEventArgs e)
		{
			this.mapMain.ZoomLevel = 15;
			this.mapMain.Center = new Location(40.05382, -88.24591);

			this.tileselections = new ObservableCollection<TileSelection>();
			this.tileselections.CollectionChanged += new System.Collections.Specialized.NotifyCollectionChangedEventHandler(tileselections_CollectionChanged);
			this.listSelections.ItemsSource = this.tileselections;
			{
				ObservableCollection<TileSelection> selections = null;
				try {
					selections = TileSelectionFile.Read(this.repository.SelectionFile);
					MessageBox.Show(this.repository.SelectionFile + " was loaded.");
					if (selections != null) {
						foreach (var selection in selections)
							this.tileselections.Add(selection);
					}
				}
				catch (Exception ex) {
					MessageBox.Show(ex.StackTrace, ex.Message, MessageBoxButton.OK);
				}
			}
		}

		void mapMain_ViewChangeStart(object sender, MapEventArgs e)
		{
			if (this.mapMain.Selection.Active)
				this.mapMain.Selection.Cancel();

			this.mapunstable = true;

			this.ShowSelection(false);
			this.ShowMouseMoveTile(false);
		}

		void mapMain_ViewChangeEnd(object sender, MapEventArgs e)
		{
			this.mapunstable = false;
			this.tilelayer.SetStable(this.mapMain);

			this.ShowMouseMoveTile(true);
			this.ShowSelection(true);
		}

		void mapMain_MouseMove(object sender, MouseEventArgs e)
		{
			this.cursorpos = e.GetPosition(this.mapMain);

			if (!this.mapunstable) {
				this.ShowSelection(true);
				this.ShowMouseMoveTile(true);
			}
		}

		void mapMain_SelectionDoneEvent(object sender, MouseButtonEventArgs e)
		{
			var aerialmode = this.mapMain.Mode is AerialMode;
			var roadmode = this.mapMain.Mode is RoadMode;

			var name = "Selection " + (this.tileselections.Count + 1).ToString();
			var selection = new TileSelection(name, this.mapMain.Selection, this.tilelayer, aerialmode, roadmode);

			var subpage = new TileSelectionPage(this, selection);
			subpage.Closed += new EventHandler(tileselectionpage_Closed);
			subpage.Show();

			this.IsEnabled = false;
		}

		private void listSelections_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			var selection = (TileSelection)this.listSelections.SelectedItem;
			var nwloc = this.tilelayer.GetLocation(selection.NW, selection.Level, false);
			var seloc = this.tilelayer.GetLocation(selection.SE, selection.Level, true);
			var locations = new List<Location>();
			locations.Add(nwloc);
			locations.Add(seloc);
			this.mapMain.SetView(new LocationRect(locations));

			this.ShowSelection(true);
		}

		void tileselectionpage_Closed(object sender, EventArgs e)
		{
			var subpage = (TileSelectionPage)sender;
			if (subpage.DialogResult ?? false) {
				this.borderSelectingTiles.Visibility = System.Windows.Visibility.Collapsed;
				this.tileselections.Add(subpage.SelectedTile);

				this.listSelections.SelectedItem = subpage.SelectedTile;
			}

			this.IsEnabled = true;
		}

		void tileselections_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
		{
			this.UpdateSummaryText();
		}

		private void buttonSaveSelection_Click(object sender, RoutedEventArgs e)
		{
			try {
				TileSelectionFile.Write(this.repository.SelectionFile, this.tileselections);
				MessageBox.Show(this.repository.SelectionFile + " was saved.");
			}
			catch (Exception ex) {
				MessageBox.Show(ex.StackTrace, ex.Message, MessageBoxButton.OK);
			}
		}

		private void buttonDownload_Click(object sender, RoutedEventArgs e)
		{
			var selected = this.listSelections.SelectedItem;
			if (selected != null && !this.downloadworker.IsBusy)
				this.downloadworker.RunWorkerAsync(selected);
		}

		private void buttonCancel_Click(object sender, RoutedEventArgs e)
		{
			if (this.downloadworker.IsBusy)
				this.downloadworker.CancelAsync();
		}

		void downloadworker_DoWork(object sender, DoWorkEventArgs e)
		{
			var selection = (TileSelection)e.Argument;
			this.downloadercontext = new DownloaderContext(selection.NumTotalTiles);

			this.BeginDownload(selection, e);
		}

		void downloadworker_ProgressChanged(object sender, ProgressChangedEventArgs e)
		{
			this.progressDownload.Value = e.ProgressPercentage;
		}

		void downloadworker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
		{
			var s = new StringBuilder();
			s.Append(this.downloadercontext.NumSuccess + " tiles were downloaded.\r\n");
			if (this.downloadercontext.NumFailure > 0)
				s.Append(this.downloadercontext.NumFailure + " tiles were failed.\r\n");

			MessageBox.Show(s.ToString(), "Download Result", MessageBoxButton.OK);

			this.downloadercontext = null;
		}

		#endregion

		private void UpdateSummaryText()
		{
			long numtiles = 0;
			long numbytes = 0;

			foreach (var selection in this.tileselections) {
				numtiles += selection.NumTotalTiles;
				numbytes += selection.TotalFileSize;
			}

			string[] suffix = { "", "K", "M", "G", "T" };
			double remaining = numbytes;
			int suffixindex = 0;
			for (; suffixindex < suffix.Length; ++suffixindex) {
				if (remaining < 1024)
					break;
				remaining /= 1024;
			}
			string prettysize = string.Format("{0:0.###}", remaining) + " " + suffix[suffixindex] + "bytes";

			this.textNumTiles.Text = numtiles.ToString() + " tiles";
			this.textNumBytes.Text = prettysize;
		}

		private void DrawSelectionTiles(Border border, TileLayer.PixelPair nw, TileLayer.PixelPair se)
		{
			var nwpos = this.tilelayer.GetClippedScreenPosition(nw);
			var sepos = this.tilelayer.GetClippedScreenPosition(se);

			if (!(nwpos.HasValue && sepos.HasValue)) return;

			border.Margin = new Thickness(nwpos.Value.X, nwpos.Value.Y, 0, 0);
			border.Width = sepos.Value.X - nwpos.Value.X;
			border.Height = sepos.Value.Y - nwpos.Value.Y;

			if (border.Visibility == System.Windows.Visibility.Collapsed)
				border.Visibility = System.Windows.Visibility.Visible;
		}

		private void ShowSelection(bool show)
		{
			if (show) {
				if (this.mapMain.Selection.Active) {
					var tltile = this.tilelayer.GetEnclosingTile(this.mapMain.Selection.TopLeft);
					var brtile = this.tilelayer.GetEnclosingTile(this.mapMain.Selection.BottomRight);
					var tlpix = this.tilelayer.GetPixel(tltile);
					var brpix = this.tilelayer.GetPixel(brtile);
					brpix.X += this.tilelayer.TileSize;
					brpix.Y += this.tilelayer.TileSize;
					this.DrawSelectionTiles(this.borderSelectingTiles, tlpix, brpix);
				}
				else if (this.listSelections.SelectedItem != null) {
					var selection = (TileSelection)this.listSelections.SelectedItem;

					var toodetailed = this.tilelayer.ZoomLevel > selection.MaxLevel;
					var color = toodetailed ? Colors.Magenta : Colors.Green;
					this.borderSelectedTiles.Background = new SolidColorBrush(color);

					var nwpix = this.tilelayer.GetPixel(selection.NW, selection.Level, false);
					var sepix = this.tilelayer.GetPixel(selection.SE, selection.Level, true);
					this.DrawSelectionTiles(this.borderSelectedTiles, nwpix, sepix);
				}
			}
			else {
				this.borderSelectingTiles.Visibility = System.Windows.Visibility.Collapsed;
				this.borderSelectedTiles.Visibility = System.Windows.Visibility.Collapsed;
			}
		}

		private void ShowMouseMoveTile(bool show)
		{
			bool visible = false;
			if (show) {
				var pixel = this.tilelayer.GetPixel(this.cursorpos);
				var tile = this.tilelayer.GetEnclosingTile(pixel);
				var enclosing = this.tilelayer.GetScreenPosition(tile);
				if (enclosing.HasValue) {
					this.borderMouseOverTile.Margin = new Thickness(enclosing.Value.X, enclosing.Value.Y, 0, 0);
					this.textTileInfo.Margin = new Thickness(enclosing.Value.X + 10, enclosing.Value.Y + 5, 0, 0);

					var quadkey = this.tilelayer.GetQuadKey(tile);
					this.textTileInfo.Text = quadkey;

					visible = true;
				}
			}

			var flag = visible ? System.Windows.Visibility.Visible : System.Windows.Visibility.Collapsed;
			if (this.borderMouseOverTile.Visibility != flag) {
				this.borderMouseOverTile.Visibility = flag;
				this.textTileInfo.Visibility = flag;
			}
		}

		class DownloaderContext
		{
			public long NumWorks;
			public long NumSuccess;
			public long NumFailure;

			public DownloaderContext(long works)
			{
				this.NumWorks = works;
			}

			public void FinishOne(bool success)
			{
				lock (this) {
					if (success)
						this.NumSuccess++;
					else
						this.NumFailure++;
				}
			}
		}

		private DownloaderContext downloadercontext;
		private AutoResetEvent downloadcompleteevent = new AutoResetEvent(false);

		private void BeginDownload(TileSelection selection, DoWorkEventArgs e)
		{
			foreach (var quadkey in selection.Tiles) {
				if (this.downloadworker.CancellationPending) {
					e.Cancel = true;
					break;
				}

				if (selection.HasAerial)
					this.SaveTile(quadkey, Repository.MapStyle.Hybrid);
				if (selection.HasRoad)
					this.SaveTile(quadkey, Repository.MapStyle.Road);

				this.downloadworker.ReportProgress((int)(100.0 * this.downloadercontext.NumSuccess / this.downloadercontext.NumWorks));
			}
		}

		private Uri GetTileURL(string quadkey, Repository.MapStyle style)
		{
			// The following information is from http://gc-livepedia.de/wiki/Map_servers.
			var s = new StringBuilder();
			s.Append(@"http://ecn.t0.tiles.virtualearth.net/tiles/");
			switch (style) {
				case Repository.MapStyle.Road:
					s.Append('r');
					s.Append(quadkey);
					s.Append(@".png?g=471&mkt=en-us&shading=hill");
					break;
				case Repository.MapStyle.Hybrid:
					s.Append('h');
					s.Append(quadkey);
					s.Append(@".jpeg?g=471");
					break;
			}
			return new Uri(s.ToString());
		}

		class DownloaderArgument
		{
			public string QuadKey;
			public Repository.MapStyle Style;

			public DownloaderArgument(string key, Repository.MapStyle style)
			{
				this.QuadKey = key;
				this.Style = style;
			}
		}

		private void SaveTile(string quadkey, Repository.MapStyle style)
		{
			if (this.repository.Exist(quadkey, style)) {
				this.downloadercontext.FinishOne(true);
				return;
			}

			var uri = this.GetTileURL(quadkey, style);

			var arg = new DownloaderArgument(quadkey, style);

			var client = new WebClient();
			client.OpenReadCompleted += new OpenReadCompletedEventHandler(client_OpenReadCompleted);
			client.OpenReadAsync(uri, arg);

			this.downloadcompleteevent.WaitOne();
		}

		void client_OpenReadCompleted(object sender, OpenReadCompletedEventArgs e)
		{
			var arg = (DownloaderArgument)e.UserState;

			this.repository.PreparePath(arg.QuadKey, arg.Style);
			var path = this.repository.GetAbsolutePath(arg.QuadKey, arg.Style);

			bool successful = false;
			try {
				using (var writer = new FileStream(path, FileMode.CreateNew)) {
					byte[] buffer = new byte[4096];
					for (; ; ) {
						var count = e.Result.Read(buffer, 0, buffer.Length);
						if (count <= 0) break;
						writer.Write(buffer, 0, count);
					}
				}

				successful = true;
			}
			catch (Exception) {
			}
			finally {
				try {
					if (e.Result != null)
						e.Result.Close();
				}
				catch (Exception) {
					successful = false;
				}
			}

			if (!successful) {
				try {
					File.Delete(path);
				}
				catch (Exception) {
				}
			}

			this.downloadercontext.FinishOne(successful);
			this.downloadcompleteevent.Set();
		}

		internal void SetZoomLevel(int newval)
		{
			var center = this.mapMain.Center;
			this.mapMain.SetView(center, newval);
		}
	}

	public class TileLayer
	{
		internal static readonly int TILESIZE = 256;
		internal static readonly int MINZOOMLEVEL = 1;
		internal static readonly int MAXZOOMLEVEL = 20;
		internal static readonly int AVGTILEFILESIZE = 10000;

		public int TileSize { get { return TILESIZE; } }
		public int ZoomLevel { get; private set; }
		public int MinZoomLevel { get { return MINZOOMLEVEL; } }
		public int MaxZoomLevel { get { return MAXZOOMLEVEL; } }
		public int AvgTileFileSize { get { return AVGTILEFILESIZE; } }
		public PixelPair PixelNW { get; private set; }
		public PixelPair PixelSE { get; private set; }

		public struct PixelPair
		{
			public int X;
			public int Y;

			public PixelPair(int x, int y)
			{
				this.X = x;
				this.Y = y;
			}

			public override string ToString()
			{
				return this.X.ToString() + ", " + this.Y.ToString();
			}
		}

		public struct TilePair
		{
			public int X;
			public int Y;

			public TilePair(int x, int y)
			{
				this.X = x;
				this.Y = y;
			}

			public override string ToString()
			{
				return this.X.ToString() + ", " + this.Y.ToString();
			}
		}

		public bool IsStable { get; private set; }

		public void SetStable(Map map)
		{
			this.ZoomLevel = (int)map.TargetZoomLevel;
			this.PixelNW = this.GetPixel(map.TargetBoundingRectangle.Northwest);
			this.PixelSE = this.GetPixel(map.TargetBoundingRectangle.Southeast);

			this.IsStable = true;
		}

		public Location GetLocation(TilePair tile, int tilelod, bool secorner)
		{
			var pixel = this.GetPixel(tile, tilelod, secorner);
			double lat, lon;
			TileSystem.PixelXYToLatLong(pixel.X, pixel.Y, this.ZoomLevel, out lat, out lon);
			return new Location(lat, lon);
		}

		public PixelPair GetPixel(Location location)
		{
			var level = this.ZoomLevel;
			int pixelx, pixely;
			TileSystem.LatLongToPixelXY(location.Latitude, location.Longitude, level, out pixelx, out pixely);
			return new PixelPair(pixelx, pixely);
		}

		public PixelPair GetPixel(Point pos)
		{
			var x = this.PixelNW.X + (int)pos.X;
			var y = this.PixelNW.Y + (int)pos.Y;
			return new PixelPair(x, y);
		}

		private static PixelPair Pixel(TilePair tile)
		{
			int pixx, pixy;
			TileSystem.TileXYToPixelXY(tile.X, tile.Y, out pixx, out pixy);
			return new PixelPair(pixx, pixy);
		}

		public PixelPair GetPixel(TilePair tile)
		{
			return Pixel(tile);
		}

		public PixelPair GetPixel(TilePair tile, int tilelod, bool secorner)
		{
			var srcpix = this.GetPixel(tile);
			if (secorner) {
				srcpix.X += this.TileSize;
				srcpix.Y += this.TileSize;
			}

			var diff = this.ZoomLevel - tilelod;
			var factor = Math.Pow(2.0, diff);

			var x = (int)(srcpix.X * factor);
			var y = (int)(srcpix.Y * factor);
			return new PixelPair(x, y);
		}

		private static TilePair EnclosingTile(PixelPair pixel)
		{
			int tilex, tiley;
			TileSystem.PixelXYToTileXY(pixel.X, pixel.Y, out tilex, out tiley);
			return new TilePair(tilex, tiley);
		}

		public TilePair GetEnclosingTile(PixelPair pixel)
		{
			return EnclosingTile(pixel);
		}

		public bool IsScreenPositionInferred()
		{
			// As the Silverlight SDK provides only the information about the bounding box,
			// it seems there is no way to find out if there is a margin at the top, especially
			// when the zoom level is very low and the map is not large enough to fill the map.
			// Even worse, the margin at the top is always visible if the user moves the map to
			// the north pole. Not knowing how to get the exact pixel or latitude information,
			// the best I can do is to warn the user.
			if (this.PixelNW.Y == 0)
				return false;
			if (this.PixelNW.X == 0)
				return false;

			return true;
		}

		public Point? GetScreenPosition(TilePair tile)
		{
			if (!this.IsScreenPositionInferred()) return null;

			int pixelx, pixely;
			TileSystem.TileXYToPixelXY(tile.X, tile.Y, out pixelx, out pixely);
			return this.GetScreenPosition(new PixelPair(pixelx, pixely));
		}

		public Point? GetScreenPosition(PixelPair pixel)
		{
			if (!this.IsScreenPositionInferred()) return null;

			return new Point(pixel.X - this.PixelNW.X, pixel.Y - this.PixelNW.Y);
		}

		public Point? GetClippedScreenPosition(PixelPair pixel)
		{
			if (!this.IsScreenPositionInferred()) return null;

			int x;
			if (this.PixelNW.X > pixel.X)
				x = 0;
			else if (pixel.X > this.PixelSE.X)
				x = this.PixelSE.X - this.PixelNW.X;
			else
				x = pixel.X - this.PixelNW.X;

			int y;
			if (this.PixelNW.Y > pixel.Y)
				y = 0;
			else if (pixel.Y > this.PixelSE.Y)
				y = this.PixelSE.Y - this.PixelNW.Y;
			else
				y = pixel.Y - this.PixelNW.Y;

			return new Point(x, y);
		}

		public string GetQuadKey(TilePair tile)
		{
			return QuadKey(tile, this.ZoomLevel);
		}

		public static string QuadKey(TilePair tile, int lod)
		{
			return TileSystem.TileXYToQuadKey(tile.X, tile.Y, lod);
		}

		public static IEnumerable<TilePair> IterateTiles(PixelPair nw, PixelPair se, int lod)
		{
			var mapwidth = TileSystem.MapSize(lod);
			var boundx = se.X;
			if (nw.X > boundx)
				boundx += (int)mapwidth;

			for (int x = nw.X; x < boundx; x += TILESIZE) {
				for (int y = nw.Y; y < se.Y; y += TILESIZE) {
					var actualx = x % (int)mapwidth;
					yield return EnclosingTile(new PixelPair(actualx, y));
				}
			}
		}

		public static int CountTiles(PixelPair nw, PixelPair se, int lod)
		{
			var mapwidth = TileSystem.MapSize(lod);
			var boundx = se.X;
			if (nw.X > boundx)
				boundx += (int)mapwidth;

			int countx = (boundx - nw.X + TILESIZE - 1) / TILESIZE;
			int county = (se.Y - nw.Y + TILESIZE - 1) / TILESIZE;
			return countx * county;
		}

		public static IEnumerable<TilePair> IterateTiles(TilePair nw, TilePair se, int lod)
		{
			se.X++;
			se.Y++;
			var pixnw = Pixel(nw);
			var pixse = Pixel(se);
			return IterateTiles(pixnw, pixse, lod);
		}

		private static void ScaleTile(TilePair nw, TilePair se, int lod, int targetlod, out PixelPair pixnw, out PixelPair pixse)
		{
			se.X++;
			se.Y++;

			var diff = targetlod - lod;
			var factor = Math.Pow(2.0, diff);

			var targetnw = new TilePair((int)(nw.X * factor), (int)(nw.Y * factor));
			var targetse = new TilePair((int)(se.X * factor), (int)(se.Y * factor));

			if (targetnw.X == targetse.X) targetse.X++;
			if (targetnw.Y == targetse.Y) targetse.Y++;

			if (diff < 0) {
				if (targetse.X * (1.0 / factor) < se.X) targetse.X++;
				if (targetse.Y * (1.0 / factor) < se.Y) targetse.Y++;
			}

			pixnw = Pixel(targetnw);
			pixse = Pixel(targetse);
		}

		public static IEnumerable<TilePair> IterateTiles(TilePair nw, TilePair se, int lod, int targetlod)
		{
			PixelPair pixnw, pixse;
			ScaleTile(nw, se, lod, targetlod, out pixnw, out pixse);
			return IterateTiles(pixnw, pixse, targetlod);
		}

		public static int CountTiles(TilePair nw, TilePair se, int lod, int targetlod)
		{
			PixelPair pixnw, pixse;
			ScaleTile(nw, se, lod, targetlod, out pixnw, out pixse);
			return CountTiles(pixnw, pixse, targetlod);
		}
	}

	public class BingMap : Map
	{
		public TileLayer Layer { get; internal set; }
		public PixelSelection Selection { get; private set; }

		public event MouseButtonEventHandler SelectionDoneEvent;

		public BingMap()
		{
			this.Selection = new PixelSelection();
		}

		public class PixelSelection
		{
			public bool Active { get; private set; }
			private TileLayer.PixelPair begins;
			private TileLayer.PixelPair ends;

			public void Start(TileLayer.PixelPair pair)
			{
				this.Active = true;
				this.begins = pair;
			}

			public void Move(TileLayer.PixelPair pair)
			{
				this.ends = pair;
			}

			public void Cancel()
			{
				this.Active = false;
			}

			public void End(TileLayer.PixelPair pair)
			{
				this.Move(pair);

				this.Active = false;
			}

			public TileLayer.PixelPair TopLeft
			{
				get
				{
					return new TileLayer.PixelPair(Math.Min(this.begins.X, this.ends.X), Math.Min(this.begins.Y, this.ends.Y));
				}
			}

			public TileLayer.PixelPair BottomRight
			{
				get
				{
					return new TileLayer.PixelPair(Math.Max(this.begins.X, this.ends.X), Math.Max(this.begins.Y, this.ends.Y));
				}
			}
		}

		protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
		{
			if (Keyboard.Modifiers == ModifierKeys.Shift) {
				Point pos = e.GetPosition(this);
				var pixel = this.Layer.GetPixel(pos);
				this.Selection.Start(pixel);
				return;
			}

			base.OnMouseLeftButtonDown(e);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			if (this.Selection.Active) {
				Point pos = e.GetPosition(this);
				var pixel = this.Layer.GetPixel(pos);
				this.Selection.Move(pixel);
			}

			base.OnMouseMove(e);
		}

		protected override void OnMouseLeftButtonUp(MouseButtonEventArgs e)
		{
			if (this.Selection.Active) {
				Point pos = e.GetPosition(this);
				var pixel = this.Layer.GetPixel(pos);
				this.Selection.End(pixel);

				if (this.SelectionDoneEvent != null)
					this.SelectionDoneEvent(this, e);
				return;
			}

			base.OnMouseLeftButtonUp(e);
		}
	}
}
