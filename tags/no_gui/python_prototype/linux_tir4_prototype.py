#! /usr/bin/python
# -*- coding: utf-8 -*-
##bc##################################################################
## (C) Copyright 2009, All Rights Reserved.
##
## Name         : linux_tir4_prototype.py
## Author       : DT Austin
## Created      : 07/01/2009
## SVN Rev      : $Revision$
## SVN Date     : $Date$
##
######################################################################
## Description: prototype app showing communication to the TIR4 camera
##ec##################################################################
import sys
from optparse import OptionParser
import gobject
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.glade
except:
    print("GTK Not Availible")
    sys.exit(1)
try:
    import tir4
except:
    print("tir4.py missing")
    sys.exit(1)

class GeneralException(Exception):
    def __init__(self, msg):
        self.msg = msg

class Enumeration(object):
    def __init__(self, names):
        for number, name in enumerate(names):
            setattr(self, name, number)

class appwindow(object):
    def __init__(self):
        self.tir4 = tir4.TIR4Control()
        self.state_enum = Enumeration(("NO_DEVICE",
                                       "OPEN_DEVICE",
                                       "DEVICE_LOADING",
                                       "DEVICE_READYING",
                                       "DEVICE_READY",
                                       ))
        self.wTree = gtk.glade.XML("appwindow.glade")
        dic        = {
            "on_window_destroy" : self.quit,
            "on_window_delete_event" : self.quit,
            "on_cropvideo_checkbutton_toggled" : self.cropVideoCheckButton_toggled,
            "on_enableIRLEDsCheckbox_toggled" : self.enableIRLEDsCheckbox_toggled,
            "on_enableGreenLEDCheckbox_toggled" : self.enableGreenLEDCheckbox_toggled,
            "on_enableRedLEDCheckbox_toggled" : self.enableRedLEDCheckbox_toggled,
            "on_enableBlueLEDCheckbox_toggled" : self.enableBlueLEDCheckbox_toggled
            }
        self.wTree.signal_autoconnect(dic)
        self.topwin  = self.wTree.get_widget("topwin")
        self.statusLabel = self.wTree.get_widget("status_label")
        self.statusprogbar = self.wTree.get_widget("status_progressbar")
        self.enableIRLEDsCheckbox = self.wTree.get_widget("enableIRLEDsCheckbox")
        self.enableGreenLEDCheckbox = self.wTree.get_widget("enableGreenLEDCheckbox")
        self.enableRedLEDCheckbox = self.wTree.get_widget("enableRedLEDCheckbox")
        self.enableBlueLEDCheckbox = self.wTree.get_widget("enableBlueLEDCheckbox")
        self.cropVideoCheckButton = self.wTree.get_widget("cropvideo_checkbutton")
        self.video_container = self.wTree.get_widget("video_container")
        self.max_framesize_label = self.wTree.get_widget("max_framesize_label")
        self.max_framesize_result = self.wTree.get_widget("max_framesize_result")
        self.min_vline_label = self.wTree.get_widget("min_vline_label")
        self.min_vline_result = self.wTree.get_widget("min_vline_result")
        self.max_vline_label = self.wTree.get_widget("max_vline_label")
        self.max_vline_result = self.wTree.get_widget("max_vline_result")
        self.min_hpix_label = self.wTree.get_widget("min_hpix_label")
        self.min_hpix_result = self.wTree.get_widget("min_hpix_result")
        self.max_hpix_label = self.wTree.get_widget("max_hpix_label")
        self.max_hpix_result = self.wTree.get_widget("max_hpix_result")
        self.drawarea = TIR4Drawable()
        self.video_container.add(self.drawarea)
        self.video_container.show_all()
        gobject.idle_add(self.idleTask,None)
        self.reset()
        gtk.main()
    def reset_extent_watch(self):
        self.maximum_framesize = None
        self.minimum_vline = None
        self.maximum_vline = None
        self.minimum_hpix = None
        self.maximum_hpix = None
        self.max_framesize_result.set_text("TBD")
        self.max_framesize_result.queue_draw()
        self.min_vline_result.set_text("TBD")
        self.min_vline_result.queue_draw()
        self.max_vline_result.set_text("TBD")
        self.max_vline_result.queue_draw()
        self.min_hpix_result.set_text("TBD")
        self.min_hpix_result.queue_draw()
        self.max_hpix_result.set_text("TBD")
        self.max_hpix_result.queue_draw()
    def reset(self):
        self.reset_extent_watch()
        self.statusLabel.set_label("TIR4 hardware not detected.")
        self.statusprogbar.set_fraction(0.0)
        self.enableIRLEDsCheckbox.set_active(False)
        self.enableGreenLEDCheckbox.set_active(False)
        self.enableRedLEDCheckbox.set_active(False)
        self.enableBlueLEDCheckbox.set_active(False)
        self.cropVideoCheckButton.set_active(False)
        self.enableIRLEDsCheckbox.set_sensitive(False)
        self.enableGreenLEDCheckbox.set_sensitive(False)
        self.enableRedLEDCheckbox.set_sensitive(False)
        self.enableBlueLEDCheckbox.set_sensitive(False)
        self.cropVideoCheckButton.set_sensitive(False)
        self.state = self.state_enum.NO_DEVICE
        self.video_container.set_size_request(tir4.CROPPED_NUM_HPIX,tir4.CROPPED_NUM_VLINES*2)
    def quit(self, widget, data=None):
        self.quit_worker()
    def quit_worker(self):
        gtk.main_quit()
        sys.exit()
    def msgbox(self, message):
        dialog = gtk.Dialog('Error',
                            self.topwin,  # the window that spawned this dialog
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_OK, gtk.RESPONSE_ACCEPT,)
                            )
        dialog.vbox.pack_start(gtk.Label(message))
        dialog.show_all()
        dialog.run()
        dialog.destroy()
    def msgbox_and_die(self, message):
        self.msgbox(message)
        self.quit_worker()
    def handle_exception(self, errorinfo):
        if errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.USB_LIST_FAILED:
            self.msgbox_and_die("Unable to obtain a list the USB devices.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.FIND_DEVICE_FAILED:
            self.msgbox_and_die("Unable to find TIR4 device.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.CREATE_HANDLE_FAILED:
            self.msgbox_and_die("Unable to create a handle for the TIR4 device.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.CLAIM_FAILED:
            self.msgbox_and_die("Unable to claim the TIR4 device.\nThis is most likely a permissions problem.\nRunning this app sudo is recommended.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.DISCONNECT:
            self.msgbox_and_die("TIR4 device disconnected.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.UNKNOWN_READ_ERROR:
            self.msgbox_and_die("Unknown usb read error.\nApplication will close.")
        elif errorinfo.args[0] == tir4.TIR4EXCEPTION_ENUM.UNKNOWN_PACKET:
            self.msgbox("Unknown usb read error.\nApplication will close.")
    def idleTask(self, widget, data=None):
        try:
            if self.state == self.state_enum.NO_DEVICE:
                if self.tir4.is_device_present():
                    self.statusLabel.set_label("TIR Device Found.  Initializing...")
                    self.state = self.state_enum.OPEN_DEVICE
            elif self.state == self.state_enum.OPEN_DEVICE:
                self.tir4.do_init_step_start()
                self.progbar_fract = 0.0
                self.tir4.set_init_step_5percent_callback(self.tick_progress_bar)
                self.state = self.state_enum.DEVICE_LOADING
            elif self.state == self.state_enum.DEVICE_LOADING:
                self.tir4.do_init_step()
                if self.tir4.is_init_step_done():
                    self.state = self.state_enum.DEVICE_READYING
            elif self.state == self.state_enum.DEVICE_READYING:
                self.statusprogbar.set_fraction(1.0)
                self.tir4.set_all_led_off()
                self.tir4.set_ir_led_on(True)
                self.tir4.set_green_led_on(True)
                self.video_container.show()
                self.enableIRLEDsCheckbox.set_active(True)
                self.enableGreenLEDCheckbox.set_active(True)
                self.cropVideoCheckButton.set_active(True)
                self.statusLabel.set_label("Initialization complete.  Device Ready.")
                self.enableIRLEDsCheckbox.set_sensitive(True)
                self.enableGreenLEDCheckbox.set_sensitive(True)
                self.enableRedLEDCheckbox.set_sensitive(True)
                self.enableBlueLEDCheckbox.set_sensitive(True)
                self.cropVideoCheckButton.set_sensitive(True)
                self.tir4.set_crop_frames(True)
                self.state = self.state_enum.DEVICE_READY
            elif self.state == self.state_enum.DEVICE_READY:
                self.tir4.do_read_usb()
                self.tir4.process_readbyteq()
                if self.tir4.is_frame_available():
                    frame = self.tir4.pop_frame()
                    blobs = frame.find_blobs()
                    blobpoints = []
                    for i, blob in zip(range(len(blobs)), blobs):
                        blobpoints+=[blob.get_center_coords()]
                    self.drawarea.set_frame(frame)
                    self.drawarea.set_blobpoints(blobpoints)
                    self.drawarea.queue_draw()
                    self.tir4.trim()
                    for stripe in frame:
                        self.update_all_minmax(stripe)

                    (self.maximum_framesize, updated) = self.update_max(self.maximum_framesize, len(frame))
                    if updated:
                        self.max_framesize_result.set_label(str(self.maximum_framesize))
                        self.max_framesize_result.queue_draw()
        except tir4.TIR4Exception, errorinfo:
            gobject.idle_add(self.handle_exception,errorinfo)
            return False
        return True
    def update_all_minmax(self, stripe):
        minmax_list = [self.minimum_vline, 
                       self.maximum_vline,
                       self.minimum_hpix,
                       self.maximum_hpix,
                       self.minimum_hpix,
                       self.maximum_hpix]
        input_list = [stripe.vline,
                      stripe.vline,
                      stripe.hstart,
                      stripe.hstart,
                      stripe.hstart,
                      stripe.hstart]
        domin_list = [True,
                      False,
                      True,
                      False,
                      True,
                      False]
        for i, mm, inp, domin in zip(range(len(minmax_list)), minmax_list, input_list, domin_list):
            (nmm, updated) = self.update_minmax(mm,inp,domin)
            if i == 0:
                self.minimum_vline = nmm
                self.min_vline_result.set_label(str(self.minimum_vline))
                self.min_vline_result.queue_draw()
            elif i == 1:
                self.maximum_vline = nmm
                self.max_vline_result.set_label(str(self.maximum_vline))
                self.max_vline_result.queue_draw()
            elif i in (2,4):
                self.minimum_hpix = nmm
                self.min_hpix_result.set_label(str(self.minimum_hpix))
                self.min_hpix_result.queue_draw()
            elif i in (3,5):
                self.maximum_hpix = nmm
                self.max_hpix_result.set_label(str(self.maximum_hpix))
                self.max_hpix_result.queue_draw()
    def update_min(self, minimum, input):
        return self.update_minmax(minimum, input, True)
    def update_max(self, maximum, input):
        return self.update_minmax(maximum, input, False)
    def update_minmax(self, minmax, input, do_min):
        returnval = (minmax, False)
        if minmax == None:
            returnval = (input, True)
        else:
            if do_min == True:
                cmp = input - minmax
            else:
                cmp = minmax - input
            if cmp < 0:
                returnval = (input, True)
        return returnval
    def tick_progress_bar(self):
        self.progbar_fract += 0.05
        if self.progbar_fract > 1.0:
            self.progbar_fract = 1.0
            self.statusprogbar.set_fraction(self.progbar_fract)
    def enableIRLEDsCheckbox_toggled(self, widget):
        self.tir4.set_ir_led_on(widget.get_active())
    def enableGreenLEDCheckbox_toggled(self, widget):
        self.tir4.set_green_led_on(widget.get_active())
    def enableRedLEDCheckbox_toggled(self, widget):
        self.tir4.set_red_led_on(widget.get_active())
    def enableBlueLEDCheckbox_toggled(self, widget):
        self.tir4.set_blue_led_on(widget.get_active())
    def cropVideoCheckButton_toggled(self, widget):
        self.reset_extent_watch()
        if widget.get_active():
            self.video_container.set_size_request(tir4.CROPPED_NUM_HPIX,tir4.CROPPED_NUM_VLINES*2)
            self.tir4.set_crop_frames(True)
        else:
            self.video_container.set_size_request(tir4.RAW_NUM_HPIX,tir4.RAW_NUM_VLINES*2)
            self.tir4.set_crop_frames(False)

class TIR4Drawable(gtk.DrawingArea):
    def __init__(self):
        gtk.DrawingArea.__init__(self)
        self.gc = None  # initialized in realize-event handler
        self.width  = 0 # updated in size-allocate handler
        self.height = 0 # idem
        self.connect('size-allocate', self.on_size_allocate)
        self.connect('size-request', self.on_size_request)
        self.connect('expose-event',  self.on_expose_event)
        self.connect('realize',       self.on_realize)
        self.cropped = False
        self.frame = []
        self.blobpoints = []
    def on_realize(self, widget):
        self.stripe_gc = widget.window.new_gc()
        self.stripe_gc.set_line_attributes(1, gtk.gdk.LINE_SOLID,
                                           gtk.gdk.CAP_ROUND, gtk.gdk.JOIN_ROUND)
        self.good_blobpoints_gc = widget.window.new_gc()
        self.bad_blobpoints_gc = widget.window.new_gc()
#        self.blobpoints_gc.set_line_attributes(3, gtk.gdk.LINE_ON_OFF_DASH,
#                                               gtk.gdk.CAP_ROUND, gtk.gdk.JOIN_ROUND)
        self.good_blobpoints_gc.set_line_attributes(3, gtk.gdk.LINE_SOLID,
                                                    gtk.gdk.CAP_ROUND, gtk.gdk.JOIN_ROUND)
        self.bad_blobpoints_gc.set_line_attributes(3, gtk.gdk.LINE_SOLID,
                                                   gtk.gdk.CAP_ROUND, gtk.gdk.JOIN_ROUND)
        cmap = self.get_colormap()
        self.white = cmap.alloc_color("white")
        self.black = cmap.alloc_color("black")
        self.green = cmap.alloc_color("green")
        self.red = cmap.alloc_color("red")
        self.stripe_gc.set_foreground(self.white)
        self.stripe_gc.set_background(self.black)
        self.good_blobpoints_gc.set_foreground(self.green)
        self.bad_blobpoints_gc.set_foreground(self.red)
        widget.modify_bg(gtk.STATE_NORMAL, self.black)
    def set_frame(self, frame):
        self.frame = frame
    def set_blobpoints(self, blobpoints):
        self.blobpoints = blobpoints
    def on_size_request(self, width, height):
        self.width = width
        self.height = height
    def on_size_allocate(self, widget, allocation):
        self.width = allocation.width
        self.height = allocation.height
    def on_expose_event(self, widget, event):
        # This is where the drawing takes place
        for stripe in self.frame:
            sp = (self.width-1-stripe.hstart,
                  2*stripe.vline)
            ep = (self.width-1-stripe.hstop,
                  2*stripe.vline)
            widget.window.draw_line(self.stripe_gc, sp[0], sp[1], ep[0], ep[1])
#         if len(self.blobpoints) == 3:
#             cpfs = []
#             for i, pt in zip(range(len(self.blobpoints)), self.blobpoints):
#                 cpfs += [(pt[1], 2*pt[0])]
#             print "[",
#             for cpf in cpfs:
#                 print ("%s %s;" % (cpf[0],cpf[1])), 
#             print "]"
        for i, pt in zip(range(len(self.blobpoints)), self.blobpoints):
            if i < 3:
                working_gc = self.good_blobpoints_gc
            else:
                working_gc = self.bad_blobpoints_gc
            sp = (self.width-1-int(round(pt[1]))-5, int(round(2*pt[0]))-5)
            ep = (self.width-1-int(round(pt[1]))+5, int(round(2*pt[0]))+5)
            widget.window.draw_line(working_gc, sp[0], sp[1], ep[0], ep[1])
            sp = (self.width-1-int(round(pt[1]))+5, int(round(2*pt[0]))-5)
            ep = (self.width-1-int(round(pt[1]))-5, int(round(2*pt[0]))+5)
            widget.window.draw_line(working_gc, sp[0], sp[1], ep[0], ep[1])
            

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
    
    topwin = appwindow()
    gtk.main()
    
if __name__ == "__main__":
    sys.exit(main(sys.argv))
