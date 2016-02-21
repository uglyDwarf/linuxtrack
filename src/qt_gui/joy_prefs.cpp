
#include "joy_prefs.h"
#include "joy_driver_prefs.h"
#include "ltr_gui_prefs.h"
#include "utils.h"
#include <iostream>
#include <map>

#include "dyn_load.h"

typedef bool (*enum_axes_t)(ifc_type_t ifc, const char *name, axes_t *axes);
typedef void (*free_axes_t)(axes_t axes);
typedef joystickNames_t *(*enum_joysticks_t)(ifc_type_t ifc);
typedef void (*free_joysticks_t)(joystickNames_t *nl);

static enum_axes_t enum_axes_fun = NULL;
static free_axes_t free_axes_fun = NULL;
static enum_joysticks_t enum_joysticks_fun = NULL;
static free_joysticks_t free_joysticks_fun = NULL;

static lib_fun_def_t functions[] = {
  {(char *)"ltr_int_joy_enum_axes", (void*) &enum_axes_fun},
  {(char *)"ltr_int_joy_free_axes", (void*) &free_axes_fun},
  {(char *)"ltr_int_joy_enum_joysticks", (void*) &enum_joysticks_fun},
  {(char *)"ltr_int_joy_free_joysticks", (void*) &free_joysticks_fun},
  {NULL, NULL}
};

void *JoyPrefs::libhandle = NULL;

JoyPrefs::JoyPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id), initializing(false)
{
  ui.setupUi(this);
  Activate(id, true);
}

JoyPrefs::~JoyPrefs()
{
  if(libhandle){
    //printf("Unloading library....\n");
    ltr_int_unload_library(libhandle, functions);
    libhandle = NULL;
  }
}

static std::map<int, QString> axisMap;
static std::map<int, int> axisComboMap;
static bool enumerateAxes(ifc_type_t ifc, const QString &joyName)
{
  axes_t axes;
  bool res = enum_axes_fun(ifc, joyName.toUtf8().constData(), &axes);
  if(!res){
    return false;
  }
  axisMap.insert(std::pair<int, QString>(-1, QString::fromUtf8("-1")));
  for(size_t i = 0; i < axes.axes; ++i){
    axisMap.insert(std::pair<int, QString>(axes.axesList[i], QString::fromUtf8(axes.axisNames[i])));
  }
  return true;
}


bool JoyPrefs::Activate(const QString &ID, bool init)
{
  if(libhandle == NULL){
    //printf("Libhandle NULL...\n");
    //printf("Loading libjoy...\n");
    libhandle = ltr_int_load_library((char *)"libjoy", functions);
    //printf("Libjoy loaded? %p\n", libhandle);
    if(libhandle == NULL){
      return false;
    }
  }
  QString sec;
  initializing = init;
  QString joyId = ID;
  ltr_int_log_message("Activating joystick ID: '%s'.\n", joyId.toUtf8().constData());
  if(PREF.getFirstDeviceSection(QString::fromUtf8("Joystick"), joyId, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
    if(!ltr_int_joy_init_prefs()){
      initializing = false;
      ltr_int_log_message("Couldn't initialize joystick preferences.\n");
      return false;
    }
  }else{
    sec = QString::fromUtf8("Joystick");
    initializing = false;
    if(PREF.createSection(sec)){
      if(!ltr_int_joy_init_prefs()){
        initializing = false;
        ltr_int_log_message("Couldn't initialize joystick preferences.\n");
        return false;
      }
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device"), QString::fromUtf8("Joystick"));
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device-id"), ID);
      PREF.addKeyVal(sec, QString::fromUtf8("Pitch-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("Yaw-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("Roll-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("TX-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("TY-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("TZ-Axis"), QString::number(-1));
      PREF.addKeyVal(sec, QString::fromUtf8("Interface"), QString::fromUtf8("Evdev"));
      PREF.addKeyVal(sec, QString::fromUtf8("Angle"), QString::number(ltr_int_joy_get_angle_base()));
      PREF.addKeyVal(sec, QString::fromUtf8("Trans"), QString::number(ltr_int_joy_get_trans_base()));
      PREF.addKeyVal(sec, QString::fromUtf8("PollsPerSecond"),
                     QString::number(ltr_int_joy_get_pps()));
      PREF.activateDevice(sec);
    }
  }

  if(!enumerateAxes(e_JS, joyId)){
    ui.JsButton->setEnabled(false);
  }
  axisMap.clear();
  if(!enumerateAxes(e_EVDEV, joyId)){
    ui.EvdevButton->setEnabled(false);
  }
  axisMap.clear();

  if(!enumerateAxes(ltr_int_joy_get_ifc(), joyId)){
    initializing = false;
    ltr_int_log_message("Couldn't enumerate joystick axes.\n");
    return false;
  }
  std::map<int, QString>::iterator a;
  int cntr = 1;
  axisComboMap.insert(std::pair<int, int>(-1, 0));
  for(a = axisMap.begin(); a != axisMap.end(); ++a){
    ui.PitchCombo->addItem(a->second, QVariant(a->first));
    ui.YawCombo->addItem(a->second, QVariant(a->first));
    ui.RollCombo->addItem(a->second, QVariant(a->first));
    ui.TXCombo->addItem(a->second, QVariant(a->first));
    ui.TYCombo->addItem(a->second, QVariant(a->first));
    ui.TZCombo->addItem(a->second, QVariant(a->first));
    axisComboMap.insert(std::pair<int, int>(a->first, cntr++));
  }
  ui.PitchCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_pitch_axis()]);
  ui.YawCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_yaw_axis()]);
  ui.RollCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_roll_axis()]);
  ui.TXCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_tx_axis()]);
  ui.TYCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_ty_axis()]);
  ui.TZCombo->setCurrentIndex(axisComboMap[ltr_int_joy_get_tz_axis()]);
  if(ltr_int_joy_get_ifc() == e_JS){
    ui.JsButton->setChecked(true);
  }else{
    ui.EvdevButton->setChecked(true);
  }
  initializing = false;
  return true;
}


static bool enumerateJoysticks(QStringList &joyList)
{
  bool res = true;
  joystickNames_t *evdev_names;
  joystickNames_t *js_names;
  QString devName;

  evdev_names = enum_joysticks_fun(e_EVDEV);
  js_names = enum_joysticks_fun(e_JS);

  for(size_t i = 0; i < js_names->namesFound; ++i){
    ltr_int_log_message("JS: %s\n", js_names->nameList[i]);
    devName = QString::fromUtf8(js_names->nameList[i]);
    if(!joyList.contains(devName)){
      joyList<<devName;
    }
  }

  for(size_t i = 0; i < evdev_names->namesFound; ++i){
    ltr_int_log_message("EVDEV: %s\n", evdev_names->nameList[i]);
    devName = QString::fromUtf8(evdev_names->nameList[i]);
    if(!joyList.contains(devName)){
      joyList<<devName;
    }
  }

  free_joysticks_fun(js_names);
  free_joysticks_fun(evdev_names);

  return res;
}

bool JoyPrefs::AddAvailableDevices(QComboBox &combo)
{
  if(libhandle == NULL){
    //printf("Libhandle NULL...\n");
    //printf("Loading libjoy...\n");
    libhandle = ltr_int_load_library((char *)"libjoy", functions);
    //printf("Libjoy loaded? %p\n", libhandle);
    if(libhandle == NULL){
      return false;
    }
  }
  //printf("Enumerating...\n");
  QStringList joyList;
  bool res = enumerateJoysticks(joyList);

  QString id, sec, ifcString;
  deviceType_t dt;
  bool joy_selected = false;
  if(PREF.getActiveDevice(dt,id, sec) && (dt == JOYSTICK)){
    joy_selected = true;
  }

  PrefsLink *pl;
  QVariant v;
  QStringList::const_iterator i;
  for(i = joyList.begin(); i != joyList.end(); ++i){
    QString comboName = *i;
    pl = new PrefsLink(JOYSTICK, comboName);
    v.setValue(*pl);
    combo.addItem(comboName, v);
    if(joy_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  return res;
}

void JoyPrefs::on_PitchCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_pitch_axis(ui.PitchCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_YawCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_yaw_axis(ui.YawCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_RollCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_roll_axis(ui.RollCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_TXCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_tx_axis(ui.TXCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_TYCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_ty_axis(ui.TYCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_TZCombo_activated(int index)
{
  if(!initializing){
    ltr_int_joy_set_tz_axis(ui.TZCombo->itemData(index).toInt());
  }
}

void JoyPrefs::on_JsButton_pressed()
{
  if(!initializing){
    ltr_int_joy_set_ifc(e_JS);
  }
}

void JoyPrefs::on_EvdevButton_pressed()
{
  if(!initializing){
    ltr_int_joy_set_ifc(e_EVDEV);
  }
}

void JoyPrefs::on_PPSFreq_valueChanged(int i)
{
  if(!initializing){
    ltr_int_joy_set_pps(i);
  }
}
