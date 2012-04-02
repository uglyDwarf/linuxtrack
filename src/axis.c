#include <assert.h>
#include <string.h>
#include "axis.h"
#include "spline.h"
#include "utils.h"
#include "pref_int.h"

struct axis_def{
  bool enabled;
  splines_def curve_defs;
  splines curves;
  bool valid;
  float l_factor, r_factor;
  float l_limit, r_limit;
  char *prefix;
};

struct lt_axes {
  struct axis_def pitch_axis;
  struct axis_def yaw_axis;
  struct axis_def roll_axis;
  struct axis_def tx_axis;
  struct axis_def ty_axis;
  struct axis_def tz_axis;
};

typedef enum{
  SENTRY1, DEADZONE, LCURV, RCURV, LMULT, RMULT, LIMITS, LLIMIT, RLIMIT, ENABLED, SENTRY_2
}axis_fields;
static const char *fields[] = {NULL, "-deadzone",
				"-left-curvature", "-right-curvature", 
				"-left-multiplier", "-right-multiplier",
				"-limits", "-left-limit", "-right-limit", "-enabled", NULL};
static struct lt_axes ltr_int_axes;
static bool ltr_int_axes_changed_flag = false;

static struct axis_def *get_axis(enum axis_t id)
{
  switch(id){
    case PITCH:
      return &(ltr_int_axes.pitch_axis);
      break;
    case ROLL:
      return &(ltr_int_axes.roll_axis);
      break;
    case YAW:
      return &(ltr_int_axes.yaw_axis);
      break;
    case TX:
      return &(ltr_int_axes.tx_axis);
      break;
    case TY:
      return &(ltr_int_axes.ty_axis);
      break;
    case TZ:
      return &(ltr_int_axes.tz_axis);
      break;
    default:
      assert(0);
      break;
  }
}

static char *get_axis_prefix(enum axis_t id)
{
  switch(id){
    case PITCH:
      return "Pitch";
      break;
    case ROLL:
      return "Roll";
      break;
    case YAW:
      return "Yaw";
      break;
    case TX:
      return "Xtranslation";
      break;
    case TY:
      return "Ytranslation";
      break;
    case TZ:
      return "Ztranslation";
      break;
    default:
      assert(0);
      break;
  }
}

float ltr_int_val_on_axis(enum axis_t id, float x)
{
  struct axis_def *axis = get_axis(id);
  if(!axis->enabled){
    return 0.0f;
  }
  if(!(axis->valid)){
    ltr_int_curve2pts(&(axis->curve_defs), &(axis->curves));
  }
  float mf = x < 0 ? axis->l_factor : axis->r_factor;
  float lim = x < 0 ? axis->l_limit : axis->r_limit;
  if(lim == 0.0) return 0.0;
  x *= mf; //apply factor (sensitivity)
  x /= lim; //normalize to apply the spline
  if(x < -1.0){
    x = -1.0;
  }
  if(x > 1.0){
    x = 1.0;
  }
  return ltr_int_spline_point(&(axis->curves), x) * lim;
}

static void signal_change()
{
  ltr_int_axes_changed_flag = true;
}

static bool save_val_flt(enum axis_t id, axis_fields field, float val)
{
  const char *field_name = ltr_int_my_strcat(get_axis_prefix(id), fields[field]);
  bool res = ltr_int_change_key_flt(ltr_int_get_custom_section_name(), field_name, val);
  free((void*)field_name);
  return res;
}

static bool save_val_str(enum axis_t id, axis_fields field, const char *val)
{
  const char *field_name = ltr_int_my_strcat(get_axis_prefix(id), fields[field]);
  bool res = ltr_int_change_key(ltr_int_get_custom_section_name(), field_name, val);
  free((void*)field_name);
  return res;
}

bool ltr_int_is_symetrical(enum axis_t id)
{
  struct axis_def *axis = get_axis(id);
  
  if((axis->l_factor == axis->r_factor) && 
     (axis->curve_defs.l_curvature == axis->curve_defs.r_curvature) &&
     (axis->l_limit == axis->r_limit)){
    return true;
  }else{
    return false;
  }
}

bool ltr_int_set_axis_param(enum axis_t id, enum axis_param_t param, float val)
{
  struct axis_def *axis = get_axis(id);
  axis->valid = false;
  
  switch(param){
    case AXIS_DEADZONE:
      axis->curve_defs.dead_zone = val;
      save_val_flt(id, DEADZONE, val);
      signal_change();
      break;
    case AXIS_LCURV:
      axis->curve_defs.l_curvature = val;
      save_val_flt(id, LCURV, val);
      signal_change();
      break;
    case AXIS_RCURV: 
      axis->curve_defs.r_curvature = val;
      save_val_flt(id, RCURV, val);
      signal_change();
      break;
    case AXIS_LMULT:
      axis->l_factor = val;
      save_val_flt(id, LMULT, val);
      signal_change();
      break;
    case AXIS_RMULT:
      axis->r_factor = val;
      save_val_flt(id, RMULT, val);
      signal_change();
      break;
    case AXIS_LLIMIT:
      axis->l_limit = val;
      save_val_flt(id, LLIMIT, val);
      signal_change();
      break;
    case AXIS_RLIMIT:
      axis->r_limit = val;
      save_val_flt(id, RLIMIT, val);
      signal_change();
      break;
    default:
      return false;
      break;
  }
  return true;
}

float ltr_int_get_axis_param(enum axis_t id, enum axis_param_t param)
{
  struct axis_def *axis = get_axis(id);
  switch(param){
    case AXIS_DEADZONE: 
      return axis->curve_defs.dead_zone;
      break;
    case AXIS_LCURV:
      return axis->curve_defs.l_curvature;
      break;
    case AXIS_RCURV: 
      return axis->curve_defs.r_curvature;
      break;
    case AXIS_LMULT:
      return axis->l_factor;
      break;
    case AXIS_RMULT:
      return axis->r_factor;
      break;
    case AXIS_LLIMIT:
      return axis->l_limit;
      break;
    case AXIS_RLIMIT:
      return axis->r_limit;
      break;
    default:
      return 0.0;
      break;
  }
}

bool ltr_int_set_axis_bool_param(enum axis_t id, enum axis_param_t param, bool val)
{
  struct axis_def *axis = get_axis(id);

  switch(param){
    case AXIS_ENABLED:
      axis->enabled = val;
      if(val){
        save_val_str(id, ENABLED, "Yes");
      }else{
        save_val_str(id, ENABLED, "No");
      }
      signal_change();
      break;
    default:
      return false;
      break;
  }
  return true;
}

bool ltr_int_get_axis_bool_param(enum axis_t id, enum axis_param_t param)
{
  struct axis_def *axis = get_axis(id);
  switch(param){
    case AXIS_ENABLED:
      return axis->enabled;
      break;
    default:
      return false;
      break;
  }
  return false;
}

void ltr_int_init_axis(struct axis_def *axis, const char *prefix)
{
  assert(axis != NULL);
  axis->valid = false;
  axis->enabled = true;
  axis->prefix = ltr_int_my_strdup(prefix);
  axis->l_factor = 1.0f;
  axis->r_factor = 1.0f;
  axis->r_limit = 50.0f;
  axis->l_limit = 50.0f;
  axis->curve_defs.dead_zone = 0.0f;
  axis->curve_defs.l_curvature = 0.5f;
  axis->curve_defs.r_curvature = 0.5f;
}

void ltr_int_close_axis(enum axis_t id)
{
  struct axis_def *axis = get_axis(id);
  assert(axis != NULL);
  
  free(axis->prefix);
  axis->prefix = NULL;
}


static void set_axis_field(struct axis_def *axis, axis_fields field, float val, enum axis_t id)
{
  assert(axis != NULL);
  axis->valid = false;
  switch(field){
    case(DEADZONE):
      axis->curve_defs.dead_zone = val;
      break;
    case(LCURV):
      axis->curve_defs.l_curvature = val;
      break;
    case(RCURV):
      axis->curve_defs.r_curvature = val;
      break;
    case(LMULT):
      axis->l_factor = val;
      break;
    case(RMULT):
      axis->r_factor = val;
      break;
    case(LLIMIT):
      axis->l_limit = val;
      break;
    case(RLIMIT):
      axis->r_limit = val;
      break;
    case(LIMITS):
      //Set some sensible defaults...
      if((id == PITCH) || (id == ROLL) || (id == YAW)){
        axis->l_limit = 90.0f;
        axis->r_limit = 90.0f;
      }else{
        axis->l_limit = 300.0f;
        axis->r_limit = 300.0f;
      }
      break;
    default:
      assert(0);
      break;
  }
}

bool ltr_int_get_axis(enum axis_t id, struct axis_def *axis)
{
  axis_fields i;
  char *field_name = NULL;
  const char *string;
  float val;
  char *prefix = get_axis_prefix(id);
  assert(prefix != NULL);
  assert(axis != NULL);
  
  ltr_int_init_axis(axis, prefix);
//  axis->prefix = ltr_int_my_strdup(prefix);
  for(i = DEADZONE; i <= ENABLED; ++i){
    field_name = ltr_int_my_strcat(prefix, fields[i]);
    if(i != ENABLED){
      if(ltr_int_get_key_flt(NULL, field_name, &val)){
        set_axis_field(axis, i, val, id);
      }
    }else{
      string = ltr_int_get_key(NULL, field_name);
      if((string == NULL) || (strcasecmp(string, "No") != 0)){
        axis->enabled = true;
      }else{
        axis->enabled = false;
      }
    }
    free(field_name);
    field_name = NULL;
  }
  ltr_int_val_on_axis(id, 0.0f);
  return true;
}

bool ltr_int_axes_changed(bool reset_flag)
{
  bool flag = ltr_int_axes_changed_flag;
  if(reset_flag){
    ltr_int_axes_changed_flag = false;
  }
  return flag;
}

void ltr_int_init_axes()
{
  ltr_int_log_message("Initializing axes!\n");
  bool res = true;
  ltr_int_axes_changed_flag = false;
  res &= ltr_int_get_axis(PITCH, &(ltr_int_axes.pitch_axis));
  res &= ltr_int_get_axis(YAW, &(ltr_int_axes.yaw_axis));
  res &= ltr_int_get_axis(ROLL, &(ltr_int_axes.roll_axis));
  res &= ltr_int_get_axis(TX, &(ltr_int_axes.tx_axis));
  res &= ltr_int_get_axis(TY, &(ltr_int_axes.ty_axis));
  res &= ltr_int_get_axis(TZ, &(ltr_int_axes.tz_axis));
}

void ltr_int_close_axes()
{
  ltr_int_log_message("Closing axes!\n");
  ltr_int_close_axis(PITCH);
  ltr_int_close_axis(ROLL);
  ltr_int_close_axis(YAW);
  ltr_int_close_axis(TX);
  ltr_int_close_axis(TY);
  ltr_int_close_axis(TZ);
}

