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
      printf("Opened %s\n", alternative_names[i]);
      *fname = alternative_names[i];
      return fd;
    }else{
      if(errno == EACCES){
        printf("Check permissons!\n");
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
  memset(&mouse, 0, sizeof(mouse));
  size_t str_len = sizeof(mouse.name);
  strncpy(mouse.name, "Linuxtrack's Mickey", str_len);
  mouse.name[str_len - 1]= '\0';
  printf("Name: '%s'\n", mouse.name);
  write(fd, &mouse, sizeof(mouse));
  ioctl(fd, UI_SET_EVBIT, EV_REL);
  ioctl(fd, UI_SET_RELBIT, REL_X);
  ioctl(fd, UI_SET_RELBIT, REL_Y);
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE);
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
  ioctl(fd, UI_DEV_CREATE, 0);
  return true;
}

void movem(int fd, int dx, int dy)
{
  struct input_event event;
  
  event.type = EV_REL;
  event.code = REL_X;
  event.value = dx;
  write(fd, &event, sizeof(event));
  event.type = EV_REL;
  event.code = REL_Y;
  event.value = dy;
  write(fd, &event, sizeof(event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  write(fd, &event, sizeof(event));
}

void send_click(int fd, int btn, bool pressed)
{
  struct input_event event;
  gettimeofday(&event.time, NULL);
  event.type = EV_KEY;
  event.code = btn;
  event.value = pressed;
  write(fd, &event, sizeof(event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  write(fd, &event, sizeof(event));
}

void click(int fd, int btns)
{
  static int prev_btns = 0;
  int changed = btns ^ prev_btns;
  
  if(changed & LEFT_BUTTON){
    send_click(fd, BTN_LEFT, (btns & LEFT_BUTTON) != 0);
  }
  if(changed & RIGHT_BUTTON){
    send_click(fd, BTN_RIGHT, (btns & RIGHT_BUTTON) != 0);
  }
  prev_btns = btns;
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