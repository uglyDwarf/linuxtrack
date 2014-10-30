#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "resource.h"
#include "rest.h"

#ifdef HAVE_CONFIG_H
  #include "../../../config.h"
#endif


HINSTANCE hInst;
HANDLE mutex;
bool hidden = false;

HMODULE tvHandle;
typedef int (WINAPI *TIRViewsFun_t)(void);
TIRViewsFun_t TIRViewsVersion;
TIRViewsFun_t TIRViewsStart;
TIRViewsFun_t TIRViewsStop;
char *client = NULL;

/*  Make the class name into a global variable  */
char szClassName[ ] = "LtrWineCtrlApp";
HWND hCtrl0_0;
HWND hCtrl1_0;
HWND hwindow;               /* This is the handle for our window */


int load_fun(HMODULE lib, const char *fun_name, TIRViewsFun_t *fun)
{
  *fun = (TIRViewsFun_t)GetProcAddress(lib, fun_name);
  if(*fun == NULL){
    printf("Can't locate '%s' function!\n", fun_name);
    return 1;
  }
  return 0;
}

void message_(const char *msg)
{
  MessageBox(NULL, msg, "Fake TrackIR problem", 0);
}

UINT_PTR timer = 0;
int cntr = 0;

VOID CALLBACK TimerProcedure(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  (void) hwnd;
  (void) uMsg;
  (void) idEvent;
  (void) dwTime;
  if(tryExclusiveLock(client)){
    PostQuitMessage(0);
  };
}

/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  //printf("Msg: %d  %d\n", hwnd, message);
  switch(message){                  /* handle the messages */
    case WM_CREATE:
      hCtrl0_0 = CreateWindowEx(0, WC_BUTTON, ("Quit"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | 0x00000001,
                                10, 45, 170, 30, hwnd, (HMENU)IDQUIT, hInst, 0);
      hCtrl1_0 = CreateWindowEx(0, WC_STATIC, ("Linuxtrack " PACKAGE_VERSION ),
                                WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT,
                                18, 5, 170, 15, hwnd, (HMENU)IDQUIT+1, hInst, 0);
      hCtrl1_0 = CreateWindowEx(0, WC_STATIC, ("Mock TrackIR window"),
                                WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT,
                                18, 20, 170, 15, hwnd, (HMENU)IDQUIT+2, hInst, 0);
            /* SetDlgItemInt(hwndDlg, IDC_APPID, 2307, true); */
            tvHandle = LoadLibrary("mfc42u.dll");
            if(!tvHandle){
              /* DWORD err = GetLastError(); */
              message_("Can't load mfc42u.dll needed by TIRViews.dll.\nTry installing MFC42 (e.g. using winetricks mfc42).");
              return TRUE;
            }
            tvHandle = LoadLibrary("TIRViews.dll");
            if(!tvHandle){
              /* DWORD err = GetLastError(); */
              message_("Can't load TIRViews.dll -\n - please reinstall TrackIR firmware to get it.");
              return TRUE;
            }
            if(load_fun(tvHandle, "TIRViewsVersion", &TIRViewsVersion) ||
                load_fun(tvHandle, "TIRViewsStart", &TIRViewsStart) ||
                load_fun(tvHandle, "TIRViewsStop", &TIRViewsStop)){
              message_("Couldn't locate all necessary functions in TIRViews.dll");
              return TRUE;
            }
            client = file_path("NPClient.dll");
            if(client == NULL){
              message_("Couldn't locate NPClient.dll");
              return TRUE;
            }
            TIRViewsStart();
            if(hidden){
              timer = SetTimer(hwnd, 0, 1000, TimerProcedure);
            }
      return 0;
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
      break;
    case WM_COMMAND:
         switch(LOWORD(wParam)){
         case IDQUIT:
                   TIRViewsStop();
                   CloseHandle(mutex);
                   if(timer != 0){
                     KillTimer(hwnd, timer);
                   }
                   if(client != NULL){
                     free(client);
                     client = NULL;
                   }
          DestroyWindow(hwnd);
          break;
         }
      break;
    default:                      /* for messages that we don't deal with */
      return DefWindowProc (hwnd, message, wParam, lParam);
  }
  return 0;
}


int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
  (void) hPrevInstance;
  (void) lpszArgument;
  //MSG messages;            /* Here messages to the application are saved */
  WNDCLASSEX wincl;        /* Data structure for the windowclass */

  /* The Window structure */
  hInst = hThisInstance;

  if(nCmdShow == SW_HIDE){
    hidden = true;
  }

  mutex = CreateMutex(NULL, TRUE, "linuxtrack_wine_fake_trackir_mutex");
  if(mutex == NULL){
    message_("Can't create mutex.");
    return 0;
  }

  if(WaitForSingleObject(mutex, 0) != WAIT_OBJECT_0){
    message_("Fake TrackIR executable already running!.");
    CloseHandle(mutex);
    return 0;
  }

  wincl.hInstance = hThisInstance;
  wincl.lpszClassName = szClassName;
  wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
  wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
  wincl.cbSize = sizeof (WNDCLASSEX);

  /* Use default icon and mouse-pointer */
  wincl.hIcon = LoadIcon (hThisInstance, MAKEINTRESOURCE(1002));
  wincl.hIconSm = LoadIcon (hThisInstance, MAKEINTRESOURCE(1001));
  wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
  wincl.lpszMenuName = NULL;                 /* No menu */
  wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
  wincl.cbWndExtra = 0;                      /* structure or the window instance */
  /* Use Windows's default colour as the background of the window */
  wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

  /* Register the window class, and if it fails quit the program */
  if(!RegisterClassEx (&wincl)){
     return 0;
  }
  /* The class is registered, let's create the program*/
  hwindow = CreateWindowEx (
         0,                   /* Extended possibilites for variation */
         szClassName,         /* Classname */
         "TrackIR",           /* Title Text */
         WS_OVERLAPPEDWINDOW, /* default window */
         CW_USEDEFAULT,       /* Windows decides the position */
         CW_USEDEFAULT,       /* where the window ends up on the screen */
         200,                 /* The programs width */
         110,                 /* and height in pixels */
         HWND_DESKTOP,        /* The window is a child-window to desktop */
         NULL,                /* No menu */
         hThisInstance,       /* Program Instance handler */
         NULL                 /* No Window Creation data */
         );

  /* Make the window visible on the screen */
  if(nCmdShow == SW_HIDE){
    hidden = true;
  }
  ShowWindow (hwindow, nCmdShow);
  UpdateWindow(hwindow);
  BOOL bRet;
  MSG msg;
  while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0){
    if(bRet == -1){
      break;
    }else{
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return msg.wParam;
}

