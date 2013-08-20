#ifndef WIIMOTE__H
#define WIIMOTE__H

#include <QObject>
#include <QThread>
#include <cwiid.h>

class WiiThread : public QThread
{
  Q_OBJECT
 public:
  void run();
  void stop_it();
  void pass_ir_data(struct cwiid_ir_src *data);
 signals:
  void change_state(int state);
 public slots:
  void pause(int indication);
  void wakeup(int indication);
  void stop();
 private:
  void idle();
  bool exit_flag;
  cwiid_wiimote_t *gWiimote;
  bdaddr_t bdaddr;
  int idle_cntr;
  uint8_t idle_sig;
  bool isIdle;
};


typedef enum {WII_DISCONNECTED, WII_CONNECTING, WII_CONNECTED} server_state_t;

class Wiimote : public QObject
{
  Q_OBJECT
 public:
  Wiimote(struct mmap_s *m);
  ~Wiimote();
  server_state_t get_wii_state() const {return server_state;};
 signals:
  void changing_state(server_state_t state);
  void pause(int indication);
  void wakeup(int indication);
  void stop();
 public slots:
  void connect(void);
  void disconnect(void);
  void changed_state(int state);
 private:
  server_state_t server_state;
  WiiThread *thread;
};

#endif


