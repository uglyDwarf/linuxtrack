
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>
#include "tir_model.h"
typedef std::vector<unsigned char> packet_t;
//*******************************************************************
void print_packet(unsigned char data[], size_t length)
{
  std::cout<<"Fakeusb: "<<std::hex;
  for(size_t i = 0; i < length; ++i){
    std::cout<<(int)data[i]<<",";
  }
  std::cout<<std::endl;
}

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

//read_next fails only on non-recoverable error!
bool input_data::read_next(packet_t &data)
{
  std::string dir;
  bool restarted = false;
  data.clear();
  if(!f.is_open()){
    return false;
  }
  //Read in the next input packet
  //  wrap around is handled, if wraps second time in the loop, 
  //  fail is reported (no input packet in the whole file)
  while(1){
    if(restarted){
      return false;
    }
    if(!f.good()){
      //we have a problem, start from the begining
      f.clear();
      f.seekg(0);
      restarted = true;
    }
    //real next line and check direction
    getline(f, str);
    is.str(str);
    is.clear();
    is>>dir;
    if(dir.compare("in") == 0){
      break;
    }
  }
  is>>std::hex;
  int num;
  while(1){
    is>>num;
    if(!is.good()){
      break;
    }
    //Data are accepted only upon successfull read-in
    data.push_back(num);
  }
  return true;
}
//*******************************************************************


/*
class device_model
{
 public:
  device_model(std::string fname);
  virtual bool send_packet(unsigned char packet[], size_t length) = 0;
  virtual bool receive_packet(unsigned char packet[], size_t length, int timeout);
 protected:
  void push_packet(packet_t packet);
  void push_packet(const unsigned char data[], const size_t length);
 private:
};
*/


class smartnav4
{
 public:
  smartnav4(std::string fname);
  virtual ~smartnav4(){};
  virtual bool send_packet(unsigned char packet[], size_t length);
  virtual bool receive_packet(unsigned char packet[], size_t length, 
                              size_t *read, int timeout);
 private:
  input_data inp_data;
  std::queue<packet_t> packet_buffer;

  size_t packet2data(const packet_t packet, unsigned char data[], size_t length);
  void data2packet(const unsigned char data[], const size_t length, packet_t &packet);
  void enqueue_packet(const unsigned char data[], const size_t length);
  void enqueue_packet(const packet_t &packet);
  
  void video_on(){video_on_flag = true; camera_on_flag = true;};
  void video_off(){video_on_flag = false;};
  void camera_off(){camera_on_flag = false;};
  void set_leds(unsigned char leds, unsigned char mask);
  void get_config();
  void get_status();
  void do_whatever();
  void load_firmware(const unsigned char data[], const size_t length);
  
  bool firmware_loaded;
  unsigned int firmware_csum;
  bool video_on_flag;
  bool camera_on_flag;
  bool led_r, led_g, led_b, led_ir;
  bool firmware_active;
};

smartnav4::smartnav4(std::string fname) : inp_data(fname), 
  firmware_loaded(false), firmware_csum(0), video_on_flag(false), camera_on_flag(false), 
  led_r(false), led_g(false), led_b(false), led_ir(false), 
  firmware_active(false)
{
}

void smartnav4::set_leds(unsigned char leds, unsigned char mask)
{
  if(mask & 0x80){
    led_ir = ((leds & 0x80) != 0);
  }
  if(mask & 0x40){
    led_b = ((leds & 0x40) != 0);
  }
  if(mask & 0x20){
    led_g = ((leds & 0x20) != 0);
  }
  if(mask & 0x10){
    led_r = ((leds & 0x10) != 0);
  }
}

bool smartnav4::send_packet(unsigned char packet[], size_t length)
{
  if(length == 0){
    return true;
  }
  if(packet[0] != 0x1C){
    print_packet(packet, length);
  }
  switch(packet[0]){
    case 0x10:
      set_leds(packet[1], packet[2]);
      break;
    case 0x13:
      camera_off();
      break;
    case 0x14:
      switch(packet[1]){
        case 0x00:
          video_on();
          break;
        case 0x01:
          video_off();
          break;
        default:
          std::cout<<"Bad packet!"<<std::endl;
          return false;
          break;
      }
      break;
    case 0x17: //get config request
      get_config();
      break;
    case 0x1B: //fpga_init
      firmware_loaded = false;
      firmware_active = false;
      firmware_csum = 0;
      break;
    case 0x1C: //firmware packet
      load_firmware(packet, length);
      break;
    case 0x1D: //get status request
      get_status();
      break;
    case 0x1F: //no idea
      do_whatever();
      break;
    case 0x20: //reload config
      firmware_active = true;
      break;
    default:
      //std::cout.fill('0');
      //std::cout.width(2);
      //for(int i = 0; i < length; ++i){
      //  std::cout<<std::showbase<<(int)packet[i]<<" ";
      //}
      //std::cout<<std::endl;
      break;
  }
  return true;
}

void smartnav4::load_firmware(const unsigned char data[], const size_t length)
{
  unsigned int byte = 0;
  size_t i = 2; //skip header
  
  while(i < length){
    byte = (unsigned int)data[i];

    firmware_csum += byte;
    byte = byte << 4;
    firmware_csum ^= byte;

    ++i; 
  }
  firmware_csum &= 0xffff;
  firmware_loaded = true;
}

void smartnav4::enqueue_packet(const unsigned char data[], const size_t length)
{
  packet_t tmp;
  data2packet(data, length, tmp);
  packet_buffer.push(tmp);
}

void smartnav4::enqueue_packet(const packet_t &packet)
{
  packet_buffer.push(packet);
}


void smartnav4::data2packet(const unsigned char data[], const size_t length, 
                                  packet_t &packet)
{
  packet.resize(length);
  for(size_t i = 0; i < length; ++i){
    packet[i] = data[i];
  }
}


size_t smartnav4::packet2data(const packet_t packet, 
                                  unsigned char data[], size_t length)
{
  size_t min = (packet.size() < length) ? packet.size() : length;
  for(size_t i = 0; i < min; ++i){
    data[i] = packet[i];
  }
  return min;
}


bool smartnav4::receive_packet(unsigned char packet[], size_t length, size_t *read, int timeout)
{
  (void) timeout;
  *read = 0;
  if(packet_buffer.size() <= 0){
    if((!video_on_flag) || (!camera_on_flag)){
      return false;
    }
    std::vector<unsigned char> data;
    if(inp_data.read_next(data)){
      *read = packet2data(data, packet, length);
    }
  }else{
    *read = packet2data(packet_buffer.front(), packet, length);
    packet_buffer.pop();
  }
  return true;
}

void smartnav4::get_config()
{
  unsigned char config[] = {0x15, 0x40, 0x02, 0x01, 0x02, 0xFF, 0xFF, 0x08, 
                            0x04, 0x00, 0x64, 0x02, 0x80, 0x01, 0xE0, 0x8C,
                            0x00, 0x00, 0x00, 0x00, 0x02};
  enqueue_packet(config, sizeof(config));
}

void smartnav4::get_status()
{
  unsigned char status[] = {0x07, 0x20, 0x01, 0x00, 0x00, 0x00, 0x01};
  status[3] = firmware_loaded ? 1 : 0;
  status[4] = firmware_csum >> 8;
  status[5] = firmware_csum & 0xFF;
  status[6] = firmware_active ? 2 : 1;
  enqueue_packet(status, sizeof(status));
}

void smartnav4::do_whatever()
{
  unsigned char whatever[] = {0x06, 0x20, 0x02, 0x20, 0x00, 0x00};
  enqueue_packet(whatever, sizeof(whatever));
}


smartnav4 *model = NULL;

void init_model(char fname[])
{
  if(model == NULL){
    model = new smartnav4(fname);
  }
}

void fakeusb_send(unsigned char data[], size_t length)
{
  model->send_packet(data, length);
}

void fakeusb_receive(unsigned char data[], size_t length, size_t *read, int timeout)
{
  model->receive_packet(data, length, read, timeout);
}

void close_model()
{
  if(model != NULL){
    delete model;
    model = NULL;
  }
}

/*

out 14 01 //video off
out 10 00 80
out 10 00 20
out 13 //camera off
out 17 //config request
  in 15 40 02 01 02 3A 1B 08 04 00 64 02 80 01 E0 8C 00 00 00 00 02
out 1D //status request
  in 07 20 01 00 00 00 01
out 1B //fpga init
out 1C 3E ... //upload firmware
...
out 1C 1D 01 00 00 5F 57 30 00 80 01 00 00 00 0D 20 00 00 00 20 00 00 00 20 00 00 00 20 00 00 00
out 19 05 10 10 00 //unk_7
out 1F 20 //unk_e
  in 06 20 02 20 00 00 //????
out 1D //get status
  in 07 20 01 01 6D 31 01
out 20 //cfg_reload
out 1D //get status
  in 07 20 01 01 6D 31 02
out 19 04 0F 00 0F //unk_9
out 19 14 10 00 01 //unk_a
out 23 90 0B 00 01 3C //unk_b
out 23 90 F0 32 01 3C //unk_c
out 19 14 10 00 00 //unk_d
out 15 78 01 00 //set threshold
out 14 00 //video on
  in 00 00 23 8A 00 00 00 08
  in 03 00 00 00 00 00 24 8D FC C9 DA 18 FD C6 E1 18 FE C5 E3 18 FF C5 E4 18 00 C7 E5 1C 01 C8 E5 1C 02 C8 E5 1C 03 C8 E5 1C 04 C8 E5 1C 05 C7 E5 1C 06 C6 E5 1C 07 C5 E5 1C 08 C5 E4 1C 09 C8 E3 1C 00 00 00 40
out 10 20 20
  in 03 00 00 00 00 00 25 8C FC C9 D7 18 FD C6 DF 18 FE C5 E2 18 FF C4 E3 18 00 C6 E4 1C 01 C7 E4 1C 02 C7 E5 1C 03 C7 E5 1C 04 C7 E5 1C 05 C6 E5 1C 06 C6 E4 1C 07 C5 E4 1C 08 C5 E4 1C 09 C6 E3 1C 0A D2 DA 1C 0A DD DE 1C 00 00 00 48
out 14 01 //video off
out 10 00 80
out 10 00 20
out 13 //camera stop
out 17 //config request
  in 15 40 02 01 02 3A 1B 08 04 00 64 02 80 01 E0 8C 00 00 00 00 02
out 1D //status request
  in 07 20 01 01 6D 31 02
out 14 00 //video on
  in 03 00 00 00
  in 00 00 36 9F 00 00 00 08
out 10 20 20
  in 03 00 00 00 00 00 37 9E 00 00 00 08
  in 03 00 00 00 00 00 38 91 00 00 00 08
out 10 80 80
*/


/*
out 14 01 //video off
out 12 //flush fifo
out 10 00 80
out 10 00 20
out 13 //stop camera

out 14 00 //video on
out 10 20 20

out 14 01 //video off
out 10 00 80
out 10 00 20
out 13 //stop camera

out 14 00 //video on
out 10 20 20
out 10 80 80
out 10 00 40
*/


