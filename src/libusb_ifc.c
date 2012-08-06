#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#include <stdio.h>
#define USB_IMPL_ONLY
#include "usb_ifc.h"
#include "utils.h"

static libusb_context *usb_context = NULL;
static libusb_device_handle *handle = NULL;
static unsigned int in, out;
static bool interface_claimed = false;

bool ltr_int_init_usb()
{
  ltr_int_log_message("Initializing libusb.\n");
  int res = libusb_init(&usb_context);
  if(res != 0){
    ltr_int_log_message("Problem initializing libusb!\n");
    return false;
  }
  ltr_int_log_message("Libusb initialized successfuly.\n");
  
  libusb_set_debug(usb_context, 0);
  ltr_int_log_message("Libusb debug level set.\n");
  return true;
}

static bool is_tir(libusb_device *dev, unsigned int devid)
{
  struct libusb_device_descriptor desc;

  ltr_int_log_message("Checking, if device is Track IR.\n");
  if(libusb_get_device_descriptor(dev, &desc)){
    ltr_int_log_message("Error getting device descriptor!\n");
    return false;
  }
  ltr_int_log_message("Device descriptor received.\n");
  if((desc.idVendor == 0x131D) && (desc.idProduct == devid)){
    ltr_int_log_message("Device is a TrackIR (%04X:%04X).\n", desc.idVendor, desc.idProduct);
    return true;
  }else{
    ltr_int_log_message("Device is not a TrackIR(%04X:%04X).\n", desc.idVendor, desc.idProduct);
    return false;
  }
}

dev_found ltr_int_find_tir(unsigned int devid)
{
  (void) devid;
  // discover devices
  libusb_device **list;
  libusb_device *found = NULL;
  dev_found dev = NONE;
  
  ltr_int_log_message("Requesting device list.\n");
  ssize_t cnt = libusb_get_device_list(usb_context, &list);
  ltr_int_log_message("Device list received (%d devices).\n", cnt);
  ssize_t i = 0;
  int err = 0;
  if (cnt < 0){
    ltr_int_log_message("Error enumerating devices!\n");
    return NONE;
  }

  for(i = 0; i < cnt; i++){
    libusb_device *device = list[i];
    if(is_tir(device, 0x0158)){
      found = device;
      dev = TIR5V2;
      break;
    }
  }
  
  if(!found){
    for(i = 0; i < cnt; i++){
      libusb_device *device = list[i];
      if(is_tir(device, 0x0157)){
	found = device;
	dev = TIR5;
	break;
      }
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

  if(!found){
    for(i = 0; i < cnt; i++){
      libusb_device *device = list[i];
      if(is_tir(device, 0x0155)){
        found = device;
        dev = TIR3;
        break;
      }
    }
  }

  if(!found){
    for(i = 0; i < cnt; i++){
      libusb_device *device = list[i];
      if(is_tir(device, 0x0150)){
        found = device;
        dev = TIR2;
        break;
      }
    }
  }

  if(found){
    ltr_int_log_message("Opening handle to the device found.\n");
    err = libusb_open(found, &handle);
    if(err){
      ltr_int_log_message("Error opening device!\n");
      return dev | NOT_PERMITTED;
    }
    ltr_int_log_message("Handle opened successfully.\n");
  }else{
    ltr_int_log_message("Can't find any TrackIR!\n");
  }
  ltr_int_log_message("Freeing device list.\n");
  libusb_free_device_list(list, 1);
  ltr_int_log_message("Device list freed.\n");
  if(handle != NULL){
    return dev;
  }else{
    ltr_int_log_message("Bad handle!\n");
    return NONE;
  }
}

static bool configure_tir(int config)
{
  int cfg = -1;
  ltr_int_log_message("Requesting TrackIR configuration.\n");
  if(libusb_get_configuration(handle, &cfg)){
    ltr_int_log_message("Can't get device configuration!\n");
    return false;
  }
  ltr_int_log_message("TrackIR configuration received.\n");
  if(cfg != config){
    ltr_int_log_message("Trying to set TrackIR configuration.\n");
    if(libusb_set_configuration(handle, config)){
      ltr_int_log_message("Can't set device configuration!\n");
      return false;
    }
    ltr_int_log_message("TrackIR configured.\n");
  }else{
    ltr_int_log_message("TrackIR already in requested configuration.\n");
  }
  return true;
}

static bool claim_tir(int config, unsigned int interface)
{
  ltr_int_log_message("Trying to claim TrackIR interface.\n");
  if(libusb_claim_interface(handle, interface)){
    ltr_int_log_message("Couldn't claim interface!\n");
    interface_claimed = false;
    return false;
  }
  ltr_int_log_message("TrackIR interface claimed.\n");
  interface_claimed = true;
//  if(libusb_reset_device(handle)){
//    log_message("Couldn't reset device!\n");
//    return false;
//  }
  int cfg;
  ltr_int_log_message("Requesting TrackIR configuration.\n");
  if(libusb_get_configuration(handle, &cfg)){
    ltr_int_log_message("Can't get device configuration!\n");
    return false;
  }
  ltr_int_log_message("TrackIR configuration received.\n");
  if(cfg != config){
    ltr_int_log_message("Device configuration is wrong!\n");
    return false;
  }
  ltr_int_log_message("Device configuration is OK.\n");
  return true;
}

bool ltr_int_prepare_device(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep)
{
  in = in_ep;
  out = out_ep;
  return configure_tir(config) && claim_tir(config, interface);
}

bool ltr_int_send_data(unsigned char data[], size_t size)
{
  int res, transferred;
  if((res = libusb_bulk_transfer(handle, out, data, size, &transferred, 500))){
    ltr_int_log_message("Problem writing data to TIR! %d - %d transferred\n", res, transferred);
    return false;
  }
  return true;
}

bool ltr_int_receive_data(unsigned char data[], size_t size, size_t *transferred,
                  unsigned int timeout)
{
  if(timeout == 0){
    timeout = 500;
  }
  *transferred = 0;
  int res;
  if((res = libusb_bulk_transfer(handle, in, data, size, (int*)transferred, timeout))){
    if(res != LIBUSB_ERROR_TIMEOUT){
      ltr_int_log_message("Problem reading data from TIR! %d - %d transferred\n", res, *transferred);
      return false;
    }else{
      ltr_int_log_message("Data receive request timed out!\n");
    }
  }
  return true;
}

void ltr_int_finish_usb(unsigned int interface)
{
  ltr_int_log_message("Closing TrackIR.\n");
  if(interface_claimed){
    ltr_int_log_message("Releasing TrackIR interface.\n");
    libusb_release_interface(handle, interface);
    ltr_int_log_message("TrackIR interface released.\n");
  }
  interface_claimed = false;
  ltr_int_log_message("Closing TrackIR handle.\n");
  libusb_close(handle);
  ltr_int_log_message("Exiting libusb.\n");
  libusb_exit(usb_context);
  ltr_int_log_message("Libusb exited.\n");
}

