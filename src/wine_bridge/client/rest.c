#define _GNU_SOURCE
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include "rest.h"

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif


static ssize_t my_getline(char **lineptr, size_t *n, FILE *f)
{
#ifndef DARWIN
  return getline(lineptr, n, f);
#else
  size_t cnt = 0;
  char *res = fgetln(f, &cnt);
  if(res == NULL){
    return 0;
  }
  if(*lineptr == NULL){
    //there is no \0, so cnt has to be increased to contain it
    *n = ((cnt+1) > 4096) ? cnt+1 : 4096;
    *lineptr = (char *)malloc(*n);
  }else{
    if(*n < cnt+1){
      *n = ((cnt+1) > 4096) ? cnt+1 : 4096;
      *lineptr = (char *)realloc(*lineptr, *n);
    }
  }
  memcpy(*lineptr, res, cnt);
  (*lineptr)[cnt] = '\0';
  return cnt;
#endif
}


bool game_data_get_desc(int id, game_desc_t *gd)
{
  FILE *f = NULL;
  char *home = getenv("HOME");
  char *path1 = (char *)malloc(200 + strlen(home));
  sprintf(path1, "%s/.config/linuxtrack/tir_firmware/gamedata.txt", home);
    if((f = fopen(path1, "r"))== NULL){
      printf("Can't open data file '%s'!\n", path1);
      return false;
    }
    int tmp_id;
    size_t tmp_str_size = 4096;
    size_t tmp_code_size = 4096;
    char *tmp_str = (char *)malloc(tmp_str_size);
    char *tmp_code = (char *)malloc(tmp_code_size);
    unsigned int c1, c2;
    int cnt;
    gd->name = NULL;
    while(!feof(f)){
      cnt = my_getline(&tmp_str, &tmp_str_size, f);
      if(cnt > 0){
        if(tmp_str_size > tmp_code_size){
          tmp_code = (char *)realloc(tmp_code, tmp_str_size);
        }
        cnt = sscanf(tmp_str, "%d \"%[^\"]\" (%08x%08x)", &tmp_id, tmp_code, &c1, &c2);
        if(cnt == 2){
          if(tmp_id == id){
            gd->name = strdup(tmp_code);
            gd->encrypted = false;
            gd->key1 = gd->key2 = 0;
            break;
          }
        }else if(cnt == 4){
          if(tmp_id == id){
            gd->name = strdup(tmp_code);
            gd->encrypted = true;
            gd->key1 = c1;
            gd->key2 = c2;
            break;
          }
        }
      }
    }
    fclose(f);
    free(tmp_code);
    free(tmp_str);
    free(path1);
  return gd->name != NULL;
}

bool getSomeSeriousPoetry(char *verse1, char *verse2)
{
  bool res = true;
  char *home = getenv("HOME");
  char *path1 = (char *)malloc(200 + strlen(home));
  char *path2 = (char *)malloc(200 + strlen(home));
  sprintf(path1, "%s/.config/linuxtrack/tir_firmware/poem1.txt", home);
  sprintf(path2, "%s/.config/linuxtrack/tir_firmware/poem2.txt", home);
  FILE *f1 = fopen(path1, "rb");
  memset(verse1, 0, 200);
  if(f1 != NULL){
    if(fread(verse1, 1, 200, f1) == 0){
      printf("Cant read dll signature('%s')!\n", path1);
      res = false;
    }
    printf("DLL SIGNATURE: %s\n", verse1);
    fclose(f1);
  }else{
    printf("Can't open dll signature ('%s')!\n", path1);
    res = false;
  }
  free(path1);
  FILE *f2 = fopen(path2, "rb");
  memset(verse2, 0, 200);
  if(f2 != NULL){
    if(fread(verse2, 1, 200, f2) == 0){
      perror("fread");
      printf("Cant read app signature('%s')!\n", path2);
      res = false;
    }
    printf("APP SIGNATURE: %s\n", verse2);
    fclose(f2);
  }else{
    printf("Cant open app signature('%s')!\n", path2);
    res = false;
  }
  free(path2);
  return res;
}

bool getDebugFlag(const int flag)
{
  char *dbg_flags = getenv("LINUXTRACK_DBG");
  if(dbg_flags == NULL) return false;
  if(strchr(dbg_flags, flag) != NULL){
    return true;
  }else{
    return false;
  }
}

char *file_path(const char *file)
{
  HKEY  hkey   = 0;
  RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", 0,
    KEY_QUERY_VALUE, &hkey);
  if(!hkey){
    printf("Can't open registry key\n");
    return NULL;
  }

  BYTE path[1024];
  DWORD buf_len = 1024;
  LONG result = RegQueryValueEx(hkey, "Path", NULL, NULL, path, &buf_len);
  char *full_path = NULL;
  int res = -1;
  if(result == ERROR_SUCCESS && buf_len > 0){
    res = asprintf(&full_path, "%s\\%s", path, file);
  }
  RegCloseKey(hkey);
  if(res > 0){
    return full_path;
  }else{
    return NULL;
  }
}

bool tryExclusiveLock(const char *file)
{
  HANDLE f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(f == INVALID_HANDLE_VALUE){
    return false;
  }
  OVERLAPPED overlapvar;
  overlapvar.Offset = 0;
  overlapvar.OffsetHigh = 0;
  overlapvar.hEvent = 0;
  bool res = LockFileEx(f, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 10, 0, &overlapvar);
  CloseHandle(f);
  return res;
}

bool sharedLock(const char *file)
{
  HANDLE f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(f == INVALID_HANDLE_VALUE){
    return false;
  }
  OVERLAPPED overlapvar;
  overlapvar.Offset = 0;
  overlapvar.OffsetHigh = 0;
  overlapvar.hEvent = 0;
  bool res = LockFileEx(f, LOCKFILE_FAIL_IMMEDIATELY, 0, 10, 0, &overlapvar);
  CloseHandle(f);
  return res;
}

bool runFile(const char *file)
{
  (void) file;
  char *exe = file_path(file);
  if(exe == NULL){
    return false;
  }
  char *q_exe = NULL;
  if(asprintf(&q_exe, "\"%s\"", exe) < 0){
    free(exe);
    return false;
  }
  free(exe);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  ZeroMemory(&pi, sizeof(pi));
printf("Going to run this: %s\n", q_exe);
  bool res = CreateProcess(NULL, q_exe, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
  if(!res){
    printf("Failed! (%d)\n", GetLastError());
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(si.hStdInput);
  CloseHandle(si.hStdOutput);
  CloseHandle(si.hStdError);
  free(q_exe);
  return res;
}

