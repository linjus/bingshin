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
	/// Interaction logic for NavigationControl.xaml
	/// </summary>
	public partial class NavigationControl : UserControl
	{
		public NavigationControl()
		{
			InitializeComponent();
		}

		public delegate void ViewModeChangedHandler(object sender, bool roadview);
		public event ViewModeChangedHandler ViewModeChanged;

		private void RadioButton_Checked(object sender, RoutedEventArgs e)
		{
			bool? roadview = null;
			if (sender == this.radioRoadView)
				roadview = true;
			else if (sender == this.radioSatelliteView)
				roadview = false;
			if (!roadview.HasValue) return;

			if (this.ViewModeChanged != null)
				this.ViewModeChanged(this, roadview.Value);
		}
	}
}
