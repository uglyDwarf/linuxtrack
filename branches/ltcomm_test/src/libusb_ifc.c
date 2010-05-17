#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#include <stdio.h>
#include "usb_ifc.h"
#include "utils.h"

static libusb_context *usb_context = NULL;
static libusb_device_handle *handle = NULL;
static unsigned int in, out;

bool init_usb()
{
  int res = libusb_init(&usb_context);
  if(res != 0){
    log_message("Problem initializing libusb!\n");
    return false;
  }
  
  libusb_set_debug(usb_context, 0);
  return true;
}

static bool is_tir(libusb_device *dev, unsigned int devid)
{
  struct libusb_device_descriptor desc;
  if(libusb_get_device_descriptor(dev, &desc)){
    log_message("Error getting device descriptor!\n");
    return false;
  }
  if((desc.idVendor == 0x131D) && (desc.idProduct == devid)){
    return true;
  }else{
    return false;
  }
}

dev_found find_tir(unsigned int devid)
{
  // discover devices
  libusb_device **list;
  libusb_device *found = NULL;
  dev_found dev = NONE;
  size_t cnt = libusb_get_device_list(NULL, &list);
  size_t i = 0;
  int err = 0;
  if (cnt < 0){
    log_message("Error enumerating devices!\n");
    return NONE;
  }

  for(i = 0; i < cnt; i++){
    libusb_device *device = list[i];
    if(is_tir(device, 0x0157)){
      found = device;
      dev = TIR5;
      break;
    }
  }
  
  if(!found){
    for(i = 0; i < cnt; i++){
      libusb_device *device = list[i];
      if(is_tir(device, 0x0156)){
	found = device;
	dev = TIR4;
	break;
      }
    }
  }

  if(found){
    err = libusb_open(found, &handle);
    if(err){
      log_message("Error opening device!\n");
      return NONE;
    }
  }else{
    log_message("Can't find TrackIR!\n");
  }
  libusb_free_device_list(list, 1);
  if(handle != NULL){
    return dev;
  }else{
    log_message("Bad handle!\n");
    return NONE;
  }
}

static bool configure_tir(unsigned int config)
{
  int cfg = -1;
  if(libusb_get_configuration(handle, &cfg)){
    log_message("Can't get device configuration!\n");
    return false;
  }
  if(cfg != config){
    if(libusb_set_configuration(handle, config)){
      log_message("Can't set device configuration!\n");
      return false;
    }
  }
  return true;
}

static bool claim_tir(unsigned int config, unsigned int interface)
{
  if(libusb_claim_interface(handle, interface)){
    log_message("Couldn't claim interface!\n");
    return false;
  }
//  if(libusb_reset_device(handle)){
//    log_message("Couldn't reset device!\n");
//    return false;
//  }
  int cfg;
  if(libusb_get_configuration(handle, &cfg)){
    log_message("Can't get device configuration!\n");
    return false;
  }
  if(cfg != config){
    log_message("Can't set device configuration!\n");
    return false;
  }
  return true;
}

bool prepare_device(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep)
{
  in = in_ep;
  out = out_ep;
  return configure_tir(config) && claim_tir(config, interface);
}

bool send_data(unsigned char data[], size_t size)
{
  int res, transferred;
  if((res = libusb_bulk_transfer(handle, out, data, size, &transferred, 500))){
    log_message("Problem writing data to TIR! %d - %d transferred\n", res, transferred);
    return false;
  }
  return true;
}

bool receive_data(unsigned char data[], size_t size, size_t *transferred,
                  unsigned int timeout)
{
  if(timeout == 0){
    timeout = 500;
  }
  *transferred = 0;
  int res;
  if((res = libusb_bulk_transfer(handle, 0x82, data, size, (int*)transferred, timeout))){
    if(res != LIBUSB_ERROR_TIMEOUT){
      log_message("Problem reading data from TIR! %d - %d transferred\n", res, *transferred);
      return false;
    }
  }
  return true;
}

void finish_usb(unsigned int interface)
{
  libusb_release_interface(handle, interface);
  libusb_close(handle);
  libusb_exit(usb_context);
}
