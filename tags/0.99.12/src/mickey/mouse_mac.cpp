#include "mouse.h"
#include <QMutex>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <ApplicationServices/ApplicationServices.h>

struct mouseLocalData{
  mouseLocalData():pressed(0){};
  buttons_t pressed;
  QMutex mutex;
};


mouseClass::mouseClass(){
  data = new mouseLocalData();
}
  
mouseClass::~mouseClass(){
  delete data;
}

bool mouseClass::init()
{
  return true;
}

bool mouseClass::move(int dx, int dy)
{
  //Gotcha - when a key is pressed, while moving, emit not mouse moved, but mouse dragged event
  CGEventType event;
  CGEventRef ev_ref;
  CGPoint pos;
  QPoint currentPos = QCursor::pos();
  pos.x = currentPos.x() + dx;
  pos.y = currentPos.y() + dy;
  
  if(data->buttons == 0){
    event = kCGEventMouseMoved;
  }else if(data->buttons & LEFT_BUTTON){
    event = kCGEventLeftMouseDragged;
  }else if(data->buttons & RIGHT_BUTTON){
    event = kCGEventRightMouseDragged;
  }
  ev_ref = CGEventCreateMouseEvent(NULL, event, pos, 0);
  CGEventPost(kCGHIDEventTap, ev_ref);
  CFRelease(ev_ref);
  
  QCursor::setPos(new_x, new_y);
  data->mutex.unlock();
  return true;
}

bool mouseClass::click(buttons_t buttons, struct timeval ts)
{
  data->mutex.lock();
  buttons_t changed = buttons ^ data->buttons;
  CGEventType event;
  CGEventRef ev_ref;
  CGPoint pos;
  QPoint currentPos = QCursor::pos();
  pos.x = currentPos.x();
  pos.y = currentPos.y();
  if(chaged & LEFT_BUTTON){
    if(buttons & LEFT_BUTTON){
      event = kCGEventLeftMouseDown;
    }else{
      event = kCGEventLeftMouseUp;
    }
    ev_ref = CGEventCreateMouseEvent(NULL, event, pos, 0);
    CGEventPost(kCGHIDEventTap, ev_ref);
    CFRelease(ev_ref);
  }
  if(chaged & RIGHT_BUTTON){
    if(buttons & RIGHT_BUTTON){
      event = kCGEventRightMouseDown;
    }else{
      event = kCGEventRightMouseUp;
    }
    CGEventCreateMouseEvent(NULL, event, pos, 0);
    CGEventPost(kCGHIDEventTap, ev_ref);
    CFRelease(ev_ref);
  }
  data->buttons = buttons;
  data->mutex.unlock();
  return true;
}

/*
Possible leads:

https://developer.apple.com/library/mac/#documentation/graphicsimaging/reference/Quartz_Services_Ref/Reference/reference.html
http://stackoverflow.com/questions/11860285/opposit-of-cgdisplaymovecursortopoint
https://developer.apple.com/library/mac/#documentation/Carbon/Reference/QuartzEventServicesRef/Reference/reference.html
http://stackoverflow.com/questions/1483657/performing-a-double-click-using-cgeventcreatemouseevent
*/
