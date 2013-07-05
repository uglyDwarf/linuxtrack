#include "linuxtrack.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dinput.h>
#include <map>
#include <string>
#include <iostream>
#include "utils.h"
#include <stdio.h>
#include "kbi_interface.h"

// DirectInput Variables
static LPDIRECTINPUT8 fDI; // Root DirectInput Interface
static LPDIRECTINPUTDEVICE8 fDIKeyboard; // The keyboard device
//static BYTE keystate[256];
static HANDLE kbd_event;
static DIDEVICEOBJECTDATA kbd_data[10];
static std::map<int, std::string> keymap;
static int code = 0;
static int pause_code = 0;
static int recenter_code = 0;
static state_t state;
static bool paused = false;

bool read_prefs()
{
  HKEY hkey = 0;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Linuxtrack", 0, 
    KEY_QUERY_VALUE, &hkey);
  if(!hkey){
    printf("Can't open registry key\n");
    return false;
  }
  
  unsigned char buf[1024];
  DWORD buf_len = sizeof(buf)-1; //To be sure there is a space for null
  LONG result = RegQueryValueEx(hkey, "DIKeys", NULL, NULL, buf, &buf_len);
  pause_code = recenter_code = 0;
  if((result == ERROR_SUCCESS) && (buf_len > 0)){
    sscanf((char *)buf, "0X%X 0X%X", &pause_code, &recenter_code);
  }
  RegCloseKey(hkey);
  std::cout<<"P: "<<pause_code<<" R: "<<recenter_code<<std::endl;
  if((pause_code != 0) && (recenter_code != 0)){
    return true;
  }else{
    return false;
  }
}

void write_prefs()
{
  HKEY  hkey   = 0;
  if((pause_code == 0) || (recenter_code == 0)){
    return;
  }
  
  LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Linuxtrack", 0, NULL, 
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
  if(result != ERROR_SUCCESS){
    printf("Can't create registry key...\n");
    return;
  }
  char *val = NULL;
  
  asprintf(&val, "0X%X 0X%X", pause_code, recenter_code);
  result = RegSetValueEx(hkey, "DIKeys", 0, REG_SZ, (unsigned char *)val, strlen(val)+1);
  free(val);
  val = NULL;
  if(result != ERROR_SUCCESS){
    printf("Can't store registry key...\n");
    return;
  }
  RegCloseKey(hkey);
}


static BOOL cbk(LPCDIDEVICEOBJECTINSTANCE lpddoi,
         LPVOID pvRef)
{
  (void) pvRef;
  keymap.insert(std::pair<int, std::string>(lpddoi->dwOfs, std::string(lpddoi->tszName)));
  //printf("%02X %s\n", lpddoi->dwOfs, lpddoi->tszName);
  return DIENUM_CONTINUE;
}

static void update_code(int scan)
{
  code &= 0xFF;
  code <<= 8;
  code += scan;
}

static int get_code()
{
  return code;
}

static void reset_code()
{
  code = 0;
}

static const std::string &str_code(int code)
{
  static std::string str;
  if(code > 255){
    str = kbi_check((code & 0xFF00) >> 8) + " + " + kbi_check(code & 0xFF);
  }else{
    str = kbi_check(code & 0xFF);
  }
  return str;
}

void send_keys_desc()
{
  static std::string pause_keys, recenter_keys;
  pause_keys = (pause_code != 0) ? str_code(pause_code) : "Not set!" ;
  recenter_keys = (recenter_code != 0) ? str_code(recenter_code) : "Not set!" ;
  set_names(pause_keys, recenter_keys);
}


bool kbi_init(HWND hwnd)
{
  HRESULT res;
  HRESULT      hr;
  // --- Start of DirectInput initialization ---
  // Create the abstract DirectInput connection
  DirectInput8Create(
    GetModuleHandle(NULL),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8,
    (void**)&fDI,
    NULL
  );
  if (fDI == NULL){
    MessageBox(NULL, "DirectInput Connection Creation Failed!", "Controler", MB_OK);
    return FALSE;
  }
  //unnamed autoreset event with default security attrs, initially unsignalled
  kbd_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(kbd_event == NULL){
    MessageBox(NULL, "Failed to create event!", "Controler", MB_OK);
    return FALSE;
  }
  
  // Create the connection to the keyboard device
  fDI->CreateDevice(GUID_SysKeyboard, &fDIKeyboard, NULL);
  if (fDIKeyboard){
    
    fDIKeyboard->EnumObjects((LPDIENUMDEVICEOBJECTSCALLBACK)cbk, NULL, DIDFT_ALL);
    fDIKeyboard->SetDataFormat(&c_dfDIKeyboard);
    fDIKeyboard->SetCooperativeLevel(
      hwnd,
      DISCL_BACKGROUND | DISCL_NONEXCLUSIVE
    );

    DIPROPDWORD  dipdw; 
    dipdw.diph.dwSize = sizeof(DIPROPDWORD); 
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    dipdw.diph.dwObj = 0; 
    dipdw.diph.dwHow = DIPH_DEVICE; 
    dipdw.dwData = 10; 
    hr = fDIKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
    if((hr != DI_OK) && (hr != DI_PROPNOEFFECT)){
      MessageBox(NULL, "Failed to set kbd property!", "Controler", MB_OK);
      return FALSE;
    }
    res = fDIKeyboard->SetEventNotification(kbd_event);
    if((res != DI_OK) && (res != DI_POLLEDDEVICE)){
      MessageBox(NULL, "Failed to register event!", "Controler", MB_OK);
      return FALSE;
    }
    fDIKeyboard->Acquire();
  }else{
    MessageBox(NULL, "DirectInput Keyboard initialization Failed!", "Controler", MB_OK);
    return FALSE;
  } 
  // --- End of DirectInput initialization ---
  read_prefs();
  send_keys_desc();
  if(ltr_init("Default") != 0){
    MessageBox(NULL, "Can't start linuxtrack!!!", "Controler", MB_OK);
    exit(1);
  }
  return TRUE;
}

void kbi_close(void)
{
  ltr_shutdown();
  write_prefs();
  fDIKeyboard->Unacquire();    // make sure the keyboard is unacquired
  fDIKeyboard->SetEventNotification(NULL);
  CloseHandle(kbd_event);
  fDI->Release();    // close DirectInput before exiting
}

const std::string &kbi_check(int num)
{
  std::map<int, std::string>::const_iterator i;
  static std::string unkn("Unknown Key"); 
  i = keymap.find(num);
  if(i != keymap.end()){
    return i->second;
  }
  //std::cout<<"Key "<<num<<" unknown!"<<std::endl;
  return unkn;
}

void kbi_msg_loop()
{
  DWORD res;
  DWORD dwWait = 0;
  MSG msg;
  DWORD events;
  while(1){
    res = MsgWaitForMultipleObjects(1, &kbd_event, FALSE, dwWait, QS_ALLINPUT);
    dwWait = 0;
    switch(res){
      case WAIT_OBJECT_0:
        fDIKeyboard->Acquire();
        events = 10;
        fDIKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), kbd_data, &events, 0);
        for(DWORD j = 0; j < events; ++j){
          if(kbd_data[j].dwData & 0x80){
            //key pressed
            update_code(kbd_data[j].dwOfs);
            if(get_code() == pause_code){
              if(paused){
                ltr_wakeup();
                paused = false;
              }else{
                ltr_suspend();
                paused = true;
              }
            }
            if(get_code() == recenter_code){
              ltr_recenter();
            }
          }else{
            //key released
            //std::cout<<"KEY(s): "<< str_code(get_code()) << std::endl;
            if(state != RECEIVING){
              if(state == DEF_PAUSE){
                pause_code = get_code();
              }
              if(state == DEF_RECENTER){
                recenter_code = get_code();
              }
              send_keys_desc();
              state = RECEIVING;
            }
            reset_code();
          }
          //std::cout<<"Have "<<kbi_check(kbd_data[j].dwOfs)<<((kbd_data[j].dwData & 0x80) ? "Pressed" : "Released")<<std::endl;
        }
        break;
      case WAIT_OBJECT_0+1:
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){ 
            if (msg.message == WM_QUIT) { 
                goto exit_app; 
            } 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
        break;
      default:
        dwWait = INFINITE; 
        break;
    }
  }
  exit_app: ;
}

void kbi_set_state(state_t s)
{
  state = s;
}


