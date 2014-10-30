
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>
#include "tir_model.h"
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

device_model *model = NULL;

bool init_model(char fname[], int type)
{
  if(model == NULL){
    switch(type){
      case SMARTNAV4:
        model = new smartnav4(fname);
        break;
      case SMARTNAV3:
        model = new smartnav3(fname);
        break;
      case TIR4:
      case TIR5:
        model = new tir4(fname);
        break;
      default:
        return false;
        break;
    }
  }
  return true;
}

void fakeusb_send(int ep, unsigned char data[], size_t length)
{
  model->send_packet(ep, data, length);
}

void fakeusb_receive(int ep, unsigned char data[], size_t length, size_t *read, int timeout)
{
  model->receive_packet(ep, data, length, read, timeout);
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


