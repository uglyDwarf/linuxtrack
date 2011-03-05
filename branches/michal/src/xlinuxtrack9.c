#define XPLM200

#include "XPLMPlugin.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "linuxtrack.h"

static XPLMHotKeyID		gFreezeKey = NULL;
static XPLMHotKeyID		gTrackKey = NULL;

static XPLMDataRef		head_x = NULL;
static XPLMDataRef		head_y = NULL;
static XPLMDataRef		head_z = NULL;
static XPLMDataRef		head_psi = NULL;
static XPLMDataRef		head_the = NULL;
static XPLMDataRef              view = NULL;

static XPLMDataRef PV_Enabled_DR = NULL;
static XPLMDataRef PV_TIR_X_DR = NULL;
static XPLMDataRef PV_TIR_Y_DR = NULL;
static XPLMDataRef PV_TIR_Z_DR = NULL;
static XPLMDataRef PV_TIR_Pitch_DR = NULL;
static XPLMDataRef PV_TIR_Heading_DR = NULL;
static XPLMDataRef PV_TIR_Roll_DR = NULL;

static XPLMMenuID		setupMenu = NULL;
			
static int pos_init_flag = 0;
static bool freeze = false;

static float base_x;
static float base_y;
static float base_z;
static float base_psi;
static float base_the;
static bool active_flag = false;
static bool pv_present = false;

static XPLMCommandRef run_cmd;
static XPLMCommandRef pause_cmd;
static XPLMCommandRef recenter_cmd;
static bool initialized = false;

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
  
  view = XPLMFindDataRef("sim/graphics/view/view_type");
  
  if((head_x==NULL)||(head_y==NULL)||(head_z==NULL)||
     (head_psi==NULL)||(head_the==NULL)){
    return(0);
  }
  
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
  if(initialized){
    ltr_shutdown();
  }
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
  if(inFromWho == XPLM_PLUGIN_XPLANE){
    switch(inMessage){
      case XPLM_MSG_PLANE_LOADED:
        if((int)inParam == XPLM_PLUGIN_XPLANE){
          if(!initialized){
            if(ltr_init(NULL)==0){
              int c = 5;
              while(c >= 0){
                if(ltr_get_tracking_state() == RUNNING) break;
                sleep(1);
                --c;
              }
              initialized = true;
              ltr_suspend();
            }
          }


          PV_Enabled_DR = XPLMFindDataRef("sandybarbour/pv/enabled");
          PV_TIR_X_DR = XPLMFindDataRef("sandybarbour/pv/tir_x");
          PV_TIR_Y_DR = XPLMFindDataRef("sandybarbour/pv/tir_y");
          PV_TIR_Z_DR = XPLMFindDataRef("sandybarbour/pv/tir_z");
          PV_TIR_Pitch_DR = XPLMFindDataRef("sandybarbour/pv/tir_pitch");
          PV_TIR_Heading_DR = XPLMFindDataRef("sandybarbour/pv/tir_heading");
          PV_TIR_Roll_DR = XPLMFindDataRef("sandybarbour/pv/tir_roll");
          
          if((PV_Enabled_DR == NULL) || (PV_TIR_X_DR == NULL) || 
             (PV_TIR_Y_DR == NULL) || (PV_TIR_Z_DR == NULL) ||
             (PV_TIR_Pitch_DR == NULL) || (PV_TIR_Heading_DR == NULL) || 
             (PV_TIR_Roll_DR == NULL)){
            pv_present = false;
          }else{
            pv_present = true;
          }
        }
        break;
      default:
        break;
    }
  }
}

static void activate(void)
{
  if(initialized){
	  active_flag=true;
          pos_init_flag = 1;
	  freeze = false;
	  ltr_wakeup();
  }
}

static void deactivate(void)
{
	active_flag=false;
        int current_view = XPLMGetDatai(view);
        if((!pv_present) && (current_view == 1026)){
          XPLMSetDataf(head_x,base_x);
          XPLMSetDataf(head_y,base_y);
          XPLMSetDataf(head_z,base_z);
	  XPLMSetDataf(head_psi,base_psi);
	  XPLMSetDataf(head_the,base_the);
        }
    if(initialized){
	  ltr_suspend();
	}
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
      if(initialized){
        ltr_recenter();
      }
      break;
  }
}

static int cmd_cbk(XPLMCommandRef       inCommand,
                   XPLMCommandPhase     inPhase,
                   void *               inRefcon)
{
  (void) inRefcon;
    if(inPhase == xplm_CommandBegin){
      if(inCommand == run_cmd){
        if(active_flag==false){
          activate();
        }else{
          deactivate();
        }
      }else if(inCommand == pause_cmd){
        freeze = (freeze == false)? true : false;
      }else if(inCommand == recenter_cmd){
        ltr_recenter();
      }
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
  
  bool view_changed = XPLMGetDatai(view) != 1026;

  if(pos_init_flag){
    pos_init_flag = 0;
    base_x = XPLMGetDataf(head_x);
    base_y = XPLMGetDataf(head_y);
    base_z = XPLMGetDataf(head_z);
    base_psi = XPLMGetDataf(head_psi);
    base_the = XPLMGetDataf(head_the);
    view_changed = false;
  }
  if(PV_Enabled_DR)
    XPLMSetDatai(PV_Enabled_DR, active_flag);
  
  if(!active_flag){
    return -1.0;
  }

  float heading = 0.0;
  float pitch = 0.0;
  float roll = 0.0;
  float tx = 0.0;
  float ty = 0.0;
  float tz = 0.0;
  int retval;
  unsigned int counter;
  
  if(initialized){
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                   &tx, &ty, &tz, &counter);
  }
  
  if (retval < 0) {
    printf("xlinuxtrack: Error code %d detected!\n", retval);
    return -1.0;
  }
  
  if(freeze == true){
    return -1.0;
  }
  
  tx *= 1e-3;
  ty *= 1e-3;
  tz *= 1e-3;
  if(pv_present){
    XPLMSetDataf(PV_TIR_X_DR, tx);
    XPLMSetDataf(PV_TIR_Y_DR, ty);
    XPLMSetDataf(PV_TIR_Z_DR, tz);
    XPLMSetDataf(PV_TIR_Pitch_DR, pitch);
    XPLMSetDataf(PV_TIR_Heading_DR, heading);
    XPLMSetDataf(PV_TIR_Roll_DR, roll);
  }else{
    if(!view_changed){
      XPLMSetDataf(head_x,base_x + tx);
      XPLMSetDataf(head_y,base_y + ty);
      XPLMSetDataf(head_z,base_z + tz);
      XPLMSetDataf(head_psi,heading);
      XPLMSetDataf(head_the,pitch);
    }
  }
  return -1.0;
}                                   

//positive x moves us to the right (meters?)
//positive y moves us up
//positive z moves us back
