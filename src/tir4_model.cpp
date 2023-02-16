#include "tir_model.h"
#include <iostream>
#include <iomanip>

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
  std::cout<<"0x";
  for(size_t i = 0; i < length; ++i){
    std::cout<<std::uppercase<<std::setw(2)<<std::setfill('0')<<(int)data[i];
  }
  std::cout<<"\n";
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


//======================================================================================
tir5v3::tir5v3(std::string fname) : device_model(fname), state(0), camera_on(false)
{
  std::cout<<"Initializing TrackIR5 ver. 3\n";
}

/*
  $r1 = $bytes[17];
  $res = $bytes[0] ^ $bytes[$r1 & 0x0F] ^ $bytes[$r1 >> 4] ^ 0x69;
  $r2 = ($bytes[1] & 8) | ($bytes[2] & 4) | ($bytes[3] & 2) | ($bytes[4] & 1);
  if($res != 0x1A){
    #printf("Decode: %02X %02X\n", $r1, $r2);
    $res <<= 8;
    $res += $bytes[$r2] ^ $bytes[16];
    $res <<= 8;
    $res+= $bytes[$r2-1] ^ $bytes[19];
    $res <<= 8;
    $res += $bytes[$r2+1] ^ $bytes[18];
    return sprintf("0x%08X", $res);
  }else{
    $res <<= 8;
    $res += ($bytes[$r2] ^ $bytes[16]) >> 4;
    return sprintf("0x%04X", $res);
  }
 */

int tir5v3::deobfuscate_command(unsigned char packet[], unsigned char command[])
{
  unsigned char r1 = packet[17];
  command[0] = packet[0] ^ packet[r1 & 0x0F] ^ packet[r1 >> 4] ^ 0x69;
  unsigned char r2 = (packet[1] & 8) | (packet[2] & 4) | (packet[3] & 2) | (packet[4] & 1);
  if(command[0] == 0x1A){  //2 bytes command
    command[1] = ((packet[r2] ^ packet[16]) >> 4) & 7;
    return 2;
  }else if((command[0] == 0x19) || (command[0] == 0x23)){ //4 bytes command
    command[1] = packet[r2] ^ packet[16];
    command[2] = packet[r2 - 1] ^ packet[19];
    command[3] = packet[r2 + 1] ^ packet[18];
    return 4;
  }else{ //1 byte command, decoded already
    return 1;
  }
}

bool tir5v3::report_state()
{
  unsigned char config[] = {0x11, 0x20, 0x01, (unsigned char)((state > 2) ? 0x01 : 0x00),
                            0x01, 0x78, 0xF7, 0x0B, 0xA2, 0x8F, 0xB9, 0x23, 0x3F, 0x9E,
                            0x12, 0xCC, 0xD9};
  enqueue_packet(config, sizeof(config));
  return true;
}

bool tir5v3::get_config()
{
  unsigned char config[] = {0x14, 0x40, 0x03, 0x01, 0x04, 0x16, 0xE0, 0x23, 0x00,
      0x00, 0x78, 0x02, 0x80, 0x01, 0xE0, 0x96, 0x00, 0x18, 0xDF, 0x20};
  enqueue_packet(config, sizeof(config));
  return true;
}

bool tir5v3::change_state(unsigned char new_state)
{
  if(state > 6){
    std::cout << "Received request to go to nonexistent state " << new_state << ".\n";
    return false;
  }
  printf("Changing state: %d -> %d.\n", state, new_state);
  state = new_state;
  if(state == 4){
    camera_on = true;
  }
  return true;
}

const char *led_state(int v)
{
  return v ? "On" : "Off";
}

bool tir5v3::set_register(unsigned char v0, unsigned char v1,
                          unsigned char v2, unsigned char v3)
{
  //printf("Setting register ", v1, v2, v3);
  if(v0 == 0x19){
    switch(v1){
      case 0x05:
        printf("Setting light filter: %g\n", ((v2 << 8) + v3) / 2.0);
        break;
      case 0x04:
        printf("Setting LEDs: Intensity %d, L Green %s, L Red %s, R Green %s, R Red %s\n", v2,
              led_state(v3 & 2),    led_state(v3 & 1),
              led_state(v3 & 0x20), led_state(v3 & 0x20));
        break;
      case 0x09:
        if(v2 == 0){
          printf("IR LEDs %s\n", led_state(v3 & 1));
        }else{
          printf("UNKNOWN REGISTER SET: 0x19 0x%02X 0x%02X 0x%02X\n", v1, v2, v3);
        }
        break;
      case 0x35:
      case 0x3B:
        printf("Setting IR LED intensity.\n");
        //TODO: more checks
        break;
      default:
        printf("UNKNOWN REGISTER SET: 0x19 0x%02X 0x%02X 0x%02X\n", v1, v2, v3);
        break;
    }
  }else if(v0 == 0x23){
    switch(v1){
      case 0x35:
      case 0x3B:
        printf("Setting IR LED intensity.\n");
        //TODO: more checks
        break;
      default:
        printf("UNKNOWN REGISTER SET: 0x%02X 0x%02X 0x%02X 0x%02X\n", v0, v1, v2, v3);
        break;
    }
  }
  return true;
}

bool tir5v3::camera_off()
{
  printf("Turning camera Off.\n");
  camera_on = false;
  return true;
}

bool tir5v3::send_packet(int ep, unsigned char packet[], size_t length)
{
  if(length == 0){
    return true;
  }
  unsigned char command[4];
  size_t i;
  printf("Packet: ");
  for(i = 0; i < length; ++i){
    printf("%02X ", packet[i]);
  }
  printf("\n");
  int cmd_len = deobfuscate_command(packet, command);
  print_packet(ep, command, cmd_len);
  switch(command[0]){
    case 0x1A:
      if(command[1] == 7){
        report_state();
      }else{
        change_state(command[1]);
      }
      break;
    case 0x12:
      break;
    case 0x13:
      camera_off();
      break;
    case 0x19: //set register
    case 0x23:
      set_register(command[0], command[1], command[2], command[3]);
      break;
    case 0x17: //get config request
      get_config();
      std::cout<<"Got config request!\n";
      break;
    default:
      std::cout<<"Unknown packet!\n";
      break;
  }
  return true;
}

bool tir5v3::receive_packet(int ep, unsigned char packet[], size_t length, size_t *read, int timeout)
{
  (void) timeout;
  if(ep != 0x82){
    return false;
  }
  *read = 0;
  //std::cout<<"Fakeussb: Request to read from ep: "<<std::hex<<ep<<"\n";
  if(pkt_buf_size() <= 0){
    if(state != 4){
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
