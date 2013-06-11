#include "tir_model.h"

void device_model::enqueue_packet(const unsigned char data[], const size_t length)
{
  packet_t tmp;
  data2packet(data, length, tmp);
  packet_buffer.push(tmp);
}

void device_model::enqueue_packet(const packet_t &packet)
{
  packet_buffer.push(packet);
}


void device_model::data2packet(const unsigned char data[], const size_t length, 
                                  packet_t &packet)
{
  packet.resize(length);
  for(size_t i = 0; i < length; ++i){
    packet[i] = data[i];
  }
}


size_t device_model::packet2data(const packet_t packet, 
                                  unsigned char data[], size_t length)
{
  size_t min = (packet.size() < length) ? packet.size() : length;
  for(size_t i = 0; i < min; ++i){
    data[i] = packet[i];
  }
  return min;
}



tir4::tir4(std::string fname) : device_model(fname), 
  firmware_loaded(false), firmware_csum(0), video_on_flag(false), camera_on_flag(false), 
  led_r(false), led_g(false), led_b(false), led_ir(false), 
  firmware_active(false)
{
}

void tir4::set_leds(unsigned char leds, unsigned char mask)
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

bool tir4::send_packet(unsigned char packet[], size_t length)
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

void tir4::load_firmware(const unsigned char data[], const size_t length)
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

bool tir4::receive_packet(unsigned char packet[], size_t length, size_t *read, int timeout)
{
  (void) timeout;
  *read = 0;
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
    packet_buffer.pop();
  }
  return true;
}

void tir4::get_config()
{
  unsigned char config[] = {0x14, 0x40, 0x03, 0x01, 0x01, 0x83, 0xF5, 0x05, 
                            0x00, 0x00, 0x78, 0x01, 0x63, 0x01, 0x20, 0xB4,
                            0x00, 0x00, 0x00, 0x00};
  enqueue_packet(config, sizeof(config));
}

void tir4::get_status()
{
  unsigned char status[] = {0x07, 0x20, 0x01, 0x00, 0x00, 0x00, 0x01};
  status[3] = firmware_loaded ? 1 : 0;
  status[4] = firmware_csum >> 8;
  status[5] = firmware_csum & 0xFF;
  status[6] = firmware_active ? 2 : 1;
  enqueue_packet(status, sizeof(status));
}

void tir4::do_whatever()
{
  unsigned char whatever[] = {0x06, 0x10, 0x03, 0x11, 0x21, 0x00};
  enqueue_packet(whatever, sizeof(whatever));
}


