#ifndef TIR_MODEL__H
#define TIR_MODEL__H

#include "usb_ifc.h"


#ifdef __cplusplus
extern "C" {
#endif
  
  bool init_model(char fname[], int type);
  void fakeusb_send(unsigned char data[], size_t length);
  void fakeusb_receive(unsigned char data[], size_t length, size_t *read, int timeout);
  void close_model();
  void print_packet(unsigned char data[], size_t length);
  
#ifdef __cplusplus
}

#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>

typedef std::vector<unsigned char> packet_t;

//Reads-in input packets from the specified file
//
class input_data
{
 public:
  input_data(std::string fname) :
    f(fname.c_str(), std::fstream::in){};
  bool read_next(packet_t &data);
 private:
  std::ifstream f;
  std::istringstream is;
  std::string str;
};



class device_model
{
 public:
  device_model(std::string fname) : inp_data(fname){};
  virtual ~device_model(){};
  virtual bool send_packet(unsigned char packet[], size_t length) = 0;
  virtual bool receive_packet(unsigned char packet[], size_t length, 
                              size_t *read, int timeout) = 0;
  size_t pkt_buf_size(){return packet_buffer.size();};
 protected:
  size_t packet2data(const packet_t packet, unsigned char data[], size_t length);
  void data2packet(const unsigned char data[], const size_t length, packet_t &packet);
  void enqueue_packet(const unsigned char data[], const size_t length);
  void enqueue_packet(const packet_t &packet);
  input_data inp_data;
  std::queue<packet_t> packet_buffer;
};

class tir4 : public device_model
{
 public:
  tir4(std::string fname);
  virtual ~tir4(){};
  virtual bool send_packet(unsigned char packet[], size_t length);
  virtual bool receive_packet(unsigned char packet[], size_t length, 
                              size_t *read, int timeout);
 protected:
  virtual void video_on(){video_on_flag = true; camera_on_flag = true;};
  virtual void video_off(){video_on_flag = false;};
  virtual void camera_off(){camera_on_flag = false;};
  virtual void set_leds(unsigned char leds, unsigned char mask);
  virtual void get_config();
  virtual void get_status();
  virtual void do_whatever();
  virtual void load_firmware(const unsigned char data[], const size_t length);
  
  bool firmware_loaded;
  unsigned int firmware_csum;
  bool video_on_flag;
  bool camera_on_flag;
  bool led_r, led_g, led_b, led_ir;
  bool firmware_active;
};

class smartnav4 : public tir4
{
 public:
  smartnav4(std::string fname);
  virtual ~smartnav4(){};
 private:
  virtual void get_config();
  virtual void do_whatever();
};

class tir5: public tir4
{
 public:
  tir5(std::string fname);
  virtual ~tir5(){};
 private:
  virtual void get_config();
  virtual void do_whatever();
};


#endif

#endif
