#include "rest.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

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
      cnt = getline(&tmp_str, &tmp_str_size, f);
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



