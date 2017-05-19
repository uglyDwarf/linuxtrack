//to get asprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <error.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <linux/input.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>

#include <utils.h>
#include <cal.h>
#include <runloop.h>
#include <joy_driver_prefs.h>

#define NAME_LENGTH 256
#define AXMAP_SIZE (ABS_MAX + 1)

typedef bool procFunc(ifc_type_t ifc, int fd, void *param);


static const char *getAxisName(uint8_t axisID)
{
  static const char *axis_names[ABS_MAX + 1] = {
    "X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder",
    "Wheel", "Gas", "Brake", "Unknown1", "Unknown2", "Unknown3", "Unknown4", "Unknown5",
    "Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat3Y",
    "Unknown6", "Unknown7", "Unknown8", "Unknown9", "Unknown10", "Unknown11", "Unknown12",
  };
  if(axisID > ABS_MAX){
    return NULL;
  }
  return axis_names[axisID];
}

static int normalize(int val, axes_t *axes, uint8_t code)
{
  uint8_t i;
  for(i = 0; i < axes->axes; ++i){
    if(axes->axesList[i] == code){
      break;
    }
  }
  int min = axes->min[i];
  int max = axes->max[i];
  float diff = val - min;
  int res = 32767 * ((2.0 * diff / (max - min)) - 1.0);
  //printf("Val: %d, min: %d, max: %d => %d\n", val, min, max, res);
  return res;
}


static bool enumerateAxesJs(int fd, axes_t *axes)
{
  axes->axes = 0;
  if(ioctl(fd, JSIOCGAXES, &(axes->axes)) < 0){
    ltr_int_my_perror("ioctl(JSIOCGAXES)");
    return false;
  }
  if((axes->axesList = (uint8_t*)malloc(axes->axes * sizeof(uint8_t))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  if((axes->axisNames = (const char**)malloc(axes->axes * sizeof(char*))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  if((axes->min = (int*)malloc(axes->axes * sizeof(int))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  if((axes->max = (int*)malloc(axes->axes * sizeof(int))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  memset(axes->axesList, 0, axes->axes);
  uint8_t axmap[AXMAP_SIZE] = {0};

  if(ioctl(fd, JSIOCGAXMAP, axmap) < 0){
    ltr_int_my_perror("ioctl(JSIOCGAXMAP)");
    return false;
  }
  memcpy(axes->axesList, axmap, axes->axes);
  size_t i;
  for(i = 0; i < axes->axes; ++i){
    axes->min[i] = -32767;
    axes->max[i] = 32767;
    axes->axisNames[i] = getAxisName(axes->axesList[i]);
  }
  return true;
}


static bool enumerateAxesEvdev(int fd, axes_t *axes)
{
  //printf("Enumerating evdev axes\n");
  uint8_t bit[KEY_MAX / 8 + 1];
  memset(bit, 0, sizeof(bit));
  if(ioctl(fd, EVIOCGBIT(0, EV_MAX), bit) < 0){
    ltr_int_my_perror("EVIOCGBIT_1");
    return false;
  }
  int type;

  if(!(bit[0] & (1 << EV_ABS))){
    //printf("No absolute events available.\n");
    return false;
  }
  //printf("We have absolute events available!\n");
  //We have some absolute axes
  memset(bit, 0, sizeof(bit));
  if(ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), bit) < 0){
    ltr_int_my_perror("EVIOCGBIT_2");
    return false;
  }
  //Get axes number
  axes->axes = 0;
  for(type = 0; type < KEY_MAX; ++type){
    if(bit[type >> 3] & (1 << (type & 7))){
      ++axes->axes;
    }
  }
  if((axes->axesList = (uint8_t*)malloc(axes->axes * sizeof(uint8_t))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  memset(axes->axesList, 0, axes->axes);
  if((axes->axisNames = (const char**)malloc(axes->axes * sizeof(char*))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  if((axes->min = (int*)malloc(axes->axes * sizeof(int))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  if((axes->max = (int*)malloc(axes->axes * sizeof(int))) == NULL){
    ltr_int_my_perror("malloc");
    return false;
  }
  size_t current = 0;
  for(type = 0; type < KEY_MAX; ++type){
    if(bit[type >> 3] & (1 << (type & 7))){
      axes->axesList[current] = type;
      axes->axisNames[current] = getAxisName(type);

      struct input_absinfo ai = {0};

      if(ioctl(fd, EVIOCGABS(type), &ai) < 0){
        ltr_int_my_perror("EVIOCGABS");
        free(axes->axesList);
        free(axes->min);
        free(axes->max);
        axes->axesList = NULL;
        axes->min = NULL;
        axes->max = NULL;
        axes->axes = 0;
        return false;
      }

      ltr_int_log_message("Axis: %d (%s)\n", type, getAxisName(type));
      ltr_int_log_message("  Val: %d\n", ai.value);
      ltr_int_log_message("  Minimum: %d\n", ai.minimum);
      ltr_int_log_message("  Maximum: %d\n", ai.maximum);
      ltr_int_log_message("  Fuzz: %d\n", ai.fuzz);
      ltr_int_log_message("  Flat: %d\n", ai.flat);
      ltr_int_log_message("  Resolution: %d\n", ai.resolution);

      axes->min[current] = ai.minimum;
      axes->max[current] = ai.maximum;
      ++current;
    }
  }
  return true;
}

static int findJoystick(ifc_type_t ifc, const char *joystickName);

static bool enumerateAxes(ifc_type_t ifc, int fd, axes_t *axes)
{
  //printf("Going to enumerate axes (%d, %d)\n", ifc, fd);
  if(fd < 0){
    ltr_int_log_message("Invalid descriptor passed to enumerateAxes (%d).", fd);
    return false;
  }
  switch(ifc){
    case e_JS:
      return enumerateAxesJs(fd, axes);
      break;
    case e_EVDEV:
      return enumerateAxesEvdev(fd, axes);
      break;
  }
  return false;
}


bool ltr_int_joy_enum_axes(ifc_type_t ifc, const char *name, axes_t *axes)
{
  //printf("Going to enumerate axes (%d, %s)\n", ifc, name);
  int fd = findJoystick(ifc, name);
  if(fd < 0){
    ltr_int_log_message("Problem finding device '%s', aborting axis enumeration.\n");
    return false;
  }
  return enumerateAxes(ifc, fd, axes);
}

void ltr_int_joy_free_axes(axes_t axes)
{
  free(axes.axesList);
  free(axes.axisNames);
  free(axes.min);
  free(axes.max);
}

static bool isJoyName(ifc_type_t ifc, int fd, void *param)
{
  const char *joystickName = (const char *)param;
  char name[NAME_LENGTH] = "Unknown";
  switch(ifc){
    case e_JS:
      if(ioctl(fd, JSIOCGNAME(NAME_LENGTH), name) < 0){
        ltr_int_my_perror("ioctl(JSIOCGNAME)");
        return false;
      }
      break;
    case e_EVDEV:
      if(ioctl(fd, EVIOCGNAME(NAME_LENGTH), name) < 0){
        ltr_int_my_perror("ioctl(EVIOCGNAME)");
        return false;
      }
      break;
  }
  //printf("Received name '%s'.\n", name);
  size_t max_len = (strlen(joystickName) < NAME_LENGTH) ? strlen(joystickName) : NAME_LENGTH;
  if(strncmp(name, joystickName, max_len) == 0){
    return true;
  }
  return false;
}

static bool arrayBigEnough(void ***ptr, size_t *max_size, size_t *current, size_t fieldSize)
{
  if(*ptr == NULL){
    *ptr = malloc(fieldSize);
    if(*ptr == NULL){
      ltr_int_my_perror("malloc");
      return false;
    }
    *max_size = 1;
    *current = 0;
  }
  if(current >= max_size){
    size_t newLen = (*max_size < (SIZE_MAX / 2)) ? *max_size * 2 : SIZE_MAX;
    void **tmp = realloc(*ptr, newLen * fieldSize);
    if(tmp == NULL){
      ltr_int_my_perror("realloc");
      return false;
    }
    *ptr = tmp;
    *max_size = newLen;
  }
  assert(*current < *max_size);
  return true;
}



static bool storeJoyNames(ifc_type_t ifc, int fd, void *param)
{
  joystickNames_t *jsNames = (joystickNames_t *)param;

  char name[NAME_LENGTH] = "Unknown";

  switch(ifc){
    case e_JS:
      if(ioctl(fd, JSIOCGNAME(NAME_LENGTH), name) < 0){
        ltr_int_my_perror("ioctl(JSIOCGNAME)");
        return false;
      }
      break;
    case e_EVDEV:
      if(ioctl(fd, EVIOCGNAME(NAME_LENGTH), name) < 0){
        ltr_int_my_perror("ioctl(EVIOCGNAME)");
        return false;
      }
      break;
  }

  if(!arrayBigEnough((void ***)&(jsNames->nameList), &(jsNames->nameListSize), &(jsNames->namesFound), sizeof(char *))){
    return false;
  }

  jsNames->nameList[jsNames->namesFound] = strdup(name);
  ++jsNames->namesFound;
  return false;
}

void ltr_int_joy_free_joysticks(joystickNames_t *nl)
{
  size_t i;
  for(i = 0; i < nl->namesFound; ++i){
    free(nl->nameList[i]);
  }
  free(nl->nameList);
}

int enumerateJoyFiles(ifc_type_t ifc, const char *path, procFunc *fun, void *param)
{
  char *nameStart;
  switch(ifc){
    case e_JS:
      nameStart = "js";
      break;
    case e_EVDEV:
      nameStart = "event";
      break;
    default:
      return 0;
  }
  DIR *input = opendir(path);
  if(input == NULL){
    ltr_int_my_perror("opendir");
    return -1;
  }

  struct dirent *de;
  int fd = -1;
  while((de = readdir(input)) != NULL){

    if(strncmp(nameStart, de->d_name, 2) != 0){
      continue;
    }
    char *fname = NULL;
    if(asprintf(&fname, "%s/%s", path, de->d_name) < 0){
      continue;
    }

    fd = open(fname, O_RDONLY | O_NONBLOCK);
    if(fd < 0){
      ltr_int_my_perror("open");
      //printf("Problem opening '%s'.\n", fname);
      free(fname);
      fname = NULL;
      continue;
    }
    //printf("Opened '%s'.\n", fname);
    free(fname);
    fname = NULL;

    if(fun(ifc, fd, param)){
      break;
    }

    close(fd);
    fd = -1;
  }
  closedir(input);
  return fd;
}


/*
 * Finds a joystick according to its name
 *   If found, returns associated file descriptor; -1 if not found
 */
static int findJoystick(ifc_type_t ifc, const char *joystickName)
{
      return enumerateJoyFiles(ifc, "/dev/input", isJoyName, (void *)joystickName);
}

joystickNames_t *ltr_int_joy_enum_joysticks(ifc_type_t ifc)
{
  joystickNames_t *jsNames = (joystickNames_t *)malloc(sizeof(joystickNames_t));
  if(jsNames == NULL){
    return NULL;
  }
  jsNames->nameList = NULL;
  jsNames->namesFound = 0;
  enumerateJoyFiles(ifc, "/dev/input", storeJoyNames, (void *)jsNames);

  size_t i;
  for(i = 0; i < jsNames->namesFound; ++i){
    ltr_int_log_message("Found joystick named '%s'.\n", jsNames->nameList[i]);
  }
  return jsNames;
}



static ifc_type_t ifc = e_EVDEV;
static int fd = -1;
static axes_t axes;
static float yaw;
static float pitch;
static float roll;
static float tx;
static float ty;
static float tz;
static unsigned int cntr = 0;
static unsigned char bm;
static struct pollfd desc;




int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  if(!ltr_int_joy_init_prefs()){
    ltr_int_log_message("Can't initialize joystick preferences.\n");
    return -1;
  }
  const char *devName = ccb->device.device_id;
  fd = findJoystick(ifc, devName);
  if(fd < 0){
    ltr_int_log_message("Device '%s' not found.\n", devName);
    return -1;
  }
  //Js has all the axes normalized to +-32767, but evdev reports
  //  them in their native resolutions; enumerating them allows me
  //  to normalize them to +-32767 too...
  if(ifc == e_EVDEV){
    if(!enumerateAxes(e_EVDEV, fd, &axes)){
      ltr_int_log_message("Couldn't enumerate axes.\n");
      return -1;
    }
  }
  yaw = pitch = roll = tx = ty = tz = 0.0f;
  cntr = 0;
  bm = 0;
  return 0;
}

int ltr_int_tracker_close()
{
  if(ifc == e_EVDEV){
    ltr_int_joy_free_axes(axes);
  }
  close(fd);
  return 0;
}

int ltr_int_tracker_pause()
{
  //no need to do anything
  return 0;
}

int ltr_int_tracker_resume()
{
  //no need to do anything
  return 0;
}

static int mapAxis(uint8_t axisNumber, int value)
{
  if(axisNumber == ltr_int_joy_get_pitch_axis()){
    pitch = value * (ltr_int_joy_get_angle_base() / 32767);
    //printf("Pitch: %f\n", pitch);
  }else if(axisNumber == ltr_int_joy_get_yaw_axis()){
    yaw = value * (ltr_int_joy_get_angle_base() / 32767);
  }else if(axisNumber == ltr_int_joy_get_roll_axis()){
    roll = value * (ltr_int_joy_get_angle_base() / 32767);
  }else if(axisNumber == ltr_int_joy_get_tx_axis()){
    tx = value * (ltr_int_joy_get_trans_base() / 32767);
  }else if(axisNumber == ltr_int_joy_get_ty_axis()){
    ty = value * (ltr_int_joy_get_trans_base() / 32767);
  }else if(axisNumber == ltr_int_joy_get_tz_axis()){
    tz = value * (ltr_int_joy_get_trans_base() / 32767);
  }
  return 0;
}


int ltr_int_tracker_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f, bool *frame_acquired)
{
  (void) ccb;
  struct input_event event[16];
  struct js_event js[16];
  size_t i;
  desc.fd = fd;
  desc.events = POLLIN;
  desc.revents = 0;
  ssize_t res;
  int poll_res = poll(&desc, 1, 1000 / ltr_int_joy_get_pps());
  if(poll_res < 0){
    ltr_int_my_perror("poll");
    return -1;
  }else if(poll_res > 0){
    //timeout
    switch(ifc){
      case e_JS:
        res = read(fd, js, sizeof(js));
        if((res < 0) || (res % sizeof(struct js_event) != 0)){
          ltr_int_my_perror("read");
          return -1;
        }
        for(i = 0; i < res / sizeof(struct js_event); ++i){
          if((js[i].type & ~JS_EVENT_INIT) == JS_EVENT_AXIS){
            mapAxis(js[i].number, js[i].value);
          }
        }
        break;
      case e_EVDEV:
          res = read(fd, &event, sizeof(event));
          if((res < 0) || (res % sizeof(struct input_event) != 0)){
            ltr_int_my_perror("read");
            return -1;
          }
          for(i = 0; i < res / sizeof(struct input_event); ++i){
            if(event[i].type == EV_ABS){
              mapAxis(event[i].code, normalize(event[i].value, &axes, event[i].code));
            }
          }
        break;
    }
  }

  f->bloblist.num_blobs = 3;
  f->bloblist.blobs[0].x = yaw;
  f->bloblist.blobs[0].y = pitch;
  f->bloblist.blobs[1].x = roll;
  f->bloblist.blobs[1].y = tx;
  f->bloblist.blobs[2].x = ty;
  f->bloblist.blobs[2].y = tz;
  f->width = 1;
  f->height = 1;
  f->counter = cntr++;
  *frame_acquired = true;
  //f->bitmap = &bm;

  //printf("Yaw: %f     Pitch: %f     Roll: %f\n", yaw, pitch, roll);
  //printf("X: %f       Y: %f         Z: %f\n", tx, ty, tz);

  return 0;
}

