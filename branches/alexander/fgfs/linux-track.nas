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
		
		# Low pass filters.
		me.h_lowpass = aircraft.lowpass.new(0.1);
		me.p_lowpass = aircraft.lowpass.new(0.1);
		me.r_lowpass = aircraft.lowpass.new(0.1);
#		me.x_lowpass = aircraft.lowpass.new(0.1);
#		me.y_lowpass = aircraft.lowpass.new(0.1);
#		me.z_lowpass = aircraft.lowpass.new(0.1);

    },


	reset : func {
	
		var h = me.h_lowpass.filter(0);
		var p = me.p_lowpass.filter(0);
		var r = me.r_lowpass.filter(0);
#		var x = me.x_lowpass.filter(0);
#		var y = me.y_lowpass.filter(0);
#		var z = me.z_lowpass.filter(0);

		me.view.getChild("goal-heading-offset-deg").setValue(h);
		me.view.getChild("goal-pitch-offset-deg").setValue(p);
		me.view.getChild("goal-roll-offset-deg").setValue(r);
#		me.view.getChild("target-x-offset-m").setValue(x);
#		me.view.getChild("target-y-offset-m").setValue(y);
#		me.view.getChild("target-z-offset-m").setValue(z);
	},


	update : func {
	
		if (me.enabled.getValue()) {
		
			var h = me.h_lowpass.filter(-1 * asnum(me.data.getChild("heading").getValue()));
			var p = me.p_lowpass.filter( 1 * asnum(me.data.getChild("pitch").getValue()));
			var r = me.r_lowpass.filter( 1 * asnum(me.data.getChild("roll").getValue()));
#			var x = me.x_lowpass.filter( 1 * asnum(me.data.getChild("tx").getValue()));
#			var y = me.y_lowpass.filter( 1 * asnum(me.data.getChild("ty").getValue()));
#			var z = me.z_lowpass.filter( 1 * asnum(me.data.getChild("tz").getValue()));

			me.view.getChild("goal-heading-offset-deg").setValue(h);
			me.view.getChild("goal-pitch-offset-deg").setValue(p);
			me.view.getChild("goal-roll-offset-deg").setValue(r);
#			me.view.getChild("target-x-offset-m").setValue(x);
#			me.view.getChild("target-y-offset-m").setValue(y);
#			me.view.getChild("target-z-offset-m").setValue(z);
		}
		
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
    if (view.indexof("Cockpit View") != nil)
        view.manager.register("Cockpit View", ltr_view_handler);
    if (view.indexof("Pilot View") != nil)
        view.manager.register("Pilot View", ltr_view_handler);
    if (view.indexof("Copilot View") != nil)
        view.manager.register("Copilot View", ltr_view_handler);
}
