#import "OscServerAppDelegate.h"
#import "osc_server.h"
#import "linuxtrack.h"

@implementation OscServerAppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application
	paused = false;
	stopped = true;
	timer = nil;
	prefs = [NSUserDefaults standardUserDefaults];
	NSInteger portStr = [prefs integerForKey:@"ServerPort"];
	if(!portStr){
		portStr = 44444;
	}
	[portSelector setIntValue:(int)portStr];
	[Pause setEnabled:false];
	[Recenter setEnabled:false];
}

- (IBAction) StartStopPressed:(id) sender
{
	if(stopped){
		[StartStop setTitle:@"Stop Tracking"];
		[Pause setEnabled:true];
		[Recenter setEnabled:true];
		if(!timer){
			timer = [NSTimer scheduledTimerWithTimeInterval:0.2 target:self 
					 selector:@selector(timerCallback:) userInfo:nil repeats:YES];
			[timer retain];
		}
		NSInteger port = [portSelector integerValue];
		[prefs setInteger:port forKey:@"ServerPort"];
		[prefs synchronize];
		[portSelector setEnabled:false];
        oscServerInit((int)port);
		stopped = false;
	}else{
	  stopTracking();
		stopped = true;
		[timer invalidate];
		timer = nil;
		[status setStringValue:@"Linuxtrack stopped"];
		[portSelector setEnabled:true];
		[Pause setEnabled:false];
		[Recenter setEnabled:false];
		[StartStop setTitle:@"Start Tracking"];
	}
}

- (IBAction) PausePressed:(id) sender
{
	if(paused){
		linuxtrack_wakeup();
		paused = false;
		[Pause setTitle:@"Pause"];
	}else{
		linuxtrack_suspend();
		paused = true;
		[Pause setTitle:@"Resume"];
	}
}

- (IBAction) RecenterPressed:(id) sender
{
	linuxtrack_recenter();
}

-(void) timerCallback:(NSTimer*)theTimer
{
	(void) theTimer;
    [status setStringValue:[NSString stringWithCString:linuxtrack_explain(linuxtrack_get_tracking_state()) 
							encoding:NSUTF8StringEncoding]];
}

@end
