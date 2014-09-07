#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif


#include "rest.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>


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
  char *path1 = malloc(200 + strlen(home));
  sprintf(path1, "%s/.config/linuxtrack/tir_firmware/gamedata.txt", home);
    if((f = fopen(path1, "r"))== NULL){
      printf("Can't open data file '%s'!\n", path1);
      return false;
    }
    int tmp_id;
    size_t tmp_str_size = 4096; 
    size_t tmp_code_size = 4096;
    char *tmp_str = malloc(tmp_str_size);
    char *tmp_code = malloc(tmp_code_size);
    unsigned int c1, c2;
    int cnt;
    gd->name = NULL;
    while(!feof(f)){
      cnt = my_getline(&tmp_str, &tmp_str_size, f);
      if(cnt > 0){
        if(tmp_str_size > tmp_code_size){
          tmp_code = realloc(tmp_code, tmp_str_size);
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
  char *path1 = malloc(200 + strlen(home));
  char *path2 = malloc(200 + strlen(home));
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



