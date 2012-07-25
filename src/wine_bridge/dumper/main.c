#define _GNU_SOURCE
#include <stdio.h>
#include "mxml.h"
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/stat.h>
#include <string.h>

//First 5 bytes is MD5 hash of "NaturalPoint"
static uint8_t key[] = {0x0e, 0x9a, 0x63, 0x71, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t S[256] = {0};

static char *decoded = NULL;

static mxml_node_t *xml = NULL;
static mxml_node_t *tree = NULL;

static void ksa(uint8_t key[], size_t len)
{
  unsigned int i, j;
  for(i = 0; i < 256; ++i){
    S[i] = i;
  }
  j = 0;
  for(i = 0; i < 256; ++i){
    j = (j + S[i] + key[i % len]) % 256;
    uint8_t tmp = S[i];
    S[i] = S[j];
    S[j] = tmp;
  }
}

static uint8_t rc4()
{
  static uint8_t i = 0;
  static uint8_t j = 0;

  i += 1;
  j += S[i];
  uint8_t tmp = S[i];
  S[i] = S[j];
  S[j] = tmp;
  return S[(S[i] + S[j]) % 256];
}

static bool decrypt_file(char *fname)
{
  ksa(key, 16);
  FILE *inp;
  struct stat fst;

  if((inp = fopen(fname, "rb")) == NULL){
    printf("Can't open input file '%s'", fname);
    return false;
  }

  if(fstat(fileno(inp), &fst) != 0){
    printf("Cannot stat file '%s'\n", fname);
    return false;
  }
  if((decoded = (char *)malloc(fst.st_size+1)) == NULL){
    printf("malloc failed!\n");
    return false;
  }
  memset(decoded, 0, fst.st_size+1);
  size_t len, i;
  len = fread(decoded, 1, fst.st_size, inp);
  for(i = 0; i < fst.st_size; ++i) decoded[i] ^= rc4();
  fclose(inp);
  return true;
}

bool game_data_init(char *fname)
{
  static bool initialized = false;
  if(initialized){
    return true;
  }
  if(!decrypt_file(fname)){
    printf("Error decrypting file!\n");
    return false;
  }
  xml = mxmlNewXML("1.0");
  tree = mxmlLoadString(xml, decoded, MXML_TEXT_CALLBACK);
  return (tree != NULL);
}

void game_data_close()
{
  mxmlDelete(tree);
  free(decoded);
}


int main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(stderr, "usage: dumper sgl.dat\n");
    return 1;
  }
  
  if(!game_data_init(argv[1])){
    fprintf(stderr, "Can't open the data file!\n");
    return 1;
  }
  
  mxml_node_t *game;
  const char *name;
  const char *id;
  for(game = mxmlFindElement(tree, tree, "Game", NULL, NULL, MXML_DESCEND); 
      game != NULL;
      game =  mxmlFindElement(game, tree, "Game", NULL, NULL, MXML_DESCEND)){
    name = mxmlElementGetAttr(game, "Name");
    id = mxmlElementGetAttr(game, "Id");
      
    mxml_node_t *appid = mxmlFindElement(game, game, "ApplicationID", NULL, NULL, MXML_DESCEND);
    if(appid == NULL){
      printf("%s \"%s\"\n", id, name);
    }else{
      printf("%s \"%s\" (%s)\n", id, name, appid->child->value.text.string);
    }
  }  
  
  game_data_close();
  return 0;
}



