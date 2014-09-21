#include <stdbool.h>
#include <stdio.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "uinput_ifc.h"

static char *alternative_names[] = {"/dev/misc/uinput", "/dev/input/uinput", "/dev/uinput"};

int open_uinput(char **fname, bool *permProblem)
{
  int i, fd = -1;
  *permProblem = false;
  int names = sizeof(alternative_names) / sizeof(char *);
  for(i = 0; i < names; ++i){
    //printf("Checking %s\n", alternative_names[i]);
    fd = open(alternative_names[i], O_WRONLY | O_NONBLOCK);
    if(fd >= 0){
      //printf("Opened %s\n", alternative_names[i]);
      *fname = alternative_names[i];
      return fd;
    }else{
      if(errno == EACCES){
        //printf("Check permissions!\n");
        *fname = alternative_names[i];
        *permProblem = true;
        return -1;
      }
    }
  }
  *fname = NULL;
  return -1;
}

bool create_device(int fd)
{
  struct uinput_user_dev mouse;
  int res = 0;
  memset(&mouse, 0, sizeof(mouse));
  size_t str_len = sizeof(mouse.name);
  strncpy(mouse.name, "Linuxtrack's Mickey", str_len);
  mouse.name[str_len - 1]= '\0';
  //printf("Name: '%s'\n", mouse.name);
  res |= (write(fd, &mouse, sizeof(mouse)) == -1);
  res |= (ioctl(fd, UI_SET_EVBIT, EV_REL) == -1);
  res |= (ioctl(fd, UI_SET_RELBIT, REL_X) == -1);
  res |= (ioctl(fd, UI_SET_RELBIT, REL_Y) == -1);
  res |= (ioctl(fd, UI_SET_EVBIT, EV_KEY) == -1);
  res |= (ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE) == -1);
  res |= (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) == -1);
  res |= (ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT) == -1);
  res |= (ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE) == -1);
  res |= (ioctl(fd, UI_DEV_CREATE, 0) == -1);
  return (res == 0);
}

static int limit(int val, int min, int max)
{
  if(val < min) return min;
  if(val > max) return max;
  return val;
}

bool movem(int fd, int dx, int dy)
{
  int res = 0;
  struct input_event event;
  event.type = EV_REL;
  event.code = REL_X;
  event.value = limit(dx, -100, 100);
  res |= (write(fd, &event, sizeof(event)) == -1);
  event.type = EV_REL;
  event.code = REL_Y;
  event.value = limit(dy, -100, 100);
  res |= (write(fd, &event, sizeof(event)) == -1);
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  res |= (write(fd, &event, sizeof(event)) == -1);
  return (res == 0);
}

bool send_click(int fd, int btn, bool pressed, struct timeval *ts)
{
  int res = 0;
  //printf("Sending click %d@%d\n", btn, pressed);
  //btn ^= 3;
  struct input_event event;
  event.time = *ts;
  event.type = EV_KEY;
  event.code = btn;
  event.value = pressed;
  res |= (write(fd, &event, sizeof(event)) == -1);
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  res |= (write(fd, &event, sizeof(event)) == -1);
  return (res == 0);
}

bool clickm(int fd, buttons_t btns, struct timeval ts)
{
  static int prev_btns = 0;
  //printf("Click: %d / %d\n", prev_btns, btns);
  int changed = btns ^ prev_btns;
  bool res = 0;
  
  if(changed & LEFT_BUTTON){
    res = send_click(fd, BTN_LEFT, (btns & LEFT_BUTTON) != 0, &ts);
  }
  if(changed & RIGHT_BUTTON){
    res = send_click(fd, BTN_RIGHT, (btns & RIGHT_BUTTON) != 0, &ts);
  }
  prev_btns = btns;
  return (res == 0);
}

void close_uinput(int fd)
{
  if(fd >= 0){
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
  }
}

/*
int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  int fd = open_uinput();
  printf("Hello World! %d\n", fd);
  if(fd > 0){
    create_device(fd);
    sleep(1);
    click(fd, 1);
    sleep(1);
    move(fd, 100, 0);
    sleep(1);
    move(fd, 0, 100);
    sleep(1);
    move(fd, -100, 0);
    sleep(1);
    move(fd, 0, -100);
    click(fd, 0);
    sleep(1);
    click(fd, 2);
    sleep(1);
    click(fd, 0);
    
    close_uinput(fd);
  }
  return 0;
}

*/
