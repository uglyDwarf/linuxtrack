#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <stdbool.h>
#include <zlib.h>
#include "secret.c"

bool get_sig(uint8_t buf[400])
{
  char *file = "/.linuxtrack/tir_firmware/tir4.fw.gz";
  char *home = getenv("HOME");
  char *path = (char *)malloc(strlen(home) + sizeof(file) + 100);
  strcpy(path, home);
  strcat(path, file);
  
  long size = sizeof(secret);
  uint8_t *key = (uint8_t*)malloc(size); 
  memset(key, 0, size);
  gzFile *gf;
  if((gf = gzopen(path, "rb")) == NULL){
    printf("Can't open '%s'!\n", path);
    return false;
  }
  gzread(gf, key, size);
  gzclose(gf);
  
  long i;
  for(i = 0; i < size; ++i){
    buf[i] = secret[i] ^ key[i] ^ 0x5A;
  }
  return true;
}

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  uint8_t buf[400];
  get_sig(buf);
  FILE *f;
  f = fopen("sig.txt", "wb");
  fwrite(buf, 1, 400, f);
  fclose(f);
  return 0;
}

