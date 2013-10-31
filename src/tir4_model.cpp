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

void device_model::print_packet(int ep, unsigned char data[], size_t length)
{
  std::cout<<"Fakeusb: (EP: "<<ep<<") "<<std::hex;
  for(size_t i = 0; i < length; ++i){
    std::cout<<(int)data[i]<<",";
  }
  std::cout<<std::endl;
}



tir4::tir4(std::string fname) : smartnav3(fname), 
  firmware_loaded(false), firmware_csum(0), firmware_active(false)
{
}

bool tir4::send_packet(int ep, unsigned char packet[], size_t length)
{
  bool report = true;
  if(length == 0){
    return true;
  }
  switch(packet[0]){
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
      smartnav3::send_packet(ep, packet, length);
      report = false;
      break;
  }
  if((packet[0] != 0x1C) && report){//don't print firmware packets
    print_packet(ep, packet, length);
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


