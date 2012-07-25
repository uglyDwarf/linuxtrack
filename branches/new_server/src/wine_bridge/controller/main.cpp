#include "linuxtrack.h"
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "resource.h"

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

bool read_prefs()
{
  HKEY hkey = 0;
  int res = 0;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Linuxtrack", 0, 
    KEY_QUERY_VALUE, &hkey);
  if(!hkey){
    printf("Can't open registry key\n");
    return false;
  }
  
  unsigned char buf[1024];
  DWORD buf_len = sizeof(buf)-1; //To be sure there is a space for null
  LONG result = RegQueryValueEx(hkey, "Keys", NULL, NULL, buf, &buf_len);
  if((result == ERROR_SUCCESS) && (buf_len > 0)){
    res = sscanf((char *)buf, "%d %d %d %d", 
                 &pause_vkey, &pause_scancode, &recenter_vkey, &recenter_scancode);
  }
  RegCloseKey(hkey);
  if(res == 4){
    have_pause_key = true;
    have_recenter_key = true;
    GetKeyNameText(pause_scancode, pause_key_desc, 250);
    GetKeyNameText(recenter_scancode, recenter_key_desc, 250);
    return true;
  }else{
    have_pause_key = false;
    have_recenter_key = false;
    return false;
  }
}

void write_prefs()
{
  HKEY  hkey   = 0;
  if(!(have_pause_key && have_recenter_key)){
    return;
  }
  
  LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Linuxtrack", 0, NULL, 
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
  if(result != ERROR_SUCCESS){
    printf("Can't create registry key...\n");
    return;
  }
  char *val = NULL;
  
  asprintf(&val, "%d %d\n%d %d\n", pause_vkey, pause_scancode, recenter_vkey, recenter_scancode);
  result = RegSetValueEx(hkey, "Keys", 0, REG_SZ, (unsigned char *)val, strlen(val)+1);
  free(val);
  val = NULL;
  if(result != ERROR_SUCCESS){
    printf("Can't store registry key...\n");
    return;
  }
  RegCloseKey(hkey);
}

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
  MSG messages;            /* Here messages to the application are saved */
  WNDCLASSEX wincl;        /* Data structure for the windowclass */
  
  /* The Window structure */
  hInst = hThisInstance;
  
  read_prefs();
  printf("Prefs read!\n");
  if(ltr_init("Default") != 0){
    printf("Can't start linuxtrack!!!\n");
    exit(1);
  }
  printf(">>>Linuxtrack initialized!?!\n");
  wincl.hInstance = hThisInstance;
  wincl.lpszClassName = szClassName;
  wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
  wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
  wincl.cbSize = sizeof (WNDCLASSEX);
  
  /* Use default icon and mouse-pointer */
  wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
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
  
  /* Run the message loop. It will run until GetMessage() returns 0 */
  while (GetMessage (&messages, NULL, 0, 0)){
    /* Translate virtual-key messages into character messages */
    TranslateMessage(&messages);
    /* Send message to WindowProcedure */
    DispatchMessage(&messages);
  }

  /* The program return-value is 0 - The value that PostQuitMessage() gave */
  return messages.wParam;
}


VOID CALLBACK TimerProcedure(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  if(have_pause_key){
    if(GetAsyncKeyState(pause_vkey) | 1 > 1){
      if(pause_debouncer == 0){
        paused = !paused;
        printf("Pause key pressed!\n");
        if(paused){
          ltr_suspend();
        }else{
          ltr_wakeup();
        }
      }
      pause_debouncer = 5;
    }else{
      if(pause_debouncer > 0){
        --pause_debouncer;
      }
    }
  }
  if(have_recenter_key){
    if(GetAsyncKeyState(recenter_vkey) | 1 > 1){
      printf("Recenter key pressed!\n");
      ltr_recenter();
    }
  }
}

/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  char msg[384];
  static int cntr1 = 0;
  //printf("Msg: %d  %d\n", hwnd, message);
  switch(message){                  /* handle the messages */
    case WM_DESTROY:
      PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
      ltr_shutdown();
      break;
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
      SetTimer(hwnd, 0, 50, TimerProcedure);
        break;
    case WM_KEYDOWN:
      //sprintf(msg, "Pressed %d times!", cntr1);
      //printf("Keydown %s!\n",msg);
      switch(redef_state){
        case REDEF_PAUSE:
          pause_vkey = wParam;
          pause_scancode = lParam;
          have_pause_key = true;
          GetKeyNameText(lParam, pause_key_desc, 250);
          snprintf(msg, 380, "Pause: %s", pause_key_desc);
          SetWindowText(hCtrl0_3, msg);
          break;
        case REDEF_RECENTER:
          recenter_vkey = wParam;
          recenter_scancode = lParam;
          have_recenter_key = true;
          GetKeyNameText(lParam, recenter_key_desc, 250);
          snprintf(msg, 380, "Recenter: %s", recenter_key_desc);
          SetWindowText(hCtrl0_4, msg);
          break;
      }
      redef_state = REDEF_NONE;
      break;
    case WM_COMMAND:
      SetFocus(hwnd);
      switch(wParam){
        case IDC_REDEFINE_PAUSE:
          redef_state = REDEF_PAUSE;
          break;
        case IDC_REDEFINE_RECENTER:
          redef_state = REDEF_RECENTER;
          break;
        case IDQUIT:
          write_prefs();
          DestroyWindow(hwnd);
          break;
      }
      break;
    default:                      /* for messages that we don't deal with */
      return DefWindowProc (hwnd, message, wParam, lParam);
  }
  return 0;
}

