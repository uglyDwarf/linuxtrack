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
##  Installation:
##
## - Put this file in ~/.fgfs/Nasal
## - Start FlightGear with the parameters:
##
##    --generic=socket,in,100,,6543,udp,linux-track \
##    --prop:/sim/linux-track/enabled=1
##
##  Note that X, Y, Z are not enabled by default. You can enable all of them
##  at once with the additional parameter:
##
##    --prop:/sim/linux-track/track-all=1
##
##  or any of them one by one with such parameters:
##
##    --prop:/sim/linux-track/track-x=1
##    --prop:/sim/linux-track/track-y=1
##    --prop:/sim/linux-track/track-z=1
##

var script_name = "linux-track.nas";

##
# linux-track view handler class.
# Use one instance per tracked view.
#
var ltr_view_handler = {};


ltr_view_handler.new = func {	
	return { parents: [ltr_view_handler] };
};


ltr_view_handler.init = func {	

	if (contains(me, "enabled"))
		return;

	var lt_tree = "/sim/linux-track";
	var cv_tree = "/sim/current-view";

	me.enabled   = props.globals.getNode(lt_tree ~ "/enabled", 1);
	me.data      = props.globals.getNode(lt_tree ~ "/data", 1);
	me.view      = props.globals.getNode(cv_tree, 1);
	me.vcfg      = props.globals.getNode(cv_tree ~ "/config", 1);

	me.track_all = props.globals.getNode(lt_tree ~ "/track-all", 1);
	if (me.track_all.getValue() == 1) {
		setprop(lt_tree ~ "/track-x", 1);
		setprop(lt_tree ~ "/track-y", 1);
		setprop(lt_tree ~ "/track-z", 1);
	}
	me.track_X   = props.globals.getNode(lt_tree ~ "/track-x", 1);
	me.track_Y   = props.globals.getNode(lt_tree ~ "/track-y", 1);
	me.track_Z   = props.globals.getNode(lt_tree ~ "/track-z", 1);

	var default_fov_name = "default-field-of-view-deg";
		
	# FlightGear target properties names
	var fg_H_name = "goal-heading-offset-deg";
	var fg_P_name = "goal-pitch-offset-deg";
	var fg_R_name = "goal-roll-offset-deg";
	var fg_X_name = "target-x-offset-m";
	var fg_Y_name = "target-y-offset-m";
	var fg_Z_name = "field-of-view";

	# Linux-track data properties names
	var lt_H_name = "heading";
	var lt_P_name = "pitch";
	var lt_R_name = "roll";
	var lt_X_name = "tx";
	var lt_Y_name = "ty";
	var lt_Z_name = "tz";

	# Low pass filters
	var lowpass_H = aircraft.lowpass.new(0.1);
	var lowpass_P = aircraft.lowpass.new(0.1);
	var lowpass_R = aircraft.lowpass.new(0.1);
	var lowpass_X = aircraft.lowpass.new(0.1);
	var lowpass_Y = aircraft.lowpass.new(0.1);
	var lowpass_Z = aircraft.lowpass.new(0.1);

	# Limiter for Z
	var limit_Z = func(z) {
		z = asnum(z);
		return (z < 0 ? 0 : (z > 120 ? 120 : z));
	};


	var set_fg_prop = func(name, val) {
		me.view.getChild(name).setValue(val);
	};

	var get_lt_prop = func(name) {
		return asnum(me.data.getChild(name).getValue());
	};

	var get_default_fov = func {
		return asnum(me.vcfg.getChild(default_fov_name).getValue());
	};


	me.set_fg_H = func(val) {
		val = lowpass_H.filter(asnum(val));
		set_fg_prop(fg_H_name, val);
	};

	me.set_fg_P = func(val) {
		val = lowpass_P.filter(asnum(val));
		set_fg_prop(fg_P_name, val);
	};

	me.set_fg_R = func(val) {
		val = lowpass_R.filter(asnum(val));
		set_fg_prop(fg_R_name, val);
	};

	me.set_fg_X = func(val) {
		val = lowpass_X.filter(asnum(val));
		set_fg_prop(fg_X_name, val);
	};

	me.set_fg_Y = func(val) {
		val = lowpass_Y.filter(asnum(val));
		set_fg_prop(fg_Y_name, val);
	};

	me.set_fg_Z = func(val) {
		val = lowpass_Z.filter(val + get_default_fov());
		val = limit_Z(val);
		set_fg_prop(fg_Z_name, val);
	};

	me.get_lt_H = func { return get_lt_prop(lt_H_name); };
	me.get_lt_P = func { return get_lt_prop(lt_P_name); };
	me.get_lt_R = func { return get_lt_prop(lt_R_name); };
	me.get_lt_X = func { return get_lt_prop(lt_X_name); };
	me.get_lt_Y = func { return get_lt_prop(lt_Y_name); };
	me.get_lt_Z = func { return get_lt_prop(lt_Z_name); };


	printf("%s: initialized", script_name);
	printf("%s: enabled heading, pitch, roll%s%s%s",
		script_name,
		(me.track_X.getValue() == 1 ? ", x" : ""),
		(me.track_Y.getValue() == 1 ? ", y" : ""),
		(me.track_Z.getValue() == 1 ? ", z" : ""));
	printf("%s: ready to receive data", script_name);
};


ltr_view_handler.reset = func {

	me.set_fg_H(0);
	me.set_fg_P(0);
	me.set_fg_R(0);

	if (me.track_X.getValue() == 1)
		me.set_fg_X(0);
	if (me.track_Y.getValue() == 1)
		me.set_fg_Y(0);
	if (me.track_Z.getValue() == 1)
		me.set_fg_Z(0);
};


ltr_view_handler.update = func {

	if (me.enabled.getValue() == 0)
		return 0;

	var m_H = -1;
	var m_P =  1;
	var m_R =  1;
	var m_X =  1;
	var m_Y =  1;
	var m_Z =  1;

	var view_name = me.view.getChild("name").getValue();

	if ((view_name == "Helicopter View") or
	    (view_name == "Chase View") or
	    (view_name == "Chase View Without Yaw")) {
		m_P = -1;
	}

	me.set_fg_H(m_H * me.get_lt_H());
	me.set_fg_P(m_P * me.get_lt_P());
	me.set_fg_R(m_R * me.get_lt_R());

	if (me.track_X.getValue() == 1)
		me.set_fg_X(m_X * me.get_lt_X());
	if (me.track_Y.getValue() == 1)
		me.set_fg_Y(m_Y * me.get_lt_Y());
	if (me.track_Z.getValue() == 1)
		me.set_fg_Z(m_Z * me.get_lt_Z());

	return 0;
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
