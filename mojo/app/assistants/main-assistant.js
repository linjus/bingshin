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

function MainAssistant() {
	/* this is the creator function for your scene assistant object. It will be passed all the 
	   additional parameters (after the scene name) that were passed to pushScene. The reference
	   to the scene controller (this.controller) has not be established yet, so any initialization
	   that needs the scene controller should be done in the setup function below. */
	this.trackGPS = null;
}

MainAssistant.prototype.setup = function() {
	/* this function is for setup tasks that have to happen when the scene is first created */
		
	/* use Mojo.View.render to render view templates and add them to the scene, if needed */
	
	/* setup widgets here */
	this.commandMenuModel = {
		items: [
			{ label: "Select Map", iconPath: "images/select_map.png", command: "select-map" },
			{
				items: [
					{ label: "Zoom Out", iconPath: "images/zoom_out.png", command: "zoom-out" },
					{ label: "Zoom In", iconPath: "images/zoom_in.png", command: "zoom-in" },
				]
			},
			{
				items: [
					{ label: "Track" },
					{ label: "Locate", iconPath: "images/locate_gps.png", command: "show-gps-location" },
				]			
			},
		]
	};
	this.controller.setupWidget(Mojo.Menu.commandMenu, undefined, this.commandMenuModel);
	this.updateGPSCommandMenu(false);	
	
	/* add event handlers to listen to events from widgets */
};

MainAssistant.prototype.activate = function(event) {
	/* put in event handlers here that should only be in effect when this scene is active. For
	   example, key handlers that are observing the document */
};

MainAssistant.prototype.deactivate = function(event) {
	/* remove any event handlers you added in activate and do any other cleanup that should happen before
	   this scene is popped or another scene is pushed on top */
};

MainAssistant.prototype.cleanup = function(event) {
	/* this function should do any cleanup needed before the scene is destroyed as 
	   a result of being popped off the scene stack */
};

MainAssistant.prototype.handleCommand = function(event) {
	if (event.type == Mojo.Event.command) {
		switch (event.command) {
			case "zoom-out":
				this.zoom(-1);
				break;
			case "zoom-in":
				this.zoom(1);
				break;
			case "start-gps-tracking":
				this.startGPSTracking(true);
				break;
			case "stop-gps-tracking":
				this.startGPSTracking(false);
				break;
			case "show-gps-location":
				this.showGPSLocation();
				break;
		}		
	}
}

MainAssistant.prototype.zoom = function(delta) {
	$('mapctrl').zoom(delta);
}

MainAssistant.prototype.startGPSTracking = function(on) {
	if (on) {
		if (!this.trackGPS) this.toggleGPSTracking();
		else this.showGPSLocation();
	}
	else {
		if (this.trackGPS) this.toggleGPSTracking();
	}
}

MainAssistant.prototype.showGPSLocation = function() {
	$('mapctrl').showGPSLocation();
}

MainAssistant.prototype.updateGPSCommandMenu = function(on) {
	var start = this.commandMenuModel.items[2].items[0];
	var locate = this.commandMenuModel.items[2].items[1];
	
	start.iconPath = on ? "images/tracking_off.png" : "images/tracking_on.png";
	start.command = on ? "stop-gps-tracking" : "start-gps-tracking";
	locate.disabled = !on;
	
	this.controller.modelChanged(this.commandMenuModel);	
}

MainAssistant.prototype.toggleGPSTracking = function() {
	if (this.trackGPS) {
		this.trackGPS.cancel();
		this.trackGPS = null;

		this.updateGPSCommandMenu(false);
		$('mapctrl').showGPS(0);
	}
	else {
		this.trackGPS = this.controller.serviceRequestHandle = new Mojo.Service.Request('palm://com.palm.location', {
			method: 'startTracking',
			parameters: {
				subscribe: true
			},
			onSuccess: this.handleGPSSuccess.bind(this),
			onFailure: this.handleGPSFailure.bind(this)
		});
		
		this.updateGPSCommandMenu(true);
		$('mapctrl').showGPS(1);
	}
}

MainAssistant.prototype.handleGPSSuccess = function(event) {
	lon = event.longitude;
	lat = event.latitude;
	accuracy = event.horizAccuracy;
	
	$('mapctrl').updateGPS(lon, lat, accuracy);
}

MainAssistant.prototype.handleGPSFailure = function(event) {
	$('mapctrl').invalidateGPS(event.errorCode);
}
