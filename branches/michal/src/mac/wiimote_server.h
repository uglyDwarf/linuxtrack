//
//  wiimote_server.h
//  WiiSrvr
//
//  Created by Michal Navratil on 8/30/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "wiimote.h"

@interface wiimote_server : NSObject {
  IBOutlet NSButton *connect;
  IBOutlet NSTextField *status;
  Wii *wiimote;
  NSTimer *timer;
}
  - (void) dealloc;
  - (IBAction) connectPressed:(id) sender;
  -(void) timerCallback:(NSTimer*)theTimer;

  
@end
