#import <Cocoa/Cocoa.h>

@interface OscServerAppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
	IBOutlet NSButton *StartStop;
	IBOutlet NSButton *Pause;
	IBOutlet NSButton *Recenter;
	IBOutlet NSTextField *status;
	IBOutlet NSTextField *portSelector;
	NSTimer *timer;
	NSUserDefaults *prefs;
	bool paused;
	bool stopped;
}

@property (assign) IBOutlet NSWindow *window;
- (IBAction) StartStopPressed:(id) sender;
- (IBAction) PausePressed:(id) sender;
- (IBAction) RecenterPressed:(id) sender;

-(void) timerCallback:(NSTimer*)theTimer;

@end
