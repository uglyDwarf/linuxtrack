#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "utils.h"
#include "resource.h"
#include "kbi_interface.h"

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
char szClassName[ ] = "LtrWineCtrlApp";
HWND hCtrl0_0;
HWND hCtrl0_1;
HWND hCtrl0_2;
HWND hCtrl0_3;
HWND hCtrl0_4;
HWND hwnd;               /* This is the handle for our window */
HINSTANCE hInst;

char pause_key_desc[256];
char recenter_key_desc[256];
int pause_vkey = 0;
int pause_scancode = 0;
int recenter_vkey = 0;
int recenter_scancode = 0;
bool have_pause_key = false;
bool have_recenter_key = false;

int pause_debouncer = 0;
bool paused = false;

enum redef_state_t{REDEF_NONE, REDEF_PAUSE, REDEF_RECENTER} redef_state = REDEF_NONE;

char *prefs;
NOTIFYICONDATA nid;

#define WM_TRAY_DBL_CLICK WM_USER+1

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
  (void) hPrevInstance;
  (void) lpszArgument;
  MSG messages;            /* Here messages to the application are saved */
  WNDCLASSEX wincl;        /* Data structure for the windowclass */
  
  /* The Window structure */
  hInst = hThisInstance;
  
  /*
  printf("Prefs read!\n");
  */
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
  hwnd = CreateWindowEx (
         0,                   /* Extended possibilites for variation */
         szClassName,         /* Classname */
         "Linuxtrack - Wine Server",       /* Title Text */
         WS_OVERLAPPEDWINDOW, /* default window */
         CW_USEDEFAULT,       /* Windows decides the position */
         CW_USEDEFAULT,       /* where the window ends up on the screen */
         435,                 /* The programs width */
         176,                 /* and height in pixels */
         HWND_DESKTOP,        /* The window is a child-window to desktop */
         NULL,                /* No menu */
         hThisInstance,       /* Program Instance handler */
         NULL                 /* No Window Creation data */
         );
  
  /* Make the window visible on the screen */
  ShowWindow (hwnd, nCmdShow);
  kbi_init(hwnd);
  kbi_msg_loop();
  kbi_close();
  /* The program return-value is 0 - The value that PostQuitMessage() gave */
  return messages.wParam;
}

/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  char msg[384];
  //printf("Msg: %d  %d\n", hwnd, message);
  switch(message){                  /* handle the messages */
    case WM_CREATE:
      hCtrl0_0 = CreateWindowEx(0, WC_BUTTON, ("Quit"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | 0x00000001, 
                                336, 117, 83, 23, hwnd, (HMENU)IDQUIT, hInst, 0);
      hCtrl0_1 = CreateWindowEx(0, WC_BUTTON, ("Redefine Pause Key"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
                                241, 20, 178, 33, hwnd, (HMENU)IDC_REDEFINE_PAUSE, hInst, 0);
      hCtrl0_2 = CreateWindowEx(0, WC_BUTTON, ("Redefine Recenter Key"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
                                241, 68, 178, 33, hwnd, (HMENU)IDC_REDEFINE_RECENTER, hInst, 0);
      hCtrl0_3 = CreateWindowEx(0, WC_STATIC, ("Pause key: "), WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT, 
                                18, 28, 158, 24, hwnd, (HMENU)IDC_PAUSE_LABEL, hInst, 0);
      hCtrl0_4 = CreateWindowEx(0, WC_STATIC, ("Recenter key: "), WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT, 
                                18, 76, 158, 24, hwnd, (HMENU)IDC_RECENTER_LABEL, hInst, 0);
      if(have_pause_key){
        snprintf(msg, 380, "Pause: %s", pause_key_desc);
        SetWindowText(hCtrl0_3, msg);
      }
      if(have_recenter_key){
        snprintf(msg, 380, "Recenter: %s", recenter_key_desc);
        SetWindowText(hCtrl0_4, msg);
      }
      
      nid.cbSize = sizeof(nid);
      nid.hWnd = hwnd;
      nid.uFlags = NIF_ICON | NIF_MESSAGE;
      nid.uCallbackMessage = WM_TRAY_DBL_CLICK;
      strncpy(nid.szInfo, "Info", 256);
      strncpy(nid.szInfoTitle, "Title", 64);
      nid.dwInfoFlags = 0;
      nid.hIcon = LoadIcon (hInst, MAKEINTRESOURCE(1001));
      return 0;
      break;
    case WM_SYSCOMMAND:
      if(wParam == SC_MINIMIZE){
        ShowWindow(hwnd, SW_HIDE);
        Shell_NotifyIcon(NIM_ADD, &nid);
      }
      break;
    case WM_TRAY_DBL_CLICK:
      if(lParam == WM_LBUTTONDOWN){
        ShowWindow(hwnd, SW_SHOW);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        SetForegroundWindow(hwnd);
      }
      break;
    case WM_DESTROY:
      Shell_NotifyIcon(NIM_DELETE, &nid);
      PostQuitMessage(0);
      return 0;
      break;      
    case WM_COMMAND:
      SetFocus(hwnd);
      switch(wParam){
        case IDC_REDEFINE_PAUSE:
          kbi_set_state(DEF_PAUSE);
          break;
        case IDC_REDEFINE_RECENTER:
          kbi_set_state(DEF_RECENTER);
          break;
        case IDQUIT:
          DestroyWindow(hwnd);
          break;
      }
      break;
    default:                      /* for messages that we don't deal with */
      return DefWindowProc (hwnd, message, wParam, lParam);
  }
  return 0;
}

void set_names(const std::string pause_keys, const std::string recenter_keys)
{
  SetWindowText(hCtrl0_3, pause_keys.c_str());
  SetWindowText(hCtrl0_4, recenter_keys.c_str());
}


