#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"

HINSTANCE hInst;
UINT_PTR timer;
HANDLE mutex;

HMODULE tvHandle;
typedef int (WINAPI *TIRViewsFun_t)(void);
TIRViewsFun_t TIRViewsVersion;
TIRViewsFun_t TIRViewsStart;
TIRViewsFun_t TIRViewsStop;

int load_fun(HMODULE lib, const char *fun_name, TIRViewsFun_t *fun)
{
  *fun = (TIRViewsFun_t)GetProcAddress(lib, fun_name);
  if(*fun == NULL){
    printf("Can't locate '%s' function!\n", fun_name);
    return 1;
  }
  return 0;
}

void message(const char *msg)
{
  MessageBox(NULL, msg, "Fake TrackIR problem", 0);
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    (void) lParam;
    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* SetDlgItemInt(hwndDlg, IDC_APPID, 2307, true); */
	    tvHandle = LoadLibrary("mfc42u.dll");
	    if(!tvHandle){
	      /* DWORD err = GetLastError(); */
	      message("Can't load mfc42u.dll needed by TIRViews.dll.\nTry installing MFC42 (e.g. using winetricks mfc42).");
              EndDialog(hwndDlg, 0);
	      return TRUE;
	    }
	    tvHandle = LoadLibrary("TIRViews.dll");
	    if(!tvHandle){
	      /* DWORD err = GetLastError(); */
	      message("Can't load TIRViews.dll -\n - please reinstall TrackIR firmware to get it.");
              EndDialog(hwndDlg, 0);
	      return TRUE;
	    }
	    if(load_fun(tvHandle, "TIRViewsVersion", &TIRViewsVersion) ||
		load_fun(tvHandle, "TIRViewsStart", &TIRViewsStart) ||
		load_fun(tvHandle, "TIRViewsStop", &TIRViewsStop)){
	      message("Couldn't locate all necessary functions in TIRViews.dll");
	      EndDialog(hwndDlg, 0);
	      return TRUE;
	    }
	    TIRViewsStart();
            return TRUE;

        case WM_CLOSE:
	    TIRViewsStop();
	    CloseHandle(mutex);
            EndDialog(hwndDlg, 0);
	    
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                /*
                 * TODO: Add more control ID's, when needed.
                 */
                case IDQUIT:
 	           TIRViewsStop();
                   CloseHandle(mutex);
                   EndDialog(hwndDlg, 0);
                    return TRUE;
            }
    }

    return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  (void) hPrevInstance;
  (void) lpCmdLine;
  (void) nShowCmd;
  int res;
  
  hInst = hInstance;
  
  mutex = CreateMutex(NULL, TRUE, "linuxtrack_wine_fake_trackir_mutex");
  if(mutex == NULL){
    message("Can't create mutex.");
    return 0;
  }
  
  if(WaitForSingleObject(mutex, 0) != WAIT_OBJECT_0){
    message("Fake TrackIR executable already running!.");
    CloseHandle(mutex);
    return 0;
  }
  
  // The user interface is a modal dialog box
  res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DialogProc);
  
  return res;
}


