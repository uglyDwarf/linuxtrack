#include <string.h>
#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <math_utils.h>
#include "ltlib.h"

XPLMHotKeyID	gHotKey = NULL;
XPLMDataRef		head_x = NULL;
XPLMDataRef		head_y = NULL;
XPLMDataRef		head_z = NULL;
XPLMDataRef		head_psi = NULL;
XPLMDataRef		head_the = NULL;

XPLMDataRef		joy_buttons = NULL;
int 			buttons[1520];
int 			button_number = 3;

int pos_init_flag = 0;
bool freeze = false;

float base_x;
float base_y;
float base_z;


void	MyHotKeyCallback(void *               inRefcon);    

int	AircraftDrawCallback(	XPLMDrawingPhase     inPhase,
                          int                  inIsBefore,
                          void *               inRefcon);


PLUGIN_API int XPluginStart(
                            char *		outName,
                            char *		outSig,
                            char *		outDesc)
{
  struct lt_configuration_type ltconf;
  strcpy(outName, "linuxTrack");
  strcpy(outSig, "linuxtrack.camera");
  strcpy(outDesc, "A plugin that controls view using your webcam.");

  /* Register our hot key for the new view. */
  gHotKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, 
                         "3D linuxTrack view",
                         MyHotKeyCallback,
                         NULL);
  head_x = XPLMFindDataRef("sim/graphics/view/pilots_head_x");
  head_y = XPLMFindDataRef("sim/graphics/view/pilots_head_y");
  head_z = XPLMFindDataRef("sim/graphics/view/pilots_head_z");
  head_psi = XPLMFindDataRef("sim/graphics/view/pilots_head_psi");
  head_the = XPLMFindDataRef("sim/graphics/view/pilots_head_the");
  joy_buttons = XPLMFindDataRef("sim/joystick/joystick_button_values");
  
  if((head_x==NULL)||(head_y==NULL)||(head_z==NULL)||
     (head_psi==NULL)||(head_the==NULL)||(joy_buttons==NULL)){
    return(0);
  }
  if(lt_init(ltconf, "XPlane")!=0){
    return(0);
  }
  return(1);
}

PLUGIN_API void	XPluginStop(void)
{
  XPLMUnregisterHotKey(gHotKey);
  lt_shutdown();
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(
                                      XPLMPluginID	inFromWho,
                                      long			inMessage,
                                      void *			inParam)
{
}

void	MyHotKeyCallback(void *               inRefcon)
{
  static int active=0;
	/* This is the hotkey callback.  First we simulate a joystick press and
	 * release to put us in 'free view 1'.  This guarantees that no panels
	 * are showing and we are an external view. */
	 
	/* Now we control the camera until the view changes. */
	if(active==0){
	  active=1;
          pos_init_flag = 1;
	  freeze = false;
          lt_recenter();
          XPLMRegisterDrawCallback(
                             AircraftDrawCallback,
                             xplm_Phase_LastCockpit,
                             0,
                             NULL);
	}else{
	  active=0;
	  XPLMUnregisterDrawCallback(
                               AircraftDrawCallback,
                               xplm_Phase_LastCockpit,
                               0,
                               NULL);
                               
          XPLMSetDataf(head_x,base_x);
          XPLMSetDataf(head_y,base_y);
          XPLMSetDataf(head_z,base_z);
	  XPLMSetDataf(head_psi,0.0f);
	  XPLMSetDataf(head_the,0.0f);
	}
}

void process_joy()
{
  static int state = 1;
  static float ts;
  
  XPLMGetDatavi(joy_buttons, buttons, 0, 1520);
  int button = buttons[button_number];
  switch(state){
    case 1: //Waiting for button to be pressed
      if(button != 0){
        ts = XPLMGetElapsedTime();
	state = 2;
	printf("Switching to state 2\n");
      }
      break;
    case 2: //Counting...
      if(button == 0){
        state = 1; //button was released, go back
      }else{
        if((XPLMGetElapsedTime() - ts) > 0.025){
	  freeze = (freeze == false)? true : false;
	  state = 3;
          printf("Switching to state 3\n");
	}
      }
      break;
    case 3: //Waiting for button to be released
      if(button == 0){
        ts = XPLMGetElapsedTime();
	state = 4;
	printf("Switching to state 4\n");
      }
      break;
    case 4:
      if(button != 0){
        state = 3; //button was pressed again, go back
      }else{
        if((XPLMGetElapsedTime() - ts) > 0.025){
	  state = 1;
	  printf("Switching to state 1\n");
	}
      }
      break;
    default:
      printf("Joystick button processing got into wrong state (%d)!\n", state);
      state = 1;
      break;
  }
}

int	AircraftDrawCallback(	XPLMDrawingPhase     inPhase,
                          int                  inIsBefore,
                          void *               inRefcon)
{
  float heading, pitch, roll;
  float tx, ty, tz;
  int retval;
  
  process_joy(button_number);
  retval = lt_get_camera_update(&heading,&pitch,&roll,
                                &tx, &ty, &tz);

  if(is_finite(heading) && is_finite(pitch) && is_finite(roll) &&
     is_finite(tx) && is_finite(ty) && is_finite(tz)){
    // Empty
  }else{
    printf("Bad values!\n");
    return 1;
  }

  if (retval < 0) {
    printf("xlinuxtrack: Error code %d detected!\n", retval);
    return 1;
  }
  if(freeze == true){
    return 1;
  }
  
  tx *= 1e-3;
  ty *= 1e-3;
  tz *= 1e-3;
/*   printf("heading: %f\tpitch: %f\n", heading, pitch); */ 
/*   printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz); */

  /* Fill out the camera position info. */
  /* FIXME: not doing translation */
  /* FIXME: not roll, is this even possible? */
  
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
	return 1;
}                                   

//positive x moves us to the right (meters?)
//positive y moves us up
//positive z moves us back
