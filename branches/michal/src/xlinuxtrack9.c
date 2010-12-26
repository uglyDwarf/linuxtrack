#define XPLM200

#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "ltlib.h"

static XPLMHotKeyID		gFreezeKey = NULL;
static XPLMHotKeyID		gTrackKey = NULL;

static XPLMDataRef		head_x = NULL;
static XPLMDataRef		head_y = NULL;
static XPLMDataRef		head_z = NULL;
static XPLMDataRef		head_psi = NULL;
static XPLMDataRef		head_the = NULL;

static XPLMMenuID		setupMenu = NULL;
			
static int pos_init_flag = 0;
static bool freeze = false;

static float base_x;
static float base_y;
static float base_z;
static bool active_flag = false;

static XPLMCommandRef run_cmd;
static XPLMCommandRef pause_cmd;
static XPLMCommandRef recenter_cmd;


static void MyHotKeyCallback(void *inRefcon);    
static int cmd_cbk(XPLMCommandRef       inCommand,
                   XPLMCommandPhase     inPhase,
                   void *               inRefcon);

enum {START, PAUSE, RECENTER};

static float xlinuxtrackCallback(float inElapsedSinceLastCall,    
			      float inElapsedTimeSinceLastFlightLoop,    
			      int   inCounter,    
			      void *inRefcon);

static void linuxTrackMenuHandler(void *inMenuRef, void *inItemRef)
{
  (void) inMenuRef;
  (void) inItemRef;
  return;
}

PLUGIN_API int XPluginStart(char *outName,
                            char *outSig,
                            char *outDesc)
{
  strcpy(outName, "linuxTrack");
  strcpy(outSig, "linuxtrack.camera");
  strcpy(outDesc, "A plugin that controls view using your webcam.");

  int xplane_ver;
  int sdk_ver;
  XPLMHostApplicationID app_id;
  XPLMGetVersions(&xplane_ver, &sdk_ver, &app_id);

  /* Register our hot key for the new view. */
  gTrackKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, 
                         "3D linuxTrack view",
                         MyHotKeyCallback,
                         (void*)START);
  gFreezeKey = XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag, 
                         "Freeze 3D linuxTrack view",
                         MyHotKeyCallback,
                         (void*)PAUSE);

  run_cmd = XPLMCreateCommand("sim/view/ltr_run","Start/stop tracking");
  pause_cmd = XPLMCreateCommand("sim/view/ltr_pause","Pause tracking");
  recenter_cmd = XPLMCreateCommand("sim/view/ltr_recenter","Recenter tracking");

        XPLMRegisterCommandHandler(
                    run_cmd,
                    cmd_cbk,
                    true,
                    (void *)START);
        XPLMRegisterCommandHandler(
                    pause_cmd,
                    cmd_cbk,
                    true,
                    (void *)PAUSE);
        XPLMRegisterCommandHandler(
                    recenter_cmd,
                    cmd_cbk,
                    true,
                    (void *)RECENTER);

                         
  head_x = XPLMFindDataRef("sim/graphics/view/pilots_head_x");
  head_y = XPLMFindDataRef("sim/graphics/view/pilots_head_y");
  head_z = XPLMFindDataRef("sim/graphics/view/pilots_head_z");
  
  head_psi = XPLMFindDataRef("sim/graphics/view/pilots_head_psi");
  head_the = XPLMFindDataRef("sim/graphics/view/pilots_head_the");
  
  if((head_x==NULL)||(head_y==NULL)||(head_z==NULL)||
     (head_psi==NULL)||(head_the==NULL)){
    return(0);
  }
  if(ltr_init("Default")!=0){
    return(0);
  }
  ltr_suspend();
  
  XPLMRegisterFlightLoopCallback(		
	xlinuxtrackCallback,	/* Callback */
	-1.0,					/* Interval */
	NULL);					/* refcon not used. */
  int index = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "LinuxTrack", NULL, 1);


  setupMenu = XPLMCreateMenu("LinuxTrack", XPLMFindPluginsMenu(), index, 
                         linuxTrackMenuHandler, NULL);
  XPLMAppendMenuItem(setupMenu, "Setup", (void *)"Setup", 1);
  return(1);
}

PLUGIN_API void	XPluginStop(void)
{
  XPLMUnregisterHotKey(gTrackKey);
  XPLMUnregisterHotKey(gFreezeKey);
  XPLMUnregisterFlightLoopCallback(xlinuxtrackCallback, NULL);
  ltr_shutdown();
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho,
                                      long inMessage,
                                      void *inParam)
{
  (void) inFromWho;
  (void) inMessage;
  (void) inParam;
}

static void activate(void)
{
	  active_flag=true;
          pos_init_flag = 1;
	  freeze = false;
	  XPLMCommandKeyStroke(xplm_key_forward);
	  ltr_wakeup();
}

static void deactivate(void)
{
	  active_flag=false;
          XPLMSetDataf(head_x,base_x);
          XPLMSetDataf(head_y,base_y);
          XPLMSetDataf(head_z,base_z);
	  XPLMSetDataf(head_psi,0.0f);
	  XPLMSetDataf(head_the,0.0f);
	  ltr_suspend();
}

static void MyHotKeyCallback(void *inRefcon)
{
  switch((int)inRefcon){
    case START:
      if(active_flag==false){
	activate();
      }else{
	deactivate();
      }
      break;
    case PAUSE:
      freeze = (freeze == false)? true : false;
      break;
    case RECENTER:
      ltr_recenter();
      break;
  }
}

static int cmd_cbk(XPLMCommandRef       inCommand,
                   XPLMCommandPhase     inPhase,
                   void *               inRefcon)
{
    (void) inCommand;
    //(void) inPhase;
    //(void) inRefcon;

    if (inPhase == xplm_CommandBegin) {
        MyHotKeyCallback(inRefcon);
    }
    return 1;
}

static float xlinuxtrackCallback(float inElapsedSinceLastCall,    
                              float inElapsedTimeSinceLastFlightLoop,    
                              int   inCounter,    
                              void *inRefcon)
{
  (void) inElapsedSinceLastCall;
  (void) inElapsedTimeSinceLastFlightLoop;
  (void) inCounter;
  (void) inRefcon;
  if(active_flag){
    float heading, pitch, roll;
    float tx, ty, tz;
    int retval;
    unsigned int counter;
    
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz, &counter);

    if (retval < 0) {
      printf("xlinuxtrack: Error code %d detected!\n", retval);
      return -1;
    }

    if(freeze == true){
      return -1;
    }
    
    tx *= 1e-3;
    ty *= 1e-3;
    tz *= 1e-3;

    if(pos_init_flag == 1){
      pos_init_flag = 0;
      base_x = XPLMGetDataf(head_x);
      base_y = XPLMGetDataf(head_y);
      base_z = XPLMGetDataf(head_z);
    }
      
    XPLMSetDataf(head_x,base_x + tx);
    XPLMSetDataf(head_y,base_y + ty);
    XPLMSetDataf(head_z,base_z + tz);
    XPLMSetDataf(head_psi,heading);
    XPLMSetDataf(head_the,pitch);
  }
  
  return -1.0;
}                                   

//positive x moves us to the right (meters?)
//positive y moves us up
//positive z moves us back
