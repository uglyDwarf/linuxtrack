//
//  main.m
//  WiiSrvr
//
//  Created by Michal Navratil on 8/30/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <utils.h>

int main(int argc, char *argv[])
{
    ltr_int_set_logfile_name("/tmp/linuxtrack.wii");
    ltr_int_log_message("Starting Wiimote server!\n");
    return NSApplicationMain(argc,  (const char **) argv);
}
