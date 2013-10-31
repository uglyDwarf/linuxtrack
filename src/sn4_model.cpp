#include <tir_model.h>

//*******************************************************************

smartnav4::smartnav4(std::string fname) :tir4(fname)
{
}

void smartnav4::get_config()
{
  unsigned char config[] = {0x15, 0x40, 0x02, 0x01, 0x02, 0xFF, 0xFF, 0x08, 
                            0x04, 0x00, 0x64, 0x02, 0x80, 0x01, 0xE0, 0x8C,
                            0x00, 0x00, 0x00, 0x00, 0x02};
  enqueue_packet(config, sizeof(config));
}

void smartnav4::do_whatever()
{
  unsigned char whatever[] = {0x06, 0x20, 0x02, 0x20, 0x00, 0x00};
  enqueue_packet(whatever, sizeof(whatever));
}



