##
##  Nasal code for using an external head position and orientation tracker.
##
##  This file is licensed under the GPL license version 2 or later.
##    Copyright (C) 2007 - 2009  Anders Gidenstam
##
##  Modified by Alexander Pravdin <aledin@evpatoria.com.ua> for linux-track.
##
##  For the original Anders Gidenstam's version of this file please visit
##  http://www.gidenstam.org/FlightGear/HeadTracking/ 
##
## Installation:
## - Put this file in ~/.fgfs/Nasal
## - Start FlightGear with the parameters
##    --generic=socket,in,100,,6543,udp,linux-track --prop:/sim/linux-track/enabled=1


##
# linux-track view handler class.
# Use one instance per tracked view.
#
var ltr_view_handler = {
	
	new : func {
		return { parents: [ltr_view_handler] };
	},
    

	init : func {
	
		if (contains(me, "enabled"))
			return;

		me.enabled  = props.globals.getNode("/sim/linux-track/enabled", 1);
		me.data     = props.globals.getNode("/sim/linux-track/data", 1);
		me.view     = props.globals.getNode("/sim/current-view", 1);
		me.conf     = props.globals.getNode("/sim/current-view/config", 1);
		
		# Low pass filters.
		me.h_lowpass = aircraft.lowpass.new(0.1);
		me.p_lowpass = aircraft.lowpass.new(0.1);
		me.r_lowpass = aircraft.lowpass.new(0.1);
		me.x_lowpass = aircraft.lowpass.new(0.1);
		me.y_lowpass = aircraft.lowpass.new(0.1);
		me.z_lowpass = aircraft.lowpass.new(0.1);

		# Default values
		me.z_default = asnum(me.conf.getChild("default-field-of-view-deg").getValue());

	},


	start: func {
		if (me.enabled.getValue() == 0)
			return 0;

		me.z_default = asnum(me.conf.getChild("default-field-of-view-deg").getValue());
	},


	reset : func {
	
		var h = me.h_lowpass.filter(0);
		var p = me.p_lowpass.filter(0);
		var r = me.r_lowpass.filter(0);
		var x = me.x_lowpass.filter(0);
		var y = me.y_lowpass.filter(0);
		#var z = me.z_lowpass.filter(0);
		var z = me.conf.getChild("default-field-of-view-deg").getValue();
		
		me.view.getChild("goal-heading-offset-deg").setValue(h);
		me.view.getChild("goal-pitch-offset-deg").setValue(p);
		me.view.getChild("goal-roll-offset-deg").setValue(r);
		me.view.getChild("target-x-offset-m").setValue(x);
		me.view.getChild("target-y-offset-m").setValue(y);
		#me.view.getChild("target-z-offset-m").setValue(z);
		me.view.getChild("field-of-view").setValue(z);
	},


	update : func {
	
		if (me.enabled.getValue() == 0)
			return 0;

		var hm = -1;
		var pm =  1;
		var rm =  1;
		var xm =  1;
		var ym =  1;
		var zm =  1;

		var view_name = me.view.getChild("name").getValue();

		if ((view_name == "Helicopter View") or
		    (view_name == "Chase View") or
		    (view_name == "Chase View Without Yaw")) {
			pm = -1;
		}
	
		var h = me.h_lowpass.filter(hm * asnum(me.data.getChild("heading").getValue()));
		var p = me.p_lowpass.filter(pm * asnum(me.data.getChild("pitch").getValue()));
		var r = me.r_lowpass.filter(rm * asnum(me.data.getChild("roll").getValue()));
		var x = me.x_lowpass.filter(xm * asnum(me.data.getChild("tx").getValue()));
		var y = me.y_lowpass.filter(ym * asnum(me.data.getChild("ty").getValue()));
		#var z = me.z_lowpass.filter(zm * asnum(me.data.getChild("tz").getValue()));
		var z = me.z_lowpass.filter(me.z_default + (zm * asnum(me.data.getChild("tz").getValue())));

		me.view.getChild("goal-heading-offset-deg").setValue(h);
		me.view.getChild("goal-pitch-offset-deg").setValue(p);
		me.view.getChild("goal-roll-offset-deg").setValue(r);
		me.view.getChild("target-x-offset-m").setValue(x);
		me.view.getChild("target-y-offset-m").setValue(y);
		#me.view.getChild("target-z-offset-m").setValue(z);
		me.view.getChild("field-of-view").setValue(z < 0 ? 0 : (z > 120 ? 120 : z));

		return 0;
	}
};


var asnum = func(n) {
	if(typeof(n) == "scalar") {
		return num(n);
	} else {
		return 0;
	}
}


if (getprop("/sim/linux-track/enabled")) {

	foreach (var v; view.views) {

		var view_name = v.getChild("name").getValue();

		if ((view_name == "Model View") or
		    (view_name == "Fly-By View")) {
			continue;
		}

		view.manager.register(v.getIndex(), ltr_view_handler);
	}
}
