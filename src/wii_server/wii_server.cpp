
#include "wii_server.h"
#include <com_proc.h>
#include <wii_com.h>
#include <image_process.h>
#include <utils.h>
#include <ipc_utils.h>
#include <string.h>

#include <iostream>
#include <QTimer>
#include <QMessageBox>

#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768


WiiServerWindow::WiiServerWindow(QWidget *parent) : QWidget(parent), wii(NULL), mm(NULL)
{
  ui.setupUi(this);
  if(ltr_int_initWiiCom(true, &mm)){
    std::cout<<"Wii Server initialized!!!"<<std::endl;
  }else{
    QMessageBox::critical(this, "Wii server initialization failed", 
      "Can't initialize Wii server communication channel;\n Please run Linuxtrack GUI first!");
    exit(1);
  }
  ltr_int_setCommand(mm, STOP);
  wii = new Wiimote(mm);
  cmdTimer = new QTimer(this);
  if(!QObject::connect(this, SIGNAL(connect()), wii, SLOT(connect()))) std::cout << "Sig1 fail!" << std::endl;
  if(!QObject::connect(this, SIGNAL(disconnect()), wii, SLOT(disconnect()))) std::cout << "Sig2 fail!" << std::endl;
  if(!QObject::connect(wii, SIGNAL(changing_state(server_state_t)), this, SLOT(update_state(server_state_t)))) 
    std::cout << "Sig3 fail!" << std::endl;
  if(!QObject::connect(cmdTimer, SIGNAL(timeout()), this, SLOT(handle_command()))) std::cout << "Sig4 fail!" << std::endl;
  
  if(!QObject::connect(this, SIGNAL(pause(int)), wii, SIGNAL(pause(int)))) std::cout << "Sig5 fail!" << std::endl;
  if(!QObject::connect(this, SIGNAL(wakeup(int)), wii, SIGNAL(wakeup(int)))) std::cout << "Sig6 fail!" << std::endl;
  if(!QObject::connect(this, SIGNAL(stop()), wii, SIGNAL(stop()))) std::cout << "Sig7 fail!" << std::endl;
  
  cmdTimer->start(100);
}

WiiServerWindow::~WiiServerWindow()
{
  cmdTimer->stop();
  ltr_int_closeWiiCom();
  delete cmdTimer;
  delete wii;
  wii = NULL;
}

void WiiServerWindow::on_ConnectButton_pressed()
{
  std::cout << "Connect button pressed!!!!!!" << std::endl;
  switch(wii -> get_wii_state()){
    case WII_DISCONNECTED:
      emit connect();
      std::cout << "wiimote start" << std::endl;
      break;
    case WII_CONNECTED:
      emit disconnect();
      std::cout << "wiimote stop" << std::endl;
      break;
    default:
      break;
  }
}

void WiiServerWindow::update_state(server_state_t server_state)
{
  switch(server_state){
    case WII_DISCONNECTED:
      std::cout << "Changing state to disconnected" << std::endl;
      ui.WiiServerStatus->setText("Disconnected!");
      ui.ConnectButton->setText("Connect");
      ui.ConnectButton->setEnabled(true);
      break;
    case WII_CONNECTING:
      std::cout << "Changing state to connecting" << std::endl;
      ui.WiiServerStatus->setText("Connecting!");
      ui.ConnectButton->setEnabled(false);
      break;
    case WII_CONNECTED:
      std::cout << "Changing state to connected" << std::endl;
      ui.WiiServerStatus->setText("Connected!");
      ui.ConnectButton->setText("Disconnect");
      ui.ConnectButton->setEnabled(true);
      break;
  }
}

void WiiServerWindow::handle_command()
{
  int indication = ltr_int_getWiiIndication(mm);
  static command_t old_cmd = STOP;
  command_t cmd;
  cmd = ltr_int_getCommand(mm);
  if(cmd != old_cmd){
    switch(cmd){
      case STOP:
        std::cout<<"Received stop command"<<std::endl;
        emit stop();
        break;
      case SLEEP:
        std::cout<<"Received sleep command"<<std::endl;
        emit pause(indication & 15);
        break;
      case WAKEUP:
        std::cout<<"Received wakeup command"<<std::endl;
        emit wakeup((indication >> 4) & 15);
        break;
    }
  }
  old_cmd = cmd;
}


