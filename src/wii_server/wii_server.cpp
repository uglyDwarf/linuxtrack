
#include "wii_server.h"
#include <image_process.h>
#include <utils.h>
#include <ipc_utils.h>
#include <string.h>

#include <iostream>
#include <QTimer>
#include <QMessageBox>

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768


WiiServerWindow::WiiServerWindow(QWidget *parent) : QWidget(parent), wii(NULL), mm(NULL), old_cmd(STOP)
{
  ui.setupUi(this);
  setWindowTitle(QString("Linuxtrack Wii server v")+PACKAGE_VERSION);
  if(ltr_int_initWiiCom(true, &mm)){
    std::cout<<"Wii Server initialized!!!\n";
  }else{
    QMessageBox::critical(this, "Wii server initialization failed", 
      "Can't initialize Wii server communication channel;\n Please run Linuxtrack GUI first!");
    exit(1);
  }
  wii = new Wiimote(mm);
  cmdTimer = new QTimer(this);
  if(!QObject::connect(this, SIGNAL(connect()), wii, SLOT(connect()))) std::cout << "Sig1 fail!\n";
  if(!QObject::connect(this, SIGNAL(disconnect()), wii, SLOT(disconnect()))) std::cout << "Sig2 fail!\n";
  if(!QObject::connect(wii, SIGNAL(changing_state(server_state_t)), this, SLOT(update_state(server_state_t)))) 
    std::cout << "Sig3 fail!\n";
  if(!QObject::connect(cmdTimer, SIGNAL(timeout()), this, SLOT(handle_command()))) std::cout << "Sig4 fail!\n";
  
  if(!QObject::connect(this, SIGNAL(pause(int)), wii, SIGNAL(pause(int)))) std::cout << "Sig5 fail!\n";
  if(!QObject::connect(this, SIGNAL(wakeup(int)), wii, SIGNAL(wakeup(int)))) std::cout << "Sig6 fail!\n";
  if(!QObject::connect(this, SIGNAL(stop()), wii, SIGNAL(stop()))) std::cout << "Sig7 fail!\n";
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
  std::cout << "Connect button pressed!!!!!!\n";
  switch(wii -> get_wii_state()){
    case WII_DISCONNECTED:
      emit connect();
      std::cout << "wiimote start\n";
      break;
    case WII_CONNECTED:
      emit disconnect();
      std::cout << "wiimote stop\n";
      break;
    default:
      break;
  }
}

void WiiServerWindow::update_state(server_state_t server_state)
{
  switch(server_state){
    case WII_DISCONNECTED:
      std::cout << "Changing state to disconnected\n";
      ui.WiiServerStatus->setText("Disconnected!");
      ui.ConnectButton->setText("Connect");
      ui.ConnectButton->setEnabled(true);
      cmdTimer->stop();
      old_cmd = STOP;
      break;
    case WII_CONNECTING:
      std::cout << "Changing state to connecting\n";
      ui.WiiServerStatus->setText("Connecting!");
      ui.ConnectButton->setEnabled(false);
      break;
    case WII_CONNECTED:
      std::cout << "Changing state to connected\n";
      ui.WiiServerStatus->setText("Connected!");
      ui.ConnectButton->setText("Disconnect");
      ui.ConnectButton->setEnabled(true);
      cmdTimer->start(100);
      break;
  }
}

void WiiServerWindow::handle_command()
{
  int indication = ltr_int_getWiiIndication(mm);
  command_t cmd;
  cmd = ltr_int_getCommand(mm);
  if(cmd != old_cmd){
    switch(cmd){
      case STOP:
        std::cout<<"Received stop command\n";
        emit stop();
        break;
      case SLEEP:
        std::cout<<"Received sleep command\n";
        emit pause(indication & 15);
        break;
      case WAKEUP:
        std::cout<<"Received wakeup command\n";
        emit wakeup((indication >> 4) & 15);
        break;
    }
  }
  old_cmd = cmd;
}

#include "moc_wii_server.cpp"

