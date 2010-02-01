#include <openusb.h>
#include <stdbool.h>
#include <stdio.h>
#include "usb_ifc.h"
#include "utils.h"

static openusb_handle_t handle;
static openusb_dev_handle_t devhandle;
static unsigned int in, out;


bool init_usb()
{
  if(openusb_init(0, &handle) != OPENUSB_SUCCESS){
    log_message("Problem initializing library!\n");
    return false;
  }
  openusb_set_debug(handle, 0, 0, NULL);
  openusb_set_default_timeout(handle, USB_TYPE_ALL, 500);
  return true;
}

dev_found find_tir()
{
  openusb_devid_t *devlist = NULL;
  uint32_t devs = 0;
  dev_found dev = NONE;
  if(openusb_get_devids_by_vendor(handle, 0x131D, 0x0157, &devlist, &devs)
    == OPENUSB_SUCCESS){
    dev = TIR5;
  }
  
  if(dev == NONE){
    if(openusb_get_devids_by_vendor(handle, 0x131D, 0x0156, &devlist, &devs)
      == OPENUSB_SUCCESS){
      dev = TIR4;
    }else{
      log_message("Couldn't find Track IR!\n");
      return NONE;
    }
  }
  
  if(openusb_open_device(handle, devlist[0], USB_INIT_DEFAULT, &devhandle) 
    != OPENUSB_SUCCESS){
    log_message("Open device handle failed!\n");
    return NONE;
  }
  if(openusb_reset(devhandle) != OPENUSB_SUCCESS){
    log_message("Couldn't reset dev!\n");
    return NONE;
  }
  openusb_free_devid_list(devlist);
  return dev;
}

static bool configure_tir(unsigned int config)
{
  if(openusb_set_configuration(devhandle, config) != OPENUSB_SUCCESS){
    log_message("Couldn't set configuration!\n");
    return false;
  }
  return true;
}

static bool claim_tir(unsigned int interface)
{
  if(openusb_claim_interface(devhandle, interface, USB_INIT_DEFAULT)){
    log_message("Couldn't set interface!\n");
    return false;
  }
  return true;
}

bool prepare_device(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep)
{
  in = in_ep;
  out = out_ep;
  return configure_tir(config) && claim_tir(interface);
}

bool send_data(unsigned char data[], size_t size)
{
  int32_t res;
  struct openusb_bulk_request br = {
    .payload = data,
    .length = size,
    .timeout = 500,
    .flags = 0,
    .next = NULL
  };
  
  if((res = openusb_bulk_xfer(devhandle, 0, out, &br)) != OPENUSB_SUCCESS){
    log_message("Can't send message!\n");
    return false;
  }
  return true;
}

bool receive_data(unsigned char data[], size_t size, size_t *transferred, 
                  unsigned int timeout)
{
  int32_t res;
  if(timeout == 0){
    timeout = 500;
  }
  struct openusb_bulk_request br = {
    .payload = data,
    .length = size,
    .timeout = timeout,
    .flags = 0,
    .next = NULL
  };
  if((res = openusb_bulk_xfer(devhandle, 0, in, &br)) != OPENUSB_SUCCESS){
    if(res != OPENUSB_IO_TIMEOUT){
      log_message("Can't receive message! (%d)\n", res);
      return false;
    }
  }
  *transferred = br.result.transferred_bytes;
  return true;
}

void finish_usb(unsigned int interface)
{
  int32_t res;
  if((res = openusb_release_interface(devhandle, 0)) != OPENUSB_SUCCESS){
    log_message("Couldn't release interface! (%d)\n", res);
  }
  openusb_close_device(devhandle);
  openusb_fini(handle);
}
