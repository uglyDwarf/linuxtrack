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
#include "xlinuxtrack_pref.h"

XPLMHotKeyID		gFreezeKey = NULL;
XPLMHotKeyID		gTrackKey = NULL;

XPLMDataRef		head_x = NULL;
XPLMDataRef		head_y = NULL;
XPLMDataRef		head_z = NULL;
XPLMDataRef		head_psi = NULL;
XPLMDataRef		head_the = NULL;

XPWidgetID		setupWindow = NULL;
XPWidgetID		mapText = NULL;
XPWidgetID		saveButton = NULL;
			
XPLMMenuID		setupMenu = NULL;
			
XPWidgetID		jmWindow;
XPWidgetID		jmText;
XPWidgetID		jmButton;
int			jmWindowOpened = 0;
int			jmRun = 0;

XPLMDataRef		joy_buttons = NULL;
int 			buttons[1520];

int 			buttonIndex = -1;
char			text[150];

int 			freeze_button = -1;
int			recenter_button = -1;

float			debounce_time = 0.01;

int button_array_size = -1;
int pos_init_flag = 0;
bool freeze = false;

float base_x;
float base_y;
float base_z;
bool active_flag = false;






void	MyHotKeyCallback(void *               inRefcon);    

/*
int	AircraftDrawCallback(	XPLMDrawingPhase     inPhase,
                          int                  inIsBefore,
                          void *               inRefcon);
*/
float AircraftDrawCallback(          float                inElapsedSinceLastCall,
                                   float                inElapsedTimeSinceLastFlightLoop,
                                   int                  inCounter,
                                   void *               inRefcon);    


float	joystickCallback(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon);

bool is_finite(float f)
{
  if(finite(f) != 0){
    return true;
  }else{
    return false;
  }
}

bool createSetupWindow(int x, int y, int w, int h);

struct buttonDef{
  char *caption;
  XPWidgetID text;
  XPWidgetID button;
  char *prefName;
  enum pref_id id;
};

struct buttonDef btArray[] = {
  {
    .caption = "Start/Stop tracking:",
    .prefName = "Recenter-button",
    .id = START_STOP
  },
  {
    .caption = "Tracking freeze:",
    .prefName = "Freeze-button",
    .id = PAUSE
  }
};

struct pref *xltrprefs = NULL;
char *pref_fname = NULL;

void linuxTrackMenuHandler(void *inMenuRef, void *inItemRef)
{
  (void) inMenuRef;
  (void) inItemRef;
  if(setupWindow == NULL){
    createSetupWindow(100, 600, 300, 370);
  }else{
    if(!XPIsWidgetVisible(setupWindow)){
      XPShowWidget(setupWindow);
    }
  }
  return;
}

int jmProcessJoy()
{
  int new_buttons[1520];
  int i = 0;
  XPLMGetDatavi(joy_buttons, new_buttons, 0, button_array_size);
  for(i = 0;i < button_array_size;++i){
    if(new_buttons[i] != buttons[i]){
      return i;
    }
  }
  return -1.0;
}

bool updateButtonCaption(int index, int button)
{
  if(index < 0){
    return false;
  }
  if((button < 0) || (button > button_array_size)){
    lt_log_message("Xlinuxtrack: wrong button number! %d \n", button);
    return true;
  }
  if(button >= 0){
    sprintf(text, "%s button %d", btArray[index].caption, button);
  }else{
    sprintf(text, "%s Not mapped", btArray[index].caption);
  }
  XPSetWidgetDescriptor(btArray[index].text, text);
  
  set_pref(xltrprefs, btArray[index].id, button);
  switch(index){
    case 0:
      recenter_button = button;
      break;
    case 1:
      freeze_button = button;
      break;
    default:
      break;
  };
  return true;
}

int jmWindowHandler(XPWidgetMessage inMessage,
			XPWidgetID inWidget,
			long inParam1,
			long inParam2)
{
  (void) inWidget;
  (void) inParam1;
  (void) inParam2;

  if(inMessage == xpMsg_PushButtonPressed){
    //there is only one button
    jmRun = 0;
    if(jmWindow != NULL){
      jmWindowOpened = 0;
      buttonIndex = -1;
      XPHideWidget(jmWindow);
    }
    return 1;
  }
  return 0;
}

int joyMapDialog(char *caption)
{
  if(jmWindowOpened != 0){
    return -1;
  }
  jmWindowOpened = 1;
  
  if(jmWindow != NULL){
    XPSetWidgetDescriptor(jmText, caption);
    XPShowWidget(jmWindow);
  }else{
    int x  = 100;
    int y  = 600;
    int w  = 300;
    int h  = 100;
    int x2 = x + w;
    int y2 = y - h;

    jmWindow = XPCreateWidget(x, y, x2, y2,
  				  1, //Visible
				  "Joystick button mapping...",
				  1, //Root
				  NULL, //No container
				  xpWidgetClass_MainWindow 
    );
    jmText = XPCreateWidget(x+20, y - 20, x2 -20, y -40 ,
    				   1, caption, 0, jmWindow, 
				   xpWidgetClass_Caption);
    jmButton = XPCreateWidget(x+80, y2+40, x2-80, y2+20, 1, 
  				  "Cancel", 0, jmWindow,
  				  xpWidgetClass_Button);
    XPLMGetDatavi(joy_buttons, buttons, 0, button_array_size);
    XPAddWidgetCallback(jmWindow, jmWindowHandler);
  }
  jmRun = 1;
  return 0;
}

int setupWindowHandler(XPWidgetMessage inMessage,
			XPWidgetID inWidget,
			long inParam1,
			long inParam2)
{
  (void) inWidget;
  (void) inParam2;
  if(inMessage == xpMessage_CloseButtonPushed){
    if(setupWindow != NULL){
      XPHideWidget(setupWindow);
    }
    return 1;
  }
  if(inMessage == xpMsg_PushButtonPressed){
    if(inParam1 == (long)btArray[0].button){
      buttonIndex = 0;
      joyMapDialog("Remap joystick button for Start/Stop Tracking");
    }
    if(inParam1 == (long)btArray[1].button){
      buttonIndex = 1;
      joyMapDialog("Remap joystick button for Tracking freeze");
    }
    if(inParam1 == (long)saveButton){
      save_pref(pref_fname, xltrprefs);
    }
  }
  return 0;
}

bool createSetupWindow(int x, int y, int w, int h)
{
  int x2 = x + w;
  int y2 = y - h;
  
  setupWindow = XPCreateWidget(x, y, x2, y2,
  				1, //Visible
				"Linux Track Setup",
				1, //Root
				NULL, //No container
				xpWidgetClass_MainWindow 
  );
  
  XPSetWidgetProperty(setupWindow, xpProperty_MainWindowHasCloseBoxes, 1);
  
  XPWidgetID sw = XPCreateWidget(x+10, y-30, x2-10, y2+10, 
  				1, //Visible
				"", //Desc
				0, //Root
				setupWindow,
				xpWidgetClass_SubWindow
  );
  
  XPSetWidgetProperty(sw, xpProperty_SubWindowType, 
  				xpSubWindowStyle_SubWindow
  );
  
  mapText = XPCreateWidget(x+20, y2 + 140, x2 -20, y2 + 120 ,
    				 1, "Joystick button mapping",
				 0, setupWindow, xpWidgetClass_Caption);
  btArray[0].button = XPCreateWidget(x2-120, y2+120, x2-20, y2+100, 1, 
  				"Remap", 0, setupWindow,
  				xpWidgetClass_Button);
  btArray[0].text = XPCreateWidget(x+20, y2 + 120, x2 -140, y2 + 100 ,
    				 1, btArray[0].caption,
				 0, setupWindow, xpWidgetClass_Caption);
  btArray[1].button = XPCreateWidget(x2-120, y2+90, x2-20, y2+70, 1, 
  				"Remap", 0, setupWindow,
  				xpWidgetClass_Button);
  btArray[1].text = XPCreateWidget(x+20, y2 + 90, x2 -140, y2 + 70 ,
    				 1, btArray[1].caption,
				 0, setupWindow, xpWidgetClass_Caption);
  saveButton = XPCreateWidget(x+20, y2+30, x2-20, y2+10, 1, 
  				"Save preferences", 0, setupWindow,
  				xpWidgetClass_Button);
  XPSetWidgetProperty(btArray[0].button, xpProperty_ButtonType, xpPushButton);
  XPSetWidgetProperty(btArray[1].button, xpProperty_ButtonType, xpPushButton);
  XPSetWidgetProperty(saveButton, xpProperty_ButtonType, xpPushButton);
  updateButtonCaption(0, recenter_button);
  updateButtonCaption(1, freeze_button);

  XPAddWidgetCallback(setupWindow, setupWindowHandler);
  return true;
}


PLUGIN_API int XPluginStart(
                            char *		outName,
                            char *		outSig,
                            char *		outDesc)
{
  strcpy(outName, "linuxTrack");
  strcpy(outSig, "linuxtrack.camera");
  strcpy(outDesc, "A plugin that controls view using your webcam.");

  int xplane_ver;
  int sdk_ver;
  XPLMHostApplicationID app_id;
  XPLMGetVersions(&xplane_ver, &sdk_ver, &app_id);
  if(xplane_ver < 850){
    button_array_size = 64;
  }else if(xplane_ver < 900){
    button_array_size = 160;
  }else{
    button_array_size = 1520;
  }
  lt_log_message("%d joystick buttons\n", button_array_size);
  
  pref_fname = get_pref_file_name();
  xltrprefs = new_pref();
  read_pref(pref_fname, xltrprefs);
  
  if(!is_pref_valid(xltrprefs)){
    lt_log_message("Invalid pref definitions!\n");
  }
  freeze_button = get_pref(xltrprefs, START_STOP);
  recenter_button = get_pref(xltrprefs, PAUSE);
  
  if(freeze_button > button_array_size){
    lt_log_message("Freeze button number too big! %d\n", freeze_button);
    freeze_button = -1;
  }
  if(recenter_button > button_array_size){
    lt_log_message("Recenter button number too big! %d\n", recenter_button);
    recenter_button = -1;
  }
  
  /* Register our hot key for the new view. */
  gTrackKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, 
                         "3D linuxTrack view",
                         MyHotKeyCallback,
                         (void*)0);
  gFreezeKey = XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag, 
                         "Freeze 3D linuxTrack view",
                         MyHotKeyCallback,
                         (void*)1);
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
  if(lt_init("Default")!=0){
    return(0);
  }
  
  XPLMRegisterFlightLoopCallback(		
	joystickCallback,	/* Callback */
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
  XPLMUnregisterFlightLoopCallback(joystickCallback, NULL);
  XPLMUnregisterFlightLoopCallback(AircraftDrawCallback, NULL);
  lt_shutdown();
  free(xltrprefs);
  free(pref_fname);
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
  (void) inFromWho;
  (void) inMessage;
  (void) inParam;
}

void activate(void)
{
	  active_flag=true;
          pos_init_flag = 1;
	  freeze = false;
          lt_recenter();
	  XPLMCommandKeyStroke(xplm_key_forward);
          XPLMRegisterFlightLoopCallback(
                             AircraftDrawCallback,
                             -1,
                             NULL);
	  lt_wakeup();
}

void deactivate(void)
{
	  active_flag=false;
	  XPLMUnregisterFlightLoopCallback(
                               AircraftDrawCallback,
                               NULL);
                               
          XPLMSetDataf(head_x,base_x);
          XPLMSetDataf(head_y,base_y);
          XPLMSetDataf(head_z,base_z);
	  XPLMSetDataf(head_psi,0.0f);
	  XPLMSetDataf(head_the,0.0f);
	  lt_suspend();
}

void	MyHotKeyCallback(void *               inRefcon)
{
  switch((int)inRefcon){
    case 0:
      if(active_flag==false){
	activate();
      }else{
	deactivate();
      }
      break;
    case 1:
      freeze = (freeze == false)? true : false;
      break;
  }
}

void joy_fsm(int button, int *state, float *ts, bool *flag)
{
  switch(*state){
    case 1: //Waiting for button to be pressed
      if(button != 0){
        *ts = XPLMGetElapsedTime();
	*state = 2;
      }
      break;
    case 2: //Counting...
      if(button == 0){
        *state = 1; //button was released, go back
      }else{
        if((XPLMGetElapsedTime() - *ts) > debounce_time){
	  *flag = (*flag == false)? true : false;
	  *state = 3;
	}
      }
      break;
    case 3: //Waiting for button to be released
      if(button == 0){
        *ts = XPLMGetElapsedTime();
	*state = 4;
      }
      break;
    case 4:
      if(button != 0){
        *state = 3; //button was pressed again, go back
      }else{
        if((XPLMGetElapsedTime() - *ts) > debounce_time){
	  *state = 1;
	}
      }
      break;
    default:
      lt_log_message("Joystick button processing got into wrong state (%d)!\n", *state);
      *state = 1;
      break;
  }
}

void process_joy()
{
  static float freeze_ts;
  static float recenter_ts;
  static int freeze_state = 1;
  static int recenter_state = 1;
  
  XPLMGetDatavi(joy_buttons, buttons, 0, 1520);
  
  if((freeze_button > 0) && (freeze_button < button_array_size)){
    joy_fsm(buttons[freeze_button], &freeze_state, &freeze_ts, &freeze);
  }
  if((recenter_button > 0) && (recenter_button < button_array_size)){
    joy_fsm(buttons[recenter_button], &recenter_state, &recenter_ts, &active_flag);
  }
}

float	joystickCallback(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
  (void) inElapsedSinceLastCall;
  (void) inElapsedTimeSinceLastFlightLoop;
  (void) inCounter;
  (void) inRefcon;
	if(jmRun != 0){
	  int res = jmProcessJoy();
	  if(res != -1){
	    updateButtonCaption(buttonIndex, res);
            XPHideWidget(jmWindow);
            jmWindowOpened = 0;
	    jmRun = 0;
	  }
	}else{
          bool last_active_flag = active_flag;
          process_joy();
          if(last_active_flag != active_flag){
	    if(active_flag){
	      activate();
	    }else{
	      deactivate();
	    }
	  }
	}
	return -1.0;
}                                   



float	AircraftDrawCallback(float                inElapsedSinceLastCall,
                             float                inElapsedTimeSinceLastFlightLoop,
                             int                  inCounter,
                             void *               inRefcon)    
{
  (void)inElapsedSinceLastCall;
  (void)inElapsedTimeSinceLastFlightLoop;
  (void)inCounter;
  (void)inRefcon;
  float heading, pitch, roll;
  float tx, ty, tz;
  int retval;
  
  retval = lt_get_camera_update(&heading,&pitch,&roll,
                                &tx, &ty, &tz);

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
	return -1;
}                                   

//positive x moves us to the right (meters?)
//positive y moves us up
//positive z moves us back
