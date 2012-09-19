#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  FILE *f;
  if((f = fopen("sig.bin", "rb")) == NULL){
    printf("Can't open sig.bin\n");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  uint8_t *data = (uint8_t*)malloc(size); 
  uint8_t *key = (uint8_t*)malloc(size); 
  memset(data, 0, size);
  memset(key, 0, size);
  fseek(f, 0, SEEK_SET);
  fread(data, 1, size, f);
  fclose(f);
  
  
  gzFile *gf;
  if((gf = gzopen("tir4.fw.gz", "rb")) == NULL){
    printf("Can't open sig.bin\n");
    return 1;
  }
  gzread(gf, key, size);
  gzclose(gf);
  
  long i;
  for(i = 0; i < size; ++i){
    data[i] ^= key[i];
    data[i] ^= 0x5A;
  }
  
  printf("char secret[] = {\n");
  for(i = 0; i < size; ++i){
    printf("0x%02X, ", data[i]);
    if(((i+1) % 10) == 0) printf("\n  ");
  }
  printf("};");
  return 0;
}

