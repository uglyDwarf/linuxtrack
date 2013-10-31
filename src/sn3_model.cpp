#include "tir_model.h"

smartnav3::smartnav3(std::string fname) : device_model(fname), 
  video_on_flag(false), camera_on_flag(false), 
  led_r(false), led_g(false), led_b(false), led_ir(false), threshold(0)
{
  std::cout<<"Initializing SmartNav3 model."<<std::endl;
}

void smartnav3::set_leds(unsigned char leds, unsigned char mask)
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

bool smartnav3::send_packet(int ep, unsigned char packet[], size_t length)
{
  if(length == 0){
    return true;
  }
  switch(packet[0]){
    case 0x10:
      set_leds(packet[1], packet[2]);
      break;
    case 0x12:
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
    case 0x15: //set threshold
      if((packet[2] == 1) && (packet[3] == 0)){
        set_threshold(packet[1]);
      }else{
        std::cout<<"Bad packet!"<<std::endl;
      }
      break;
    case 0x17: //get config request
      get_config();
      std::cout<<"Got config request!"<<std::endl;
      break;
    default:
      std::cout<<"Unknown packet!"<<std::endl;
      break;
  }
  print_packet(ep, packet, length);
  return true;
}

void smartnav3::set_threshold(int thr)
{
  if((thr >= 0x28) && (thr <= 0xFD)){
    threshold = thr;
    std::cout<<"Threshold set to "<<thr<<"."<<std::endl;
  }else{
    std::cout<<"Threshold out of bounds! ("<<thr<<")"<<std::endl;
  }
}

bool smartnav3::receive_packet(int ep, unsigned char packet[], size_t length, size_t *read, int timeout)
{
  (void) timeout;
  (void) ep; // TODO: check reading from right endpoint!
  *read = 0;
  //std::cout<<"Fakeussb: Request to read from ep: "<<std::hex<<ep<<std::endl;
  if(pkt_buf_size() <= 0){
    if((!video_on_flag) || (!camera_on_flag)){
      return false;
    }
    std::vector<unsigned char> data;
    if(inp_data.read_next(data)){
      *read = packet2data(data, packet, length);
    }
  }else{
    *read = packet2data(packet_buffer.front(), packet, length);
//    printf("Packet: ");
//    for(size_t i = 0; i < *read; ++i){
//      printf("%02X ", packet[i]);
//      if((i > 0) && (i % 16 == 0)){
//        printf("\n");
//      }else if((i > 0) && (i % 4 == 0)){
//        printf(" ");
//      }
//    }
//    printf("\n");
    packet_buffer.pop();
  }
  return true;
}

void smartnav3::get_config()
{
  unsigned char config[] = {0x09, 0x40, 0x02, 0x00, 0x00, 0x9B, 0x40, 0x04, 0x00};
  enqueue_packet(config, sizeof(config));
}

/*
void smartnav3::do_whatever()
{
  unsigned char whatever[] = {0x06, 0x10, 0x03, 0x11, 0x21, 0x00};
  enqueue_packet(whatever, sizeof(whatever));
}
*/
