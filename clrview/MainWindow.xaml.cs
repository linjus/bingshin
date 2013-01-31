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
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace clrview
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public MainWindow()
		{
			InitializeComponent();

			this.mapMain.Center = Locations.Seoul;
			this.mapMain.ZoomLevel = 3;

			{
				string text = "home";
				var lonlat = Locations.Champaign;

				var pin = new Pin(text);
				pin.MouseEnter += new MouseEventHandler(ctrl_MouseEnter);
				pin.MouseLeave += new MouseEventHandler(ctrl_MouseLeave);
				pin.Click += new RoutedEventHandler(ctrl_Click);
				this.pinid = this.mapMain.AddPin(lonlat.Key, lonlat.Value, pin);
			}
		}

		private int pinid;
		private int trailid;

		void ctrl_Click(object sender, RoutedEventArgs e)
		{
			var ctrl = this.mapMain.RemovePin(this.pinid);
			var button = (Button)ctrl;
		}

		void ctrl_MouseLeave(object sender, MouseEventArgs e)
		{
		}

		void ctrl_MouseEnter(object sender, MouseEventArgs e)
		{
		}

		private void buttonAction1_Click(object sender, RoutedEventArgs e)
		{
			var trail = new Trail();
			var path = new List<KeyValuePair<double, double>>();
			{
				double lon = -88;
				double lat = 40;
				double step = 0.0005;
				for (int i = 0; i < 10000; ++i) {
					if (i % 2 == 0) lon += step;
					else lat += step;
					path.Add(new KeyValuePair<double, double>(lon, lat));
				}
			}
			path.Add(new KeyValuePair<double, double>(-10, 38));
			this.trailid = this.mapMain.AddTrail(path, trail);

			var optimal = this.mapMain.GetOptimalMap(path);
			this.mapMain.AnimateTo(optimal.Key, optimal.Value);
		}

		private void buttonAction2_Click(object sender, RoutedEventArgs e)
		{
			this.mapMain.AnimateTo(10, Locations.Seoul);
		}

		private void buttonAction3_Click(object sender, RoutedEventArgs e)
		{
			this.mapMain.AnimateTo(10, Locations.Champaign);
		}

		private void buttonAction4_Click(object sender, RoutedEventArgs e)
		{
			this.mapMain.AnimateTo(7, Locations.Seoul);
		}

		private void buttonAction5_Click(object sender, RoutedEventArgs e)
		{
			this.mapMain.AnimateTo(13, Locations.Champaign);
		}
	}

	static class Locations
	{
		public static KeyValuePair<double, double> Champaign
		{ get { return new KeyValuePair<double, double>(-88.248948, 40.05298); } }

		public static KeyValuePair<double, double> Seoul
		{ get { return new KeyValuePair<double, double>(126.9, 37.5); } }
	}

	class Pin : Button, Map.ILocation
	{
		public Pin(string text)
		{
			this.Width = 50;
			this.Height = 50;
			this.Content = text;
		}

		#region ILocation Members

		public void SetPixelPair(int x, int y)
		{
			const int sizex = 50;
			const int sizey = 50;
			int adjustedx = x - sizex / 2;
			int adjustedy = y - sizey / 2;

			Canvas.SetLeft(this, adjustedx);
			Canvas.SetTop(this, adjustedy);
		}

		public UIElement UI
		{
			get { return this; }
			set { }
		}

		public bool Visible
		{
			get
			{
				return this.Visibility == System.Windows.Visibility.Visible;
			}
			set
			{
				this.Visibility = value ? System.Windows.Visibility.Visible : System.Windows.Visibility.Collapsed;
			}
		}

		public TranslateTransform Transform
		{
			get
			{
				if (this.RenderTransform == null) return null;
				return (TranslateTransform)this.RenderTransform;
			}
			set
			{
				this.RenderTransform = value;
			}
		}

		#endregion
	}

	class Trail : Map.ILocationPath
	{
		private Point? start;
		private PathSegmentCollection segments;
		private Path path;
		private KeyValuePair<Shape, Shape> nearest;

		public Trail()
		{
			this.path = new Path();
			this.path.Stroke = Brushes.DarkSlateBlue;
			this.path.StrokeThickness = 3;

			{
				Shape[] waypoints = new Shape[2];
				for (int i = 0; i < 2; ++i) {
					var waypoint = new Ellipse();
					waypoint.Width = 12;
					waypoint.Height = 12;
					waypoint.Fill = Brushes.OrangeRed;
					waypoint.Stroke = Brushes.OrangeRed;
					waypoint.StrokeThickness = 3;
					waypoint.Visibility = Visibility.Collapsed;

					waypoints[i] = waypoint;
				}
				this.nearest = new KeyValuePair<Shape, Shape>(waypoints[0], waypoints[1]);
			}

			this.path.Initialized += new EventHandler(path_Initialized);
			this.path.MouseEnter += new MouseEventHandler(Path_MouseEnter);
			this.path.MouseLeave += new MouseEventHandler(Path_MouseLeave);
			this.path.MouseMove += new MouseEventHandler(path_MouseMove);
		}

		void path_Initialized(object sender, EventArgs e)
		{
			var canvas = (Canvas)this.path.Parent;
			canvas.Children.Add(this.nearest.Key);
			canvas.Children.Add(this.nearest.Value);

			var zindex = Canvas.GetZIndex(this.path);
			Canvas.SetZIndex(this.nearest.Key, zindex - 1);
			Canvas.SetZIndex(this.nearest.Value, zindex - 1);
		}

		void Path_MouseLeave(object sender, MouseEventArgs e)
		{
			this.path.Stroke = new LinearGradientBrush(Color.FromArgb(50, 255, 0, 0), Color.FromArgb(50, 0, 0, 255), 0.0);
			this.nearest.Key.Visibility = Visibility.Collapsed;
			this.nearest.Value.Visibility = Visibility.Collapsed;
		}

		void Path_MouseEnter(object sender, MouseEventArgs e)
		{
			this.path.Stroke = new LinearGradientBrush(Color.FromArgb(255, 255, 0, 0), Color.FromArgb(255, 0, 0, 255), 90.0);
			this.nearest.Key.Visibility = Visibility.Visible;
			this.nearest.Value.Visibility = Visibility.Visible;
		}

		void path_MouseMove(object sender, MouseEventArgs e)
		{
			var pos = e.GetPosition(this.path);
			int index = this.CalculateBasePoint(pos);
			if (index == -1) return;

			var first = this.GetLocation(index);
			var second = this.GetLocation(index + 1);

			Canvas.SetLeft(this.nearest.Key, first.X - 6);
			Canvas.SetTop(this.nearest.Key, first.Y - 6);
			Canvas.SetLeft(this.nearest.Value, second.X - 6);
			Canvas.SetTop(this.nearest.Value, second.Y - 6);
		}

		int CalculateBasePoint(Point chk)
		{
			int numpoints = 1 + this.segments.Count;
			for (int i = 0; i < numpoints - 1; ++i) {
				var bp = this.GetLocation(i);
				var ep = this.GetLocation(i + 1);

				if (!this.IsWithIn(bp.X, ep.X, chk.X)) continue;
				if (!this.IsWithIn(bp.Y, ep.Y, chk.Y)) continue;

				if (this.IsCloseTo(bp, ep, chk, 3)) return i;
			}
			return -1;
		}

		Point GetLocation(int index)
		{
			if (index == 0) return this.start.Value;
			return ((LineSegment)this.segments[index - 1]).Point;
		}

		bool IsWithIn(double pt1, double pt2, double chk)
		{
			if (pt1 <= pt2) return pt1 <= chk && chk <= pt2;
			else return pt2 <= chk && chk <= pt1;
		}

		bool IsCloseTo(Point pt1, Point pt2, Point chk, double threshold)
		{
			double a, b, c;
			if (pt2.X == pt1.X) {
				a = 1;
				b = 0;
				c = -pt2.X;
			}
			else {
				double delta = (pt2.Y - pt1.Y) / (pt2.X - pt1.X);
				a = delta;
				b = -1;
				c = pt2.Y - delta * pt2.X;
			}

			double dist = Math.Abs(a * chk.X + b * chk.Y + c) / Math.Sqrt(a * a + b * b);
			return dist < threshold;
		}

		#region ILocationPath Members

		public void StartOver(int numlocations)
		{
			this.start = null;
			this.segments = null;
		}

		public void AddPixelPair(int x, int y)
		{
			if (!this.start.HasValue)
				this.start = new Point(x, y);
			else {
				if (this.segments == null)
					this.segments = new PathSegmentCollection();
				this.segments.Add(new LineSegment(new Point(x, y), true));
			}
		}

		public void EndPoint()
		{
			var pathfig = new PathFigure();
			pathfig.StartPoint = this.start.Value;
			pathfig.Segments = this.segments;

			var pathfigcoll = new PathFigureCollection();
			pathfigcoll.Add(pathfig);

			var pathgeo = new PathGeometry();
			pathgeo.Figures = pathfigcoll;

			this.path.Data = pathgeo;
		}

		public UIElement UI
		{
			get { return this.path; }
			set { }
		}

		public bool Visible
		{
			get
			{
				return this.path.Visibility == System.Windows.Visibility.Visible;
			}
			set
			{
				this.path.Visibility = value ? System.Windows.Visibility.Visible : System.Windows.Visibility.Collapsed;
			}
		}

		public TranslateTransform Transform
		{
			get
			{
				if (this.path.RenderTransform == null) return null;
				return (TranslateTransform)this.path.RenderTransform;
			}
			set
			{
				this.path.RenderTransform = value;
				this.nearest.Key.RenderTransform = value;
				this.nearest.Value.RenderTransform = value;
			}
		}

		#endregion
	}

	public class WPFMapControl : Map.MapControl
	{
		public WPFMapControl()
		{
			string reposroot = @"h:\gitdev\lindsay\winview\";
			int zoomlevel = 13;
			int width = 800;
			int height = 600;
			double lon = -88.228411;
			double lat = 40.110539;
			int numtileslimit = 100;
			this.Initialize(reposroot, zoomlevel, width, height, lon, lat, numtileslimit);

			var panel = this.ContentGrid;

			// Navigation control
			{
				var ctrl = new NavigationControl();
				ctrl.ViewModeChanged += new NavigationControl.ViewModeChangedHandler(Control_ViewModeChanged);
				Canvas.SetLeft(ctrl, 0);
				Canvas.SetTop(ctrl, 0);
				panel.Children.Add(ctrl);
			}
		}

		void Control_ViewModeChanged(object sender, bool roadview)
		{
			this.ViewMode = roadview ? Map.MapStyle.ROAD : Map.MapStyle.HYBRID;
		}
	}
}
