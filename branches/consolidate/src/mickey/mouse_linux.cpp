#include "mouse.h"
#include "uinput_ifc.h"
#include <QMutex>
#include <QMessageBox>

struct mouseLocalData{
  mouseLocalData(): fd(-1){};
  int fd;
  QMutex mutex;
};


mouseClass::mouseClass(){
  data = new mouseLocalData();
}
  
mouseClass::~mouseClass(){
  close_uinput(data->fd);
  data->fd = -1;
  delete data;
}

bool mouseClass::init()
{
  char *fileName;
  bool permProblem = false;
  data->fd = open_uinput(&fileName, &permProblem);
  if(data->fd == -1){
    QMessageBox::critical(NULL, "Error Creating Virtual Mouse", 
      QString("There was a problem accessing the file \"%1\"\n\
      Please check that you have the right to write to it.").arg(fileName));
    return false;
  }
  return create_device(data->fd);
}

bool mouseClass::move(int dx, int dy)
{
  if(data->fd == -1){
    return false;
  }
  data->mutex.lock();
  movem(data->fd, dx,dy);
  data->mutex.unlock();
  return true;
}

bool mouseClass::click(buttons_t buttons, struct timeval ts)
{
  if(data->fd == -1){
    return false;
  }
  data->mutex.lock();
  clickm(data->fd, buttons, ts);
  data->mutex.unlock();
  return true;
}


