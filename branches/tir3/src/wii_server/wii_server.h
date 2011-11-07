#ifndef WII_SERVER__H
#define WII_SERVER__H

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "wiimote.h"
#include "ui_wii_server.h"

class QTimer;

class WiiServerWindow : public QWidget
{
  Q_OBJECT
 public:
  WiiServerWindow(QWidget *parent = 0);
  ~WiiServerWindow();
 signals:
  void connect();
  void disconnect();
  void pause(int);
  void wakeup(int);
  void stop();
 private slots:
  void on_ConnectButton_pressed();
  void handle_command();
 private:
  Ui::WiiServer ui;
  Wiimote *wii;
  WiiThread *thread;
  QTimer *cmdTimer;
  struct mmap_s *mm;
 private slots:
  void update_state(server_state_t server_state);
};


#endif
