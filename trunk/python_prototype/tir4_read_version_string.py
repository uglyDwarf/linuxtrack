#! /usr/bin/python
# -*- coding: utf-8 -*-
##bc##################################################################
## (C) Copyright 2009, All Rights Reserved.
##
## Name         : tir4_read_version_string.py
## Author       : DT Austin
## Created      : 07/14/2009
## SVN date     : $Date$
##
######################################################################
## Description: sends a request version message and 
##  prints the result to stdout
##ec##################################################################
import sys
from optparse import OptionParser
import time

try:
    import tir4
except:
    print("tir4.py missing")
    sys.exit(1)


class GeneralException(Exception):
    def __init__(self, msg):
        self.msg = msg

def handle_exception(errorinfo):
    if errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.USB_LIST_FAILED:
        self.msgbox_and_die("Unable to obtain a list the USB devices.\nApplication will close.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.FIND_DEVICE_FAILED:
        self.msgbox_and_die("Unable to find TIR4 device.\nApplication will close.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.CREATE_HANDLE_FAILED:
        self.msgbox_and_die("Unable to create a handle for the TIR4 device.\nApplication will close.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.CLAIM_FAILED:
        self.msgbox_and_die("Unable to claim the TIR4 device.\nThis is most likely a USB permissions problem.\nRunning this app sudo may be a quick fix.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.DISCONNECT:
        self.msgbox_and_die("TIR4 device disconnected.\nApplication will close.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.UNKNOWN_READ_ERROR:
        self.msgbox_and_die("Unknown usb read error.\nApplication will close.")
    elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.UNKNOWN_PACKET:
        self.msgbox("Unknown usb read error.\nApplication will close.")

def main(argv=None):
    usage = "usage: %prog [options] [filename]"
    parser = OptionParser(usage=usage)
    parser.add_option('--verbose', '-v',
                       help   ='print debugging output',
                       action = 'store_true')

    try:
        (options, args) = parser.parse_args(argv)
    except:
        raise GeneralOptionParser("Usage:")

    try:
      t4d = tir4.TIR4Control()
      if not t4d.is_device_present():
        print "Unable to find TIR4 device."
        sys.exit(1)
        
      done = False
      while not(done):
        t4d.do_init_step_start()
        t4d.nq_write_usb((0x12,))
        time.sleep(0.1)
        t4d.nq_write_usb((0x14,0x01))
        time.sleep(0.1)
        t4d.nq_write_usb((0x12,))
        time.sleep(0.1)
        t4d.nq_write_usb((0x13,))
        time.sleep(0.1)
        t4d.nq_write_usb((0x17,))
        time.sleep(0.1)
        t4d.do_read_usb()
        bytes = t4d.readbyteq.peek_bytes()
        if len(bytes) > 0:
          done = True
        else:
          time.sleep(1)
      for byte in bytes:
        print "0x%02x" % byte

    except tir4.TIR4Exception, errorinfo:
      handle_exception(errorinfo)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
